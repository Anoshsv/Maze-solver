#include <Arduino.h>
#include <WiFi.h>
#include "PIDTuner.h"
#include "WebHandler.h"
#include "VelocityLogger.h"

#include <math.h>

#include "VelocityTest.h"


// ================== Config ==================

const float WHEEL_DIAMETER_MM = 44.0;
const float WHEEL_CIRCUMFERENCE_MM = 138.16; // PI * WHEEL_DIAMETER_MM
const int TICKS_PER_REV_M1 = 665;  
const int TICKS_PER_REV_M2 = 665;


// ================== Motor Pin Assignments ==================

#define ML_PWM 21
#define ML_IN1 18
#define ML_IN2 19
#define ML_ENC_A 32
#define ML_ENC_B 33

#define MR_PWM 25
#define MR_IN1 26
#define MR_IN2 27
#define MR_ENC_A 22
#define MR_ENC_B 23

// PWM config for ESP32 LEDC
const int PWM_FREQ = 5000;
const int PWM_RES = 10; // 10-bit -> 0..1023

#define ssid "GalaxyM31F974"
#define password "kmrw8220"


// --- Fast GPIO Read (IRAM safe, global helper) ---
static inline bool IRAM_ATTR fastDigitalRead(uint8_t pin) {
    if (pin < 32)
        return (GPIO.in >> pin) & 0x1;
    else
        return (GPIO.in1.val >> (pin - 32)) & 0x1;
}

// ================== Motor Class ==================
class Motor {
public:
    const char* name;
    uint8_t pwmPin, in1Pin, in2Pin, encAPin, encBPin;
    int pwmChannel;
    int pwmResolution = 10;

    volatile long encoderTicks = 0;
    long lastEncoderTicks = 0;
    unsigned long lastSampleMicros = 0;
    float filteredVel = 0; // ticks/sec

    Motor(const char* motorName,
          uint8_t pwm, uint8_t in1, uint8_t in2,
          uint8_t encA, uint8_t encB,
          int channel)
        : name(motorName),
          pwmPin(pwm), in1Pin(in1), in2Pin(in2),
          encAPin(encA), encBPin(encB),
          pwmChannel(channel) {}

    void begin(int pwmFreq, int pwmRes) {
        pwmResolution = pwmRes;
        pinMode(in1Pin, OUTPUT);
        pinMode(in2Pin, OUTPUT);
        pinMode(encAPin, INPUT_PULLDOWN);
        pinMode(encBPin, INPUT_PULLDOWN);

        ledcSetup(pwmChannel, pwmFreq, pwmRes);
        ledcAttachPin(pwmPin, pwmChannel);
        ledcWrite(pwmChannel, 0);

        attachInterruptArg(digitalPinToInterrupt(encAPin), isrHandler, this, CHANGE);
        Serial.printf("[%s] Initialized. ENC: A=%d, B=%d, PWM=%d, CH=%d\n",
                      name, encAPin, encBPin, pwmPin, pwmChannel);
    }

    void resetEncoderState() {
        noInterrupts();
        encoderTicks = 0;
        lastEncoderTicks = 0;
        lastSampleMicros = 0;
        filteredVel = 0.0f;
        interrupts();
    }

    void setPWM(float value) {
        value = constrain(value, -1023.0f, 1023.0f);
        int duty = (int)fabs(value);
        const int maxDuty = (1 << pwmResolution) - 1;
        duty = constrain(duty, 0, maxDuty);

        if (value > 0) {
            digitalWrite(in1Pin, HIGH);
            digitalWrite(in2Pin, LOW);
        } else if (value < 0) {
            digitalWrite(in1Pin, LOW);
            digitalWrite(in2Pin, HIGH);
        } else {
            digitalWrite(in1Pin, LOW);
            digitalWrite(in2Pin, LOW);
        }
        ledcWrite(pwmChannel, duty);
    }

