// #include <Arduino.h>
// #include <WiFi.h>
// #include "PIDTuner.h"
// #include "WebHandler.h"
// #include "VelocityLogger.h"

// #include <math.h>

// #include "VelocityTest.h" //TEST


// // ================== Config Pins ==================
// #define ENCODER_A_PIN 22
// #define ENCODER_B_PIN 23
// #define MOTOR_ENA_PIN 25
// #define MOTOR_A1_PIN 26
// #define MOTOR_A2_PIN 27

// // PWM config for ESP32 LEDC
// const int PWM_CHANNEL = 0;
// const int PWM_FREQ = 5000;
// const int PWM_RES = 10; // 10-bit -> 0..1023

// #define ssid "GalaxyM31F974"
// #define password "kmrw8220"

// // ================== Global Objects ==================
// PIDTuner tuner;
// VelocityLogger logger;
// VelocityTest vtest; //TEST
// WebHandler web(&tuner, &logger, &vtest); //TEST

// // ================== Motion & PID Variables ==================
// float samplePeriod = 0.02f;  // 20ms

// float alpha = 0.3f; // smoothing factor
// float filteredVel = 0;

// volatile long encoderTicks = 0;
// long lastEncoderTicks = 0;

// float prevError = 0;
// float integral = 0;

// // ================== Sequence Control ==================
// bool sequenceRunning = false;
// unsigned long sequenceStart = 0;
// unsigned long lastSampleMicros = 0;

// // ================== Encoder ISR ==================
// IRAM_ATTR void encoderISR() {
//     bool a = digitalRead(ENCODER_A_PIN);
//     bool b = digitalRead(ENCODER_B_PIN);
//     encoderTicks += (a == b) ? 1 : -1;
// }

// // ================== Sequence Functions (Linked to Web) ==================
// void startSequence() {
//     logger.start();
//     sequenceRunning = true;
//     sequenceStart = millis();

//     // --- Reset all dynamic states ---
//     prevError = 0;
//     integral = 0;
//     encoderTicks = 0;
//     lastEncoderTicks = 0;
//     filteredVel = 0;
//     lastSampleMicros = 0;

//     // --- Compute per-run sampling period for constant sample count ---
//     const size_t N = VelocityLogger::MAX_SAMPLES;  // e.g., 256
//     float Ttotal = vtest.accelTime + vtest.steadyTime + vtest.decelTime;

//     // Compute new sample period (seconds)
//     float Ts = Ttotal / (float)N;

//     // Apply safety clamps to avoid too-fast or too-slow sampling
//     const float Ts_min = 0.005f;  // 5 ms (200 Hz max)
//     const float Ts_max = 0.2f;    // 200 ms (5 Hz min)
//     if (Ts < Ts_min) Ts = Ts_min;
//     if (Ts > Ts_max) Ts = Ts_max;

//     samplePeriod = Ts;

//     Serial.printf("Sequence started: Total=%.2fs, N=%d, Ts=%.3fs (%.1f Hz)\n",
//                   Ttotal, N, samplePeriod, 1.0f / samplePeriod);
// }



// void stopSequence() {
//     sequenceRunning = false;
//     logger.stop();
//     // stop motor
//     digitalWrite(MOTOR_A1_PIN, LOW);
//     digitalWrite(MOTOR_A2_PIN, LOW);
//     ledcWrite(PWM_CHANNEL, 0);
//     Serial.println("Sequence stopped");
// }

// bool isSequenceRunning() {
//     return sequenceRunning;
// }

// // ================== Trapezoidal Velocity Profile ==================
// // float generateTrapezoidalVelocity(float t) {
// //     const float accel = 1.0f, steady = 3.0f, decel = 1.0f;

// //     if (t < accel)
// //         return maxVel * (t / accel);
// //     else if (t < accel + steady)
// //         return maxVel;
// //     else if (t < accel + steady + decel)
// //         return maxVel * (1.0f - (t - accel - steady) / decel);
// //     else
// //         return 0.0f;
// // }

// float generateTrapezoidalVelocity(float t) {
//     return vtest.generateProfile(t);
// }


// // ================== Velocity Reading ==================
// float readActualVelocity() {
//     unsigned long nowMicros = micros();
//     float dt = (nowMicros - lastSampleMicros) / 1e6f;  // seconds
//     if (lastSampleMicros == 0) {
//         // first sample: initialize timestamp and return NAN so controller waits
//         lastSampleMicros = nowMicros;
//         lastEncoderTicks = encoderTicks;
//         return NAN;
//     }
//     if (dt < samplePeriod) return NAN;

//     long currentTicks = encoderTicks;
//     long delta = currentTicks - lastEncoderTicks;
//     lastEncoderTicks = currentTicks;
//     lastSampleMicros = nowMicros;

//     return delta / dt;  // ticks per second
// }


// float readFilteredVelocity() {
//     float rawVel = readActualVelocity();
//     if (isnan(rawVel)) return NAN;
//     filteredVel = (1.0f - alpha) * filteredVel + alpha * rawVel;
//     return filteredVel;
// }

// // ================== Motor Control (ESP32 LEDC) ==================
// void setMotorPWM(float value) {
//     value = constrain(value, -1023.0f, 1023.0f);
//     int duty = (int)fabs(value);
//     const int maxDuty = (1 << PWM_RES) - 1;
//     duty = constrain(duty, 0, maxDuty);

