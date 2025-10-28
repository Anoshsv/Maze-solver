// #pragma once
// #include <Arduino.h>
// #include <Preferences.h>
// #include <WebServer.h>

// class PIDTuner {
// public:
//     float Kp = 1.0f, Ki = 0.0f, Kd = 0.0f;

//     PIDTuner() {}

//     void begin(WebServer& server) {
//         prefs.begin("pid", false);
//         loadPID();

//         // Attach routes to shared WebServer
//         server.on("/", [this, &server]() { handleRoot(server); });
//         server.on("/set", [this, &server]() { handleSet(server); });

//         Serial.println("PID tuner routes registered");
//     }

// private:
//     Preferences prefs;

//     void loadPID() {
//         Kp = prefs.getFloat("kp", 1.0f);
//         Ki = prefs.getFloat("ki", 0.0f);
//         Kd = prefs.getFloat("kd", 0.0f);
//         Serial.printf("PID: %.2f, %.2f, %.2f\n", Kp, Ki, Kd);
//     }

//     void savePID() {
//         prefs.putFloat("kp", Kp);
//         prefs.putFloat("ki", Ki);
//         prefs.putFloat("kd", Kd);
//         Serial.println("PID saved");
//     }

//     void handleRoot(WebServer& server) {
//         String html = R"(
// <!DOCTYPE html><html><head><title>PID Tuner</title>
// <style>body{font-family:Arial;margin:20px;background:#f5f5f5}
// .container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px}
// input{width:100px;margin:5px;padding:5px}
// button{padding:10px 20px;margin:5px;background:#007bff;color:white;border:none;cursor:pointer}
// .nav{text-align:center;margin:20px 0}
// .nav a{margin:10px;padding:10px 20px;background:#28a745;color:white;text-decoration:none;border-radius:5px}
// </style></head><body><div class="container"><h1>PID Tuner</h1>)";
//         html += "<p><b>Current:</b> Kp: " + String(Kp, 2) + " Ki: " + String(Ki, 2) + " Kd: " + String(Kd, 2) + "</p>";
//         html += R"(<form action="/set" method="POST">
// Kp: <input type="number" step="any" name="kp" value=")" + String(Kp, 2) + R"(" required><br>
// Ki: <input type="number" step="any" name="ki" value=")" + String(Ki, 2) + R"(" required><br>
// Kd: <input type="number" step="any" name="kd" value=")" + String(Kd, 2) + R"(" required><br>
// <button type="submit">Update PID</button></form>
// <div class="nav"><a href="/sequence">Run Sequence</a><a href="/plot">View Plot</a><a href="/upload">Upload CSV</a></div>
// </div></body></html>)";
//         server.send(200, "text/html", html);
//     }

//     void handleSet(WebServer& server) {
//         if (server.hasArg("kp")) Kp = server.arg("kp").toFloat();
//         if (server.hasArg("ki")) Ki = server.arg("ki").toFloat();
//         if (server.hasArg("kd")) Kd = server.arg("kd").toFloat();
//         savePID();
//         server.sendHeader("Location", "/");
//         server.send(303, "text/plain", "");
//     }
// };




// ABOVE CODE WORKED PROPERLY FOR 1 MOTOR, MODIFYING FOR 2 MOTORS AND YAW


#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>

class PIDTuner {
public:
    // --- Motor velocity PIDs ---
    float KpL = 1.0f, KiL = 0.0f, KdL = 0.0f;   // Left motor
    float KpR = 1.0f, KiR = 0.0f, KdR = 0.0f;   // Right motor

    // --- Yaw PID ---
    float KpYaw = 1.0f, KiYaw = 0.0f, KdYaw = 0.0f;

    PIDTuner() {}

    void begin(WebServer& server) {
        prefs.begin("pid", false);
        loadPID();

        // Attach web routes
        server.on("/", [this, &server]() { handleRoot(server); });
        server.on("/set", [this, &server]() { handleSet(server); });

        Serial.println("PID tuner routes registered");
    }

private:
    Preferences prefs;

