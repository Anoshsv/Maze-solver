// #pragma once
// #include "PIDTuner.h"
// #include "VelocityLogger.h"
// #include "VelocityTest.h" 

// #include <Arduino.h>
// #include <WebServer.h>

// // External functions implemented in main.cpp
// extern void startSequence();
// extern void stopSequence();
// extern bool isSequenceRunning();

// class WebHandler {
// public:
//     WebHandler(PIDTuner* tuner, VelocityLogger* logger, VelocityTest* vtest)
//         : tuner_(tuner), logger_(logger), vtest_(vtest), server_(80) {}   //TEST


//     void begin() {
//         tuner_->begin(server_);   // Register PID routes
//         vtest_->begin(server_);   // Register Velocity Test routes   //TEST
//         setupRoutes();
//         server_.begin();
//         Serial.println("Web server started");
//     }

//     void handleClient() { server_.handleClient(); }

// private:
//     PIDTuner* tuner_;
//     VelocityLogger* logger_;
//     VelocityTest* vtest_; //TEST
//     WebServer server_;
//     String uploadedData_ = "";

//     void setupRoutes() {
//         server_.on("/sequence", [this]() { handleSequence(); });
//         server_.on("/plot", [this]() { handlePlot(); });
//         server_.on("/upload", [this]() { handleUpload(); });

//         server_.on("/start", HTTP_POST, [this]() {
//             startSequence();
//             redirect("/sequence");
//         });

//         server_.on("/stop", HTTP_POST, [this]() {
//             stopSequence();
//             redirect("/sequence");
//         });

//         server_.on("/data", [this]() { sendCSV(logger_->toCSV()); });
//         server_.on("/uploaded-data", [this]() { sendCSV(uploadedData_); });

//         server_.on("/download", [this]() {
//             server_.sendHeader("Content-Disposition",
//                 "attachment; filename=real_data.csv");
//             sendCSV(logger_->toCSV());
//         });

//         server_.on("/file-upload", HTTP_POST,
//             [this]() { redirect("/plot"); },
//             [this]() { handleFileUpload(); });
//     }

//     void redirect(const String& url) {
//         server_.sendHeader("Location", url);
//         server_.send(303, "text/plain", "");
//     }

//     void sendCSV(const String& csv) {
//         Serial.printf("Free heap: %u, CSV size: %u\n", (unsigned)ESP.getFreeHeap(), (unsigned)csv.length());
//         server_.sendHeader("Content-Length", String(csv.length()));
//         server_.send(200, "text/csv", csv);
//     }

//     void handleSequence() {
//         String status = isSequenceRunning() ? "Running" : "Stopped";
//         String html = R"rawliteral(
//     <!DOCTYPE html>
//     <html>
//     <head>
//         <title>Velocity Control</title>
//         <style>
//             body { font-family: Arial; background: #f5f5f5; margin: 20px; text-align: center; }
//             .container { max-width: 500px; margin: auto; background: white; padding: 20px; border-radius: 10px; }
//             button { padding: 15px 30px; margin: 10px; border: none; cursor: pointer; font-size: 16px; border-radius: 5px; }
//             .start { background: #28a745; color: white; }
//             .stop { background: #dc3545; color: white; }
//             .nav a { margin: 10px; padding: 10px 20px; background: #007bff; color: white; border-radius: 5px; text-decoration: none; }
//             .status { padding: 15px; margin: 10px 0; border-radius: 5px; }
//             .running { background: #d4edda; color: #155724; }
//             .stopped { background: #f8d7da; color: #721c24; }
//         </style>
//     </head>
//     <body>
//         <div class="container">
//             <h1>Velocity Control</h1>
//             <div class="status )rawliteral";

//         html += (isSequenceRunning() ? "running" : "stopped");
//         html += R"rawliteral(">
//                 <h3>Status: )rawliteral";
//         html += status;
//         html += R"rawliteral(</h3>
//             </div>

