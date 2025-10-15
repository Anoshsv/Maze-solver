#include <Arduino.h>

// ================== Configurable Parameters ==================
const float WHEEL_DIAMETER_MM = 44.0;
const float WHEEL_CIRCUMFERENCE_MM = 138.16; //PI * WHEEL_DIAMETER_MM;
const int TICKS_PER_REV = 665;  // Measure & update this

// ================== Motor Class Definition ==================
class Motor {
public:
  const char* name;
  uint8_t pwmPin;
  uint8_t dir1Pin;
  uint8_t dir2Pin;
  uint8_t encAPin;
  uint8_t encBPin;
  int pwmChannel;

  volatile long encoderTicks = 0;

  Motor(const char* motorName,
        uint8_t pwm, uint8_t d1, uint8_t d2,
        uint8_t encA, uint8_t encB,
        int channel)
      : name(motorName),
        pwmPin(pwm),
        dir1Pin(d1),
        dir2Pin(d2),
        encAPin(encA),
        encBPin(encB),
        pwmChannel(channel) {}

  void begin() {
    pinMode(dir1Pin, OUTPUT);
    pinMode(dir2Pin, OUTPUT);
    pinMode(encAPin, INPUT_PULLDOWN);
    pinMode(encBPin, INPUT_PULLDOWN);

    ledcSetup(pwmChannel, 20000, 8); // 20 kHz PWM, 8-bit resolution
    ledcAttachPin(pwmPin, pwmChannel);

    // Attach interrupt for encoder
    attachInterruptArg(digitalPinToInterrupt(encAPin), isrHandler, this, CHANGE);

    Serial.printf("[%s] Initialized. Encoder pins: A=%d, B=%d\n", name, encAPin, encBPin);
  }

  void setSpeed(int speed) {
    // speed: -255 to 255
    if (speed > 0) {
      digitalWrite(dir1Pin, HIGH);
      digitalWrite(dir2Pin, LOW);
      ledcWrite(pwmChannel, speed);
    } else if (speed < 0) {
      digitalWrite(dir1Pin, LOW);
      digitalWrite(dir2Pin, HIGH);
      ledcWrite(pwmChannel, -speed);
    } else {
      // Stop
      digitalWrite(dir1Pin, LOW);
      digitalWrite(dir2Pin, LOW);
      ledcWrite(pwmChannel, 0);
    }
  }

  float getRevolutions() {
    return (float)encoderTicks / TICKS_PER_REV;
  }

  float getDistanceMM() {
    return getRevolutions() * WHEEL_CIRCUMFERENCE_MM;
  }

  void printStatus() {
    Serial.printf("[%s] Ticks: %ld | Rev: %.3f | Dist: %.2f mm\n",
                  name, encoderTicks, getRevolutions(), getDistanceMM());
  }

private:
  static void IRAM_ATTR isrHandler(void* arg) {
    Motor* m = static_cast<Motor*>(arg);
    bool a = digitalRead(m->encAPin);
    bool b = digitalRead(m->encBPin);
    if (a == b)
      m->encoderTicks++;
    else
      m->encoderTicks--;
  }
};
// ================== Motor Pin Assignments ==================
// Motor A
#define M1_PWM 21
#define M1_IN1 18
#define M1_IN2 19
#define M1_ENC_A 22
#define M1_ENC_B 23

// Motor B
#define M2_PWM 25
#define M2_IN1 26
#define M2_IN2 27
#define M2_ENC_A 32
#define M2_ENC_B 33

// ================== Create Motor Objects ==================
Motor motorLeft("Left Motor", M1_PWM, M1_IN1, M1_IN2, M1_ENC_A, M1_ENC_B, 0);
Motor motorRight("Right Motor", M2_PWM, M2_IN1, M2_IN2, M2_ENC_A, M2_ENC_B, 1);

// ================== Arduino Setup ==================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Dual Motor Encoder Test (ESP32 + L293D) ===");

  motorLeft.begin();
  motorRight.begin();

  Serial.println("Rotate the wheels manually or run motors to test encoders...");
  Serial.println("Displays: Ticks | Revolutions | Distance (mm)");
  Serial.println("==============================================");

  // Optional: start motors for testing
  motorLeft.setSpeed(220);
  motorRight.setSpeed(220);
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    motorLeft.printStatus();
    motorRight.printStatus();
    Serial.println("----------------------------------------------");
    lastPrint = millis();
  }
}