    float readVelocity(float samplePeriod, float alpha) {
        unsigned long now = micros();
        float dt = (now - lastSampleMicros) / 1e6f;
        if (lastSampleMicros == 0) {
            lastSampleMicros = now;
            lastEncoderTicks = encoderTicks;
            return NAN;
        }
        if (dt < samplePeriod) return NAN;

        noInterrupts();
        long encNow = encoderTicks;
        interrupts();

        long delta = encNow - lastEncoderTicks;
        lastEncoderTicks = encNow;
        lastSampleMicros = now;

        float rawVel = delta / dt; // ticks/s
        filteredVel = (1.0f - alpha) * filteredVel + alpha * rawVel;
        return filteredVel;
    }

    void stop() {
        digitalWrite(in1Pin, LOW);
        digitalWrite(in2Pin, LOW);
        ledcWrite(pwmChannel, 0);
    }

    float getRevolutions() {
        int TPR = (pwmChannel == 0) ? TICKS_PER_REV_M1 : TICKS_PER_REV_M2;
        return (float)encoderTicks / TPR;
    }

    float getDistanceMM() {
        return getRevolutions() * WHEEL_CIRCUMFERENCE_MM;
    }


private:
    static void IRAM_ATTR isrHandler(void* arg) {
        Motor* m = static_cast<Motor*>(arg);
        bool a = fastDigitalRead(m->encAPin);
        bool b = fastDigitalRead(m->encBPin);
        m->encoderTicks += (a == b) ? 1 : -1;
    }
};


// ================== Global Objects ==================
PIDTuner tuner;
VelocityLogger logger;
VelocityTest vtest; 
WebHandler web(&tuner, &logger, &vtest);

Motor leftMotor("LEFT", ML_PWM, ML_IN1, ML_IN2, ML_ENC_A, ML_ENC_B, 0);
Motor rightMotor("RIGHT", MR_PWM, MR_IN1, MR_IN2, MR_ENC_A, MR_ENC_B, 1);

// ================== Motion & PID Variables ==================
float samplePeriod = 0.02f;  // 20ms

float alpha = 0.3f; // smoothing factor


float prevErrorL = 0, integralL = 0;
float prevErrorR = 0, integralR = 0;
float prevYawError = 0, integralYaw = 0;

// ================== Sequence Control ==================
bool sequenceRunning = false;
unsigned long sequenceStart = 0;



// ================== Sequence Functions (Linked to Web) ==================
void startSequence() {
    logger.start();
    sequenceRunning = true;
    sequenceStart = millis();

    // --- Reset all dynamic states ---
    prevErrorL = 0;
    integralL = 0;
    prevErrorR = 0;
    integralR = 0;
    prevYawError = 0;
    integralYaw = 0;

    leftMotor.resetEncoderState();
    rightMotor.resetEncoderState();


    // --- Compute per-run sampling period for constant sample count ---
    const size_t N = VelocityLogger::MAX_SAMPLES;  // e.g., 256
    float Ttotal = vtest.accelTime + vtest.steadyTime + vtest.decelTime;

    // Compute new sample period (seconds)
    float Ts = Ttotal / (float)N;

    // Apply safety clamps to avoid too-fast or too-slow sampling
    const float Ts_min = 0.005f;  // 5 ms (200 Hz max)
    const float Ts_max = 0.2f;    // 200 ms (5 Hz min)
    if (Ts < Ts_min) Ts = Ts_min;
    if (Ts > Ts_max) Ts = Ts_max;

    samplePeriod = Ts;

    Serial.printf("Sequence started: Total=%.2fs, N=%d, Ts=%.3fs (%.1f Hz)\n",
                  Ttotal, N, samplePeriod, 1.0f / samplePeriod);
}


void stopSequence() {
    sequenceRunning = false;
    logger.stop();
    leftMotor.stop();
    rightMotor.stop();

    prevErrorL = 0;
    integralL = 0;
    prevErrorR = 0;
    integralR = 0;
    prevYawError = 0;
    integralYaw = 0;


    Serial.println("Sequence stopped");
}

bool isSequenceRunning() {
    return sequenceRunning;
}

float generateTrapezoidalVelocity(float t) {
    return vtest.generateProfile(t);
}