//             <div class="info">
//                 <h4>Trapezoidal Velocity Test</h4>
//                 <p>Profile: Ramp Up = )rawliteral";
//         html += String(vtest_->accelTime, 2);
//         html += R"rawliteral(s, Steady = )rawliteral";
//         html += String(vtest_->steadyTime, 2);
//         html += R"rawliteral(s, Ramp Down = )rawliteral";
//         html += String(vtest_->decelTime, 2);
//         html += R"rawliteral(s)</p>
//                 <p>Max Velocity = )rawliteral";
//         html += String(vtest_->maxVel, 1);
//         html += R"rawliteral( ticks/sec</p>
//             </div>)rawliteral";

//         if (!isSequenceRunning()) {
//             html += R"rawliteral(
//             <form method="POST" action="/start">
//                 <button class="start" type="submit">Start Test</button>
//             </form>)rawliteral";
//         } else {
//             html += R"rawliteral(
//             <form method="POST" action="/stop">
//                 <button class="stop" type="submit">Stop Test</button>
//             </form>)rawliteral";
//         }

//         html += R"rawliteral(
//             <div class="nav">
//                 <a href="/">Home</a>
//                 <a href="/plot">Plot</a>
//                 <a href="/download">Download</a>
//                 <a href="/velocity">Settings</a>
//             </div>
//         </div>
//     </body>
//     </html>
//     )rawliteral";

//         server_.send(200, "text/html", html);
//     }


//     void handlePlot() {
//         String html = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
//     <meta charset="utf-8">
//     <title>Data Plot</title>
//     <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
//     <style>
//         body { font-family: Arial; margin: 20px; background: #f5f5f5; }
//         .container { max-width: 1000px; margin: auto; background: white; padding: 20px; border-radius: 10px; }
//         .chart-wrapper { height: 400px; margin: 20px 0; }
//         .nav { text-align: center; margin: 20px 0; }
//         .nav a, .nav button { margin: 10px; padding: 10px 20px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; border: none; cursor: pointer; }
//         .nav button { background: #28a745; }
//         #message { text-align: center; margin-top: 10px; color: #666; }
//     </style>
// </head>
// <body>
//     <div class="container">
//         <h1>Performance Plot</h1>
//         <div class="chart-wrapper">
//             <canvas id="chart"></canvas>
//         </div>
//         <div id="message">Click a button to load data</div>
//         <div class="nav">
//             <a href="/">Home</a>
//             <a href="/sequence">Velocity Test</a>
//             <a href="/upload">Upload CSV</a>
//             <button onclick="loadSource('data')">Live Data</button>
//             <button onclick="loadSource('uploaded-data')">Uploaded Data</button>
//         </div>
//     </div>

//     <script>
//     let chart;

//     async function loadSource(source) {
//         document.getElementById('message').innerText = 'Loading ' + source + '...';
//         try {
//             const res = await fetch('/' + source);
//             if (!res.ok) throw new Error('HTTP ' + res.status);
//             const text = await res.text();
//             plotCSV(text, source);
//         } catch (err) {
//             console.error(err);
//             document.getElementById('message').innerText = 'Failed: ' + err.message;
//         }
//     }

//     function plotCSV(text, label) {
//         const lines = text.split(/\r?\n/);
//         let start = 0;
//         while (start < lines.length && lines[start].trim() === '') start++;
//         if (lines[start] && lines[start].toLowerCase().includes('time')) start++;

//         const time = [], desired = [], actual = [];
//         for (let i = start; i < lines.length; ++i) {
//             const line = lines[i].trim();
//             if (!line) continue;
//             const cols = line.split(',').map(s => s.trim());
//             if (cols.length < 3) continue;
//             const t = Number(cols[0]), d = Number(cols[1]), a = Number(cols[2]);
//             if (!Number.isFinite(t) || !Number.isFinite(d) || !Number.isFinite(a)) continue;
//             time.push(t);
//             desired.push(d);
//             actual.push(a);
//         }

