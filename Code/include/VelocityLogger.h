// #pragma once
// #include <Arduino.h>

// // ================== Log entry & VelocityLogger ==================
// struct LogEntry {
//     uint32_t time_ms;
//     float desired;
//     float actual;
// };

// class VelocityLogger {
// public:
//     static const size_t MAX_SAMPLES = 256;

//     void start() {
//         logCount = 0;
//         startTime = millis();
//         isRunning = true;
//     }

//     void stop() { isRunning = false; }

//     void log(float desired, float actual) {
//         if (!isRunning || logCount >= MAX_SAMPLES) return;
//         uint32_t now = millis() - startTime;
//         buffer[logCount++] = { now, desired, actual };
//     }

//     String toCSV() const {
//         String csv = "time_ms,desired,actual\n";
//         for (size_t i = 0; i < logCount; i++) {
//             csv += String(buffer[i].time_ms) + "," +
//                    String(buffer[i].desired, 3) + "," +
//                    String(buffer[i].actual, 3) + "\n";
//         }
//         return csv;
//     }

//     size_t size() const { return logCount; }

// private:
//     LogEntry buffer[MAX_SAMPLES];
//     size_t logCount = 0;
//     uint32_t startTime = 0;
//     bool isRunning = false;
// };

// ABOVE CODE WORKED PROPERLY FOR 1 MOTOR, MODIFYING FOR 2 MOTORS



#pragma once
#include <Arduino.h>

// ================== Log entry & VelocityLogger ==================
struct LogEntry {
    uint32_t time_ms;
    float desired_left;
    float actual_left;
    float desired_right;
    float actual_right;
    float avg_distance;
};

class VelocityLogger {
public:
    static const size_t MAX_SAMPLES = 256;

    void start() {
        logCount = 0;
        startTime = millis();
        isRunning = true;
    }

    void stop() { isRunning = false; }

    void log(float desired_left, float actual_left,float desired_right, float actual_right,float avg_distance) {
        if (!isRunning || logCount >= MAX_SAMPLES) return;
        uint32_t now = millis() - startTime;
        buffer[logCount++] = { now, desired_left, actual_left,desired_right, actual_right, avg_distance };
    }

    String toCSV() const {
        String csv = "time_ms,desired_left,actual_left,desired_right,actual_right,avg_distance\n";
        for (size_t i = 0; i < logCount; i++) {
            csv += String(buffer[i].time_ms) + "," +
                   String(buffer[i].desired_left, 3) + "," +
                   String(buffer[i].actual_left, 3) + "," +
                   String(buffer[i].desired_right, 3) + "," +
                   String(buffer[i].actual_right, 3) + "," +
                   String(buffer[i].avg_distance, 3) + "\n";
        }
        return csv;
    }

    size_t size() const { return logCount; }

private:
    LogEntry buffer[MAX_SAMPLES];
    size_t logCount = 0;
    uint32_t startTime = 0;
    bool isRunning = false;
};