//     Serial.println( " (duty: " + String(duty) + ")");

//     if (value > 0.0f) {
//         digitalWrite(MOTOR_A1_PIN, HIGH);
//         digitalWrite(MOTOR_A2_PIN, LOW);
//         ledcWrite(PWM_CHANNEL, duty);
//     } else if (value < 0.0f) {
//         digitalWrite(MOTOR_A1_PIN, LOW);
//         digitalWrite(MOTOR_A2_PIN, HIGH);
//         ledcWrite(PWM_CHANNEL, duty);
//     } else {
//         digitalWrite(MOTOR_A1_PIN, LOW);
//         digitalWrite(MOTOR_A2_PIN, LOW);
//         ledcWrite(PWM_CHANNEL, 0);
//     }
// }

// // ================== PID Controller ==================

// int basePWM(float targetVel) {
//     const int n = 11;
//     float speeds[n] = {0, 207, 598, 983, 1457, 1822, 2289, 2788, 3040, 3373, 3830};
//     int pwms[n]      = {0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};

//     if (targetVel <= speeds[0]) return pwms[0];
//     if (targetVel >= speeds[n - 1]) return pwms[n - 1];

//     for (int i = 0; i < n - 1; i++) {
//         if (targetVel >= speeds[i] && targetVel <= speeds[i + 1]) {
//             float ratio = (targetVel - speeds[i]) / (speeds[i + 1] - speeds[i]);
//             return pwms[i] + ratio * (pwms[i + 1] - pwms[i]);
//         }
//     }
//     return pwms[n - 1];
// }


// float computePID(float desired, float actual) {
//     float dt = samplePeriod; // fixed sample period in seconds

//     float error = desired - actual;
//     integral += error * dt;

//     // === Integral clamping (anti-windup) ===
//     const float integral_min = -200.0f;
//     const float integral_max = 200.0f;
//     if (integral > integral_max) integral = integral_max;
//     else if (integral < integral_min) integral = integral_min;

//     float derivative = (error - prevError) / dt;

//     float output = tuner.Kp * error + tuner.Ki * integral + tuner.Kd * derivative;
//     prevError = error;
//     return output;
// }


// // ================== Setup ==================
// void setup() {
//     Serial.begin(115200);
//     Serial.println("Starting Micromouse PID Controller...");

//     // pins and interrupts
//     pinMode(ENCODER_A_PIN, INPUT_PULLDOWN);
//     pinMode(ENCODER_B_PIN, INPUT_PULLDOWN);
//     pinMode(MOTOR_ENA_PIN, OUTPUT);
//     pinMode(MOTOR_A1_PIN, OUTPUT);
//     pinMode(MOTOR_A2_PIN, OUTPUT);

//     attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);

//     // setup LEDC PWM for ESP32
//     ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
//     ledcAttachPin(MOTOR_ENA_PIN, PWM_CHANNEL);
//     ledcWrite(PWM_CHANNEL, 0);

//     WiFi.begin(ssid, password);
//     Serial.print("Connecting to WiFi");
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println();
//     Serial.println("WiFi connected. IP address: " + WiFi.localIP().toString());

//     // Note: WebHandler::begin will call tuner.begin(server_) internally.
//     web.begin();


//     Serial.println("System ready. Connect to web UI to start test.");
// }

// //================== Main Loop ==================
// void loop() {
//     web.handleClient();

//     if (sequenceRunning) {
//         float t = (millis() - sequenceStart) / 1000.0f;
//         float desiredVel = generateTrapezoidalVelocity(t);
//         float actualVel = readFilteredVelocity();


//         if (!isnan(actualVel)) {
//             int base = basePWM(desiredVel);        // Minimum PWM to overcome friction
//             float pidOut = computePID(desiredVel, actualVel);

//             // Ensure correct direction
//             float pwmVal = base + pidOut;
//             if (pwmVal > 1023) pwmVal = 1023;
//             if (pwmVal < -1023) pwmVal = -1023;

//             setMotorPWM(pwmVal);
//             logger.log(desiredVel, actualVel);
//         }

//         if (t >= vtest.accelTime + vtest.steadyTime + vtest.decelTime) //TEST
//             stopSequence();

//     }
// }

// // void loop() {
// //     static int duty = 0;
// //     static unsigned long lastStep = 0;
// //     const int step = 100;      // PWM increment
// //     const int settleTime = 1500; // ms between steps

// //     unsigned long now = millis();
// //     if (now - lastStep > settleTime) {
// //         setMotorPWM(duty);
// //         delay(800);  // let speed stabilize

// //         float vel = 0, v;
// //         for (int i = 0; i < 10; i++) { // average over few samples
// //             v = readFilteredVelocity();
// //             if (!isnan(v)) vel += v;
// //             delay(20);
// //         }
// //         vel /= 10.0f;

// //         Serial.printf("PWM=%d  ->  Velocity=%.1f ticks/s\n", duty, vel);

// //         duty += step;
// //         if (duty > 1023) {
// //             setMotorPWM(0);
// //             Serial.println("Calibration complete");
// //             while (1);
// //         }
// //         lastStep = now;
// //     }
// // }