//         if (time.length === 0) {
//             document.getElementById('message').innerText = 'No data available in ' + label;
//             return;
//         }

//         document.getElementById('message').innerText = 'Showing ' + time.length + ' samples from ' + label;

//         const ctx = document.getElementById('chart').getContext('2d');
//         if (chart) chart.destroy();

//         chart = new Chart(ctx, {
//             type: 'line',
//             data: {
//                 labels: time,
//                 datasets: [{
//                     label: 'Desired Velocity (ticks/sec)',
//                     data: desired,
//                     borderColor: '#007bff',
//                     fill: false,
//                     tension: 0.1
//                 }, {
//                     label: 'Actual Velocity (ticks/sec)',
//                     data: actual,
//                     borderColor: '#dc3545',
//                     fill: false,
//                     tension: 0.1
//                 }]
//             },
//             options: {
//                 responsive: true,
//                 plugins: {
//                     title: {
//                         display: true,
//                         text: 'Data Source: ' + label
//                     }
//                 },
//                 scales: {
//                     x: { title: { display: true, text: 'Time (ms)' } },
//                     y: { title: { display: true, text: 'Velocity (ticks/sec)' } }
//                 }
//             }
//         });
//     }
//     </script>
// </body>
// </html>
// )rawliteral";
//         server_.send(200, "text/html", html);
//     }

    
//     void handleUpload() {
//         String html = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
//     <title>Upload CSV</title>
//     <style>
//         body { font-family: Arial; margin: 20px; background: #f5f5f5; }
//         .container { max-width: 600px; margin: auto; background: white; padding: 20px; border-radius: 10px; text-align: center; }
//         .upload { border: 2px dashed #007bff; padding: 30px; margin: 20px 0; border-radius: 10px; background: #f8f9fa; }
//         .upload:hover { background: #e9ecef; border-color: #0056b3; }
//         input[type="file"] { margin: 20px 0; font-size: 16px; }
//         button { padding: 15px 30px; background: #28a745; color: white; border: none; cursor: pointer; border-radius: 5px; font-size: 16px; }
//         button:hover { background: #218838; }
//         .nav a { margin: 10px; padding: 10px 20px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; }
//     </style>
// </head>
// <body>
//     <div class="container">
//         <h1>Upload CSV Data</h1>
//         <form method="POST" action="/file-upload" enctype="multipart/form-data">
//             <div class="upload">
//                 <h3>Select CSV File</h3>
//                 <input type="file" name="data" accept=".csv" required><br><br>
//                 <button type="submit">Upload & Compare</button>
//             </div>
//         </form>
//         <div class="nav">
//             <a href="/">Home</a>
//             <a href="/plot">Plot</a>
//             <a href="/sequence">Velocity Test</a>
//         </div>
//     </div>
// </body>
// </html>
// )rawliteral";
//         server_.send(200, "text/html", html);
//     }

//     void handleFileUpload() {
//         HTTPUpload& upload = server_.upload();
//         if (upload.status == UPLOAD_FILE_START) {
//             uploadedData_ = "";
//             Serial.printf("Upload started: %s\n", upload.filename.c_str());
//         } else if (upload.status == UPLOAD_FILE_WRITE) {
//             uploadedData_.concat(String((const char*)upload.buf, upload.currentSize));
//         } else if (upload.status == UPLOAD_FILE_END) {
//             Serial.printf("Upload done: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
//         }
//     }
// };


// ABOVE CODE WORKED PROPERLY FOR 1 MOTOR, MODIFYING FOR 2 MOTORS


#pragma once
#include "PIDTuner.h"
#include "VelocityLogger.h"
#include "VelocityTest.h" 

#include <Arduino.h>
#include <WebServer.h>