    void loadPID() {
        // Left motor
        KpL = prefs.getFloat("kpL", 1.0f);
        KiL = prefs.getFloat("kiL", 0.0f);
        KdL = prefs.getFloat("kdL", 0.0f);

        // Right motor
        KpR = prefs.getFloat("kpR", 1.0f);
        KiR = prefs.getFloat("kiR", 0.0f);
        KdR = prefs.getFloat("kdR", 0.0f);

        // Yaw
        KpYaw = prefs.getFloat("kpYaw", 0.0f);
        KiYaw = prefs.getFloat("kiYaw", 0.0f);
        KdYaw = prefs.getFloat("kdYaw", 0.0f);

        Serial.printf(
            "PID loaded:\n"
            "  Left  -> Kp=%.2f Ki=%.2f Kd=%.2f\n"
            "  Right -> Kp=%.2f Ki=%.2f Kd=%.2f\n"
            "  Yaw   -> Kp=%.2f Ki=%.2f Kd=%.2f\n",
            KpL, KiL, KdL, KpR, KiR, KdR, KpYaw, KiYaw, KdYaw);
    }

    void savePID() {
        // Left
        prefs.putFloat("kpL", KpL);
        prefs.putFloat("kiL", KiL);
        prefs.putFloat("kdL", KdL);
        // Right
        prefs.putFloat("kpR", KpR);
        prefs.putFloat("kiR", KiR);
        prefs.putFloat("kdR", KdR);
        // Yaw
        prefs.putFloat("kpYaw", KpYaw);
        prefs.putFloat("kiYaw", KiYaw);
        prefs.putFloat("kdYaw", KdYaw);

        Serial.println("All PID values (L, R, Yaw) saved.");
    }

    void handleRoot(WebServer& server) {
        String html = R"(
<!DOCTYPE html><html><head><title>PID Tuner</title>
<style>
body { font-family: Arial; margin: 20px; background: #f5f5f5 }
.container { max-width: 700px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px }
input { width: 80px; margin: 5px; padding: 5px }
button { padding: 10px 20px; margin: 10px; background: #007bff; color: white; border: none; cursor: pointer }
.nav { text-align: center; margin: 20px 0 }
.nav a { margin: 10px; padding: 10px 20px; background: #28a745; color: white; text-decoration: none; border-radius: 5px }
table { border-collapse: collapse; width: 100%; }
th, td { border: 1px solid #ddd; padding: 6px; text-align: center; }
th { background-color: #f0f0f0; }
</style></head>
<body><div class="container"><h1>PID Tuner</h1>
<form action="/set" method="POST">
<table>
<tr><th>Controller</th><th>Kp</th><th>Ki</th><th>Kd</th></tr>
<tr><td>Left Motor</td>
<td><input type="number" step="any" name="kpL" value=")" + String(KpL, 2) + R"("></td>
<td><input type="number" step="any" name="kiL" value=")" + String(KiL, 2) + R"("></td>
<td><input type="number" step="any" name="kdL" value=")" + String(KdL, 2) + R"("></td></tr>

<tr><td>Right Motor</td>
<td><input type="number" step="any" name="kpR" value=")" + String(KpR, 2) + R"("></td>
<td><input type="number" step="any" name="kiR" value=")" + String(KiR, 2) + R"("></td>
<td><input type="number" step="any" name="kdR" value=")" + String(KdR, 2) + R"("></td></tr>

<tr><td>Yaw</td>
<td><input type="number" step="any" name="kpYaw" value=")" + String(KpYaw, 2) + R"("></td>
<td><input type="number" step="any" name="kiYaw" value=")" + String(KiYaw, 2) + R"("></td>
<td><input type="number" step="any" name="kdYaw" value=")" + String(KdYaw, 2) + R"("></td></tr>
</table>

<button type="submit">Update PID</button>
</form>

<div class="nav">
<a href="/sequence">Run Sequence</a>
<a href="/plot">View Plot</a>
<a href="/upload">Upload CSV</a>
</div></div></body></html>)";

        server.send(200, "text/html", html);
    }

    void handleSet(WebServer& server) {
        // Left
        if (server.hasArg("kpL")) KpL = server.arg("kpL").toFloat();
        if (server.hasArg("kiL")) KiL = server.arg("kiL").toFloat();
        if (server.hasArg("kdL")) KdL = server.arg("kdL").toFloat();
        // Right
        if (server.hasArg("kpR")) KpR = server.arg("kpR").toFloat();
        if (server.hasArg("kiR")) KiR = server.arg("kiR").toFloat();
        if (server.hasArg("kdR")) KdR = server.arg("kdR").toFloat();
        // Yaw
        if (server.hasArg("kpYaw")) KpYaw = server.arg("kpYaw").toFloat();
        if (server.hasArg("kiYaw")) KiYaw = server.arg("kiYaw").toFloat();
        if (server.hasArg("kdYaw")) KdYaw = server.arg("kdYaw").toFloat();

        savePID();
        server.sendHeader("Location", "/");
        server.send(303, "text/plain", "");
    }
};
