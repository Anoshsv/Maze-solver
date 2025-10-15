// #include <Arduino.h>

// // ================== Config Pins ==================
// #define ENCODER_A_PIN 22
// #define ENCODER_B_PIN 23

// // ================== Wheel Parameters ==================
// const float WHEEL_DIAMETER_MM = 44.0;
// const float WHEEL_CIRCUMFERENCE_MM =138.16; // PI * WHEEL_DIAMETER_MM;

// // You’ll update this after measuring ticks per revolution
// const int TICKS_PER_REV = 665;  // Example: change this after measurement

// volatile long encoderTicks = 0;

// // ================== Encoder ISR ==================
// IRAM_ATTR void encoderISR() {
//   bool a = digitalRead(ENCODER_A_PIN);
//   bool b = digitalRead(ENCODER_B_PIN);
//   if (a == b)
//     encoderTicks++;
//   else
//     encoderTicks--;
// }

// void setup() {
//   Serial.begin(115200);
//   Serial.println("=== Encoder + Distance Test ===");

//   pinMode(ENCODER_A_PIN, INPUT_PULLDOWN);
//   pinMode(ENCODER_B_PIN, INPUT_PULLDOWN);

//   attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);

//   Serial.println("Rotate the wheel by hand...");
//   Serial.println("Displays: Ticks | Revolutions | Distance (mm)");
//   Serial.println("==============================================");
// }

// void loop() {
//   static long lastTicks = 0;
//   static unsigned long lastPrint = 0;

//   if (encoderTicks != lastTicks || millis() - lastPrint > 1000) {
//     long ticks = encoderTicks;
//     float revolutions = (float)ticks / TICKS_PER_REV;
//     float distanceMM = revolutions * WHEEL_CIRCUMFERENCE_MM;

//     Serial.printf("Ticks: %ld | Rev: %.3f | Distance: %.2f mm\n",
//                   ticks, revolutions, distanceMM);

//     lastTicks = ticks;
//     lastPrint = millis();
//   }
// }