// External functions implemented in main.cpp
extern void startSequence();
extern void stopSequence();
extern bool isSequenceRunning();

class WebHandler {
public:
    WebHandler(PIDTuner* tuner, VelocityLogger* logger, VelocityTest* vtest)
        : tuner_(tuner), logger_(logger), vtest_(vtest), server_(80) {}   //TEST


    void begin() {
        tuner_->begin(server_);   // Register PID routes
        vtest_->begin(server_);   // Register Velocity Test routes   //TEST
        setupRoutes();
        server_.begin();
        Serial.println("Web server started");
    }

    void handleClient() { server_.handleClient(); }

private:
    PIDTuner* tuner_;
    VelocityLogger* logger_;
    VelocityTest* vtest_; 
    WebServer server_;
    String uploadedData_ = "";

    void setupRoutes() {
        server_.on("/sequence", [this]() { handleSequence(); });
        server_.on("/plot", [this]() { handlePlot(); });
        server_.on("/upload", [this]() { handleUpload(); });

        server_.on("/start", HTTP_POST, [this]() {
            startSequence();
            redirect("/sequence");
        });

        server_.on("/stop", HTTP_POST, [this]() {
            stopSequence();
            redirect("/sequence");
        });

        server_.on("/data", [this]() { sendCSV(logger_->toCSV()); });
        server_.on("/uploaded-data", [this]() { sendCSV(uploadedData_); });

        server_.on("/download", [this]() {
            server_.sendHeader("Content-Disposition",
                "attachment; filename=real_data.csv");
            sendCSV(logger_->toCSV());
        });

        server_.on("/file-upload", HTTP_POST,
            [this]() { redirect("/plot"); },
            [this]() { handleFileUpload(); });
    }

    void redirect(const String& url) {
        server_.sendHeader("Location", url);
        server_.send(303, "text/plain", "");
    }

    void sendCSV(const String& csv) {
        Serial.printf("Free heap: %u, CSV size: %u\n", (unsigned)ESP.getFreeHeap(), (unsigned)csv.length());
        server_.sendHeader("Content-Length", String(csv.length()));
        server_.send(200, "text/csv", csv);
    }

    void handleSequence() {
        String status = isSequenceRunning() ? "Running" : "Stopped";
        String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Velocity Control</title>
        <style>
            body { font-family: Arial; background: #f5f5f5; margin: 20px; text-align: center; }
            .container { max-width: 500px; margin: auto; background: white; padding: 20px; border-radius: 10px; }
            button { padding: 15px 30px; margin: 10px; border: none; cursor: pointer; font-size: 16px; border-radius: 5px; }
            .start { background: #28a745; color: white; }
            .stop { background: #dc3545; color: white; }
            .nav a { margin: 10px; padding: 10px 20px; background: #007bff; color: white; border-radius: 5px; text-decoration: none; }
            .status { padding: 15px; margin: 10px 0; border-radius: 5px; }
            .running { background: #d4edda; color: #155724; }
            .stopped { background: #f8d7da; color: #721c24; }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>Velocity Control</h1>
            <div class="status )rawliteral";

        html += (isSequenceRunning() ? "running" : "stopped");
        html += R"rawliteral(">
                <h3>Status: )rawliteral";
        html += status;
        html += R"rawliteral(</h3>
            </div>

            <div class="info">
                <h4>Trapezoidal Velocity Test</h4>
                <p>Profile: Ramp Up = )rawliteral";
        html += String(vtest_->accelTime, 2);
        html += R"rawliteral(s, Steady = )rawliteral";
        html += String(vtest_->steadyTime, 2);
        html += R"rawliteral(s, Ramp Down = )rawliteral";
        html += String(vtest_->decelTime, 2);
        html += R"rawliteral(s)</p>
                <p>Max Velocity = )rawliteral";
        html += String(vtest_->maxVel, 1);
        html += R"rawliteral( ticks/sec</p>
            </div>)rawliteral";

        if (!isSequenceRunning()) {
            html += R"rawliteral(
            <form method="POST" action="/start">
                <button class="start" type="submit">Start Test</button>
            </form>)rawliteral";
        } else {
            html += R"rawliteral(
            <form method="POST" action="/stop">
                <button class="stop" type="submit">Stop Test</button>
            </form>)rawliteral";
        }

        html += R"rawliteral(
            <div class="nav">
                <a href="/">Home</a>
                <a href="/plot">Plot</a>
                <a href="/download">Download</a>
                <a href="/velocity">Settings</a>
            </div>
        </div>
    </body>
    </html>
    )rawliteral";