void setBothMotorsPWM(float lvalue, float rvalue) {
    leftMotor.setPWM(lvalue);
    rightMotor.setPWM(rvalue);
}


// ================== PID Controller ==================

int basePWM(float targetVel) {
    const int n = 11;
    float speeds[n] = {0, 210, 605, 1045, 1490, 1930, 2380, 2830, 3280, 3750, 4100};
    int pwms[n]      = {0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};

    if (targetVel <= speeds[0]) return pwms[0];
    if (targetVel >= speeds[n - 1]) return pwms[n - 1];

    for (int i = 0; i < n - 1; i++) {
        if (targetVel >= speeds[i] && targetVel <= speeds[i + 1]) {
            float ratio = (targetVel - speeds[i]) / (speeds[i + 1] - speeds[i]);
            return pwms[i] + ratio * (pwms[i + 1] - pwms[i]);
        }
    }
    return pwms[n - 1];
}


float computePID(float desired, float actual, float &prevError, float &integral,
                 float Kp, float Ki, float Kd,float dt)
{
    float error = desired - actual;
    integral += error * dt;
    integral = constrain(integral, -200.0f, 200.0f);

    float derivative = (error - prevError) / dt;
    prevError = error;

    return Kp * error + Ki * integral + Kd * derivative;
}

// ================== Setup ==================
void setup() {
    Serial.begin(115200);
    Serial.println("Starting Micromouse PID Controller...");

    leftMotor.begin(PWM_FREQ, PWM_RES);
    rightMotor.begin(PWM_FREQ, PWM_RES);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected. IP address: " + WiFi.localIP().toString());

    // Note: WebHandler::begin will call tuner.begin(server_) internally.
    web.begin();


    Serial.println("System ready. Connect to web UI to start test.");
}

//================== Main Loop ==================

void loop() {
    web.handleClient();

    if (!sequenceRunning) return;

    float t = (millis() - sequenceStart) / 1000.0f;

    float desiredVel = generateTrapezoidalVelocity(t);

    float vl = leftMotor.readVelocity(samplePeriod, alpha);
    float vr = rightMotor.readVelocity(samplePeriod, alpha);

    if (isnan(vl) || isnan(vr)) return;

    int baseL = basePWM(desiredVel);
    int baseR = basePWM(desiredVel);

    float pidOutL = computePID(desiredVel, vl, prevErrorL, integralL,
                               tuner.KpL, tuner.KiL, tuner.KdL, samplePeriod);

    float pidOutR = computePID(desiredVel, vr, prevErrorR, integralR,
                               tuner.KpR, tuner.KiR, tuner.KdR, samplePeriod);

    // --- Compute yaw PID (normalized) ---
    float yawErrorMM = rightMotor.getDistanceMM() - leftMotor.getDistanceMM();
    const float maxYawErrorMM = 50.0f;                // typical max difference
    float yawError = constrain(yawErrorMM / maxYawErrorMM, -1.0f, 1.0f);

    float yawOut = computePID(0.0f, yawError, prevYawError, integralYaw,
                              tuner.KpYaw, tuner.KiYaw, tuner.KdYaw, samplePeriod);

    // --- Scale yaw output to PWM contribution ---
    const float maxYawPWM = 200.0f;
    float yawCorrection = constrain(yawOut * maxYawPWM, -maxYawPWM, maxYawPWM);

    // --- Combine PID outputs + yaw correction ---
    float pwmL = constrain(baseL + pidOutL + yawCorrection, -1023.0f, 1023.0f);
    float pwmR = constrain(baseR + pidOutR - yawCorrection, -1023.0f, 1023.0f);

    setBothMotorsPWM(pwmL, pwmR);

    // --- Log distance/velocities ---
    float avgDistance = 0.5f * (leftMotor.getDistanceMM() + rightMotor.getDistanceMM());
    logger.log(desiredVel, vl, desiredVel, vr, avgDistance);

    // --- Stop sequence if finished ---
    if (t >= vtest.accelTime + vtest.steadyTime + vtest.decelTime) 
        stopSequence();
}

