#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>

class VelocityTest {
public:
    float maxVel = 2300.0f;
    float accelTime = 1.0f;
    float steadyTime = 3.0f;
    float decelTime = 1.0f;

    VelocityTest() {}

    void begin(WebServer& server) {
        prefs.begin("velocity", false);
        loadParams();

        // Register routes
        server.on("/velocity", [this, &server]() { handlePage(server); });
        server.on("/set-velocity", [this, &server]() { handleSet(server); });

        Serial.println("Velocity test routes registered");
    }

    float generateProfile(float t) const {
        if (t < accelTime)
            return maxVel * (t / accelTime);
        else if (t < accelTime + steadyTime)
            return maxVel;
        else if (t < accelTime + steadyTime + decelTime)
            return maxVel * (1.0f - (t - accelTime - steadyTime) / decelTime);
        else
            return 0.0f;
    }

private:
    Preferences prefs;

    void loadParams() {
        maxVel = prefs.getFloat("maxVel", 2300.0f);
        accelTime = prefs.getFloat("accelTime", 1.0f);
        steadyTime = prefs.getFloat("steadyTime", 3.0f);
        decelTime = prefs.getFloat("decelTime", 1.0f);
        Serial.printf("Velocity Params: maxVel=%.1f accel=%.1f steady=%.1f decel=%.1f\n",
                      maxVel, accelTime, steadyTime, decelTime);
    }

    void saveParams() {
        prefs.putFloat("maxVel", maxVel);
        prefs.putFloat("accelTime", accelTime);
        prefs.putFloat("steadyTime", steadyTime);
        prefs.putFloat("decelTime", decelTime);
        Serial.println("Velocity params saved");
    }

    void handlePage(WebServer& server) {
        String html = R"rawliteral(
<!DOCTYPE html><html><head><title>Velocity Test Settings</title>
<style>
body { font-family: Arial; margin: 20px; background: #f5f5f5; }
.container { max-width: 600px; margin: auto; background: white; padding: 20px; border-radius: 10px; }
input { width: 100px; margin: 5px; padding: 5px; }
button { padding: 10px 20px; margin: 10px; background: #28a745; color: white; border: none; border-radius: 5px; cursor: pointer; }
.nav a { margin: 10px; padding: 10px 20px; background: #007bff; color: white; border-radius: 5px; text-decoration: none; }
</style>
</head><body><div class="container">
<h1>Velocity Test Config</h1>
<form action="/set-velocity" method="POST">
<label>Max Velocity (ticks/sec):</label><input type="number" step="any" name="maxVel" value=")rawliteral" + String(maxVel) + R"rawliteral("><br>
<label>Accel Time (s):</label><input type="number" step="any" name="accel" value=")rawliteral" + String(accelTime) + R"rawliteral("><br>
<label>Steady Time (s):</label><input type="number" step="any" name="steady" value=")rawliteral" + String(steadyTime) + R"rawliteral("><br>
<label>Decel Time (s):</label><input type="number" step="any" name="decel" value=")rawliteral" + String(decelTime) + R"rawliteral("><br>
<button type="submit">Save Settings</button>
</form>
<div class="nav">
<a href="/">Home</a>
<a href="/sequence">Velocity Test</a>
<a href="/plot">Plot</a>
</div>
</div></body></html>
)rawliteral";
        server.send(200, "text/html", html);
    }

    void handleSet(WebServer& server) {
        if (server.hasArg("maxVel")) maxVel = server.arg("maxVel").toFloat();
        if (server.hasArg("accel")) accelTime = server.arg("accel").toFloat();
        if (server.hasArg("steady")) steadyTime = server.arg("steady").toFloat();
        if (server.hasArg("decel")) decelTime = server.arg("decel").toFloat();
        saveParams();
        server.sendHeader("Location", "/velocity");
        server.send(303, "text/plain", "");
    }
};