        server_.send(200, "text/html", html);
    }

    void handlePlot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Data Plot</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial; margin: 20px; background: #f5f5f5; }
        .container { max-width: 1000px; margin: auto; background: white; padding: 20px; border-radius: 10px; }
        .chart-wrapper { height: 400px; margin: 20px 0; }
        .nav { text-align: center; margin: 20px 0; }
        .nav a, .nav button { margin: 10px; padding: 10px 20px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; border: none; cursor: pointer; }
        .nav button { background: #28a745; }
        #message { text-align: center; margin-top: 10px; color: #666; }
        .checkbox-group { margin: 10px 0; text-align: center; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Performance Plot</h1>
        <div class="checkbox-group">
            <label><input type="checkbox" id="chkDesiredLeft" checked> Desired Left</label>
            <label><input type="checkbox" id="chkActualLeft" checked> Actual Left</label>
            <label><input type="checkbox" id="chkDesiredRight" checked> Desired Right</label>
            <label><input type="checkbox" id="chkActualRight" checked> Actual Right</label>
            <label><input type="checkbox" id="chkAvgDistance" checked> Avg Distance</label>
        </div>
        <div class="chart-wrapper">
            <canvas id="chart"></canvas>
        </div>
        <div id="message">Click a button to load data</div>
        <div class="nav">
            <a href="/">Home</a>
            <a href="/sequence">Velocity Test</a>
            <a href="/upload">Upload CSV</a>
            <button onclick="loadSource('data')">Live Data</button>
            <button onclick="loadSource('uploaded-data')">Uploaded Data</button>
        </div>
    </div>

    <script>
    let chart;
    let csvData = null;

    async function loadSource(source) {
        document.getElementById('message').innerText = 'Loading ' + source + '...';
        try {
            const res = await fetch('/' + source);
            if (!res.ok) throw new Error('HTTP ' + res.status);
            const text = await res.text();
            csvData = text;   // save globally
            plotCSV();        // plot after loading
        } catch (err) {
            console.error(err);
            document.getElementById('message').innerText = 'Failed: ' + err.message;
        }
    }

    function plotCSV() {
        if (!csvData) return;
        const lines = csvData.split(/\r?\n/);
        let start = 0;
        while (start < lines.length && lines[start].trim() === '') start++;
        if (lines[start] && lines[start].toLowerCase().includes('time')) start++;

        const time = [], desiredLeft = [], actualLeft = [], desiredRight = [], actualRight = [], avgDistance = [];

        for (let i = start; i < lines.length; ++i) {
            const line = lines[i].trim();
            if (!line) continue;
            const cols = line.split(',').map(s => s.trim());
            if (cols.length < 6) continue;

            const t = Number(cols[0]),
                  dL = Number(cols[1]), aL = Number(cols[2]),
                  dR = Number(cols[3]), aR = Number(cols[4]),
                  avgD = Number(cols[5]);

            if (![t,dL,aL,dR,aR,avgD].every(Number.isFinite)) continue;

            time.push(t);
            desiredLeft.push(dL);
            actualLeft.push(aL);
            desiredRight.push(dR);
            actualRight.push(aR);
            avgDistance.push(avgD);
        }

        if (time.length === 0) {
            document.getElementById('message').innerText = 'No data available';
            return;
        }

        document.getElementById('message').innerText = 'Showing ' + time.length + ' samples';

        const datasets = [];
        if (document.getElementById('chkDesiredLeft').checked)
            datasets.push({label:'Desired Left', data:desiredLeft, borderColor:'#00ffffff', fill:false, tension:0.1});
        if (document.getElementById('chkActualLeft').checked)
            datasets.push({label:'Actual Left', data:actualLeft, borderColor:'#ebc804ff', fill:false, tension:0.1});
        if (document.getElementById('chkDesiredRight').checked)
            datasets.push({label:'Desired Right', data:desiredRight, borderColor:'#0e005eff', fill:false, tension:0.1});
        if (document.getElementById('chkActualRight').checked)
            datasets.push({label:'Actual Right', data:actualRight, borderColor:'#7e1401ff', fill:false, tension:0.1});
        if (document.getElementById('chkAvgDistance').checked)
            datasets.push({label:'Avg Distance (mm)', data:avgDistance, borderColor:'#32cd32', fill:false, tension:0.1, yAxisID:'y1'});

        const ctx = document.getElementById('chart').getContext('2d');
        if (chart) chart.destroy();
        chart = new Chart(ctx, {
            type: 'line',
            data: {labels:time, datasets:datasets},
            options: {
                responsive: true,
                plugins: {
                    title: {display:true, text:'Performance Plot'},
                    tooltip: {mode:'index', intersect:false},
                    legend: {display:true}
                },
                scales: {
                    x: {title:{display:true,text:'Time (ms)'}},
                    y: {title:{display:true,text:'Velocity (ticks/sec)'}},
                    y1: {position:'right', title:{display:true,text:'Distance (mm)'}, grid:{drawOnChartArea:false}}
                }
            }
        });
    }

    // Update chart when checkboxes change
    document.querySelectorAll('.checkbox-group input[type="checkbox"]').forEach(cb => {
        cb.addEventListener('change', plotCSV);
    });

    </script>
</body>
</html>
)rawliteral";

    server_.send(200, "text/html", html);
}


    void handleUpload() {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Upload CSV</title>
    <style>
        body { font-family: Arial; margin: 20px; background: #f5f5f5; }
        .container { max-width: 600px; margin: auto; background: white; padding: 20px; border-radius: 10px; text-align: center; }
        .upload { border: 2px dashed #007bff; padding: 30px; margin: 20px 0; border-radius: 10px; background: #f8f9fa; }
        .upload:hover { background: #e9ecef; border-color: #0056b3; }
        input[type="file"] { margin: 20px 0; font-size: 16px; }
        button { padding: 15px 30px; background: #28a745; color: white; border: none; cursor: pointer; border-radius: 5px; font-size: 16px; }
        button:hover { background: #218838; }
        .nav a { margin: 10px; padding: 10px 20px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Upload CSV Data</h1>
        <form method="POST" action="/file-upload" enctype="multipart/form-data">
            <div class="upload">
                <h3>Select CSV File</h3>
                <input type="file" name="data" accept=".csv" required><br><br>
                <button type="submit">Upload & Compare</button>
            </div>
        </form>
        <div class="nav">
            <a href="/">Home</a>
            <a href="/plot">Plot</a>
            <a href="/sequence">Velocity Test</a>
        </div>
    </div>
</body>
</html>
)rawliteral";
        server_.send(200, "text/html", html);
    }

    void handleFileUpload() {
        HTTPUpload& upload = server_.upload();
        if (upload.status == UPLOAD_FILE_START) {
            uploadedData_ = "";
            Serial.printf("Upload started: %s\n", upload.filename.c_str());
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            uploadedData_.concat(String((const char*)upload.buf, upload.currentSize));
        } else if (upload.status == UPLOAD_FILE_END) {
            Serial.printf("Upload done: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
        }
    }
};