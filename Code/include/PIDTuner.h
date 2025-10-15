#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>

class PIDTuner {
public:
    float Kp = 1.0f, Ki = 0.0f, Kd = 0.0f;

    PIDTuner() {}

    void begin(WebServer& server) {
        prefs.begin("pid", false);
        loadPID();

        // Attach routes to shared WebServer
        server.on("/", [this, &server]() { handleRoot(server); });
        server.on("/set", [this, &server]() { handleSet(server); });

        Serial.println("PID tuner routes registered");
    }

private:
    Preferences prefs;

    void loadPID() {
        Kp = prefs.getFloat("kp", 1.0f);
        Ki = prefs.getFloat("ki", 0.0f);
        Kd = prefs.getFloat("kd", 0.0f);
        Serial.printf("PID: %.2f, %.2f, %.2f\n", Kp, Ki, Kd);
    }

    void savePID() {
        prefs.putFloat("kp", Kp);
        prefs.putFloat("ki", Ki);
        prefs.putFloat("kd", Kd);
        Serial.println("PID saved");
    }

    void handleRoot(WebServer& server) {
        String html = R"(
<!DOCTYPE html><html><head><title>PID Tuner</title>
<style>body{font-family:Arial;margin:20px;background:#f5f5f5}
.container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px}
input{width:100px;margin:5px;padding:5px}
button{padding:10px 20px;margin:5px;background:#007bff;color:white;border:none;cursor:pointer}
.nav{text-align:center;margin:20px 0}
.nav a{margin:10px;padding:10px 20px;background:#28a745;color:white;text-decoration:none;border-radius:5px}
</style></head><body><div class="container"><h1>PID Tuner</h1>)";
        html += "<p><b>Current:</b> Kp: " + String(Kp, 2) + " Ki: " + String(Ki, 2) + " Kd: " + String(Kd, 2) + "</p>";
        html += R"(<form action="/set" method="POST">
Kp: <input type="number" step="any" name="kp" value=")" + String(Kp, 2) + R"(" required><br>
Ki: <input type="number" step="any" name="ki" value=")" + String(Ki, 2) + R"(" required><br>
Kd: <input type="number" step="any" name="kd" value=")" + String(Kd, 2) + R"(" required><br>
<button type="submit">Update PID</button></form>
<div class="nav"><a href="/sequence">Run Sequence</a><a href="/plot">View Plot</a><a href="/upload">Upload CSV</a></div>
</div></body></html>)";
        server.send(200, "text/html", html);
    }

    void handleSet(WebServer& server) {
        if (server.hasArg("kp")) Kp = server.arg("kp").toFloat();
        if (server.hasArg("ki")) Ki = server.arg("ki").toFloat();
        if (server.hasArg("kd")) Kd = server.arg("kd").toFloat();
        savePID();
        server.sendHeader("Location", "/");
        server.send(303, "text/plain", "");
    }
};
