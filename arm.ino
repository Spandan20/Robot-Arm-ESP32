#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>


const int servo1Pin = 25;  // GPIO for Servo 1
const int servo2Pin = 26;  // GPIO for Servo 2
const int servo3Pin = 33;  // GPIO for Servo 2
const int servo4Pin = 32;  // GPIO for Servo 2

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

AsyncWebServer server(80);

bool isRecording = false;
bool isReplaying = false;
unsigned long previousMillis = 0;
const int replayInterval = 100;  // Delay between replay movements (in ms)

// Arrays to store recorded angles
const int maxSteps = 500;  // Max steps that can be recorded
int servo1Angles[maxSteps];
int servo2Angles[maxSteps];
int servo3Angles[maxSteps];
int servo4Angles[maxSteps];
int stepCount = 0;
int replayIndex = 0;

// HTML for web interface
String getHTML() {
  return R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>ESP32 Servo Control with Record & Replay</title>
      <style>
        body { text-align: center; font-family: Arial; margin-top: 50px; }
        input[type="range"] { width: 80%; }
        #angleValue1, #angleValue2, #angleValue3, #angleValue4 { font-size: 24px; }
        button { padding: 12px 24px; font-size: 18px; margin: 10px; cursor: pointer; }
        #recordBtn { background-color: red; color: white; border: none; }
        #replayBtn { background-color: green; color: white; border: none; }
      </style>
    </head>
    <body>
      <h2>ESP32 Servo Control with Record & Replay</h2>
      
      <h3>Servo 1 Control</h3>
      <input type="range" id="servoSlider1" min="0" max="180" value="90" onchange="updateServo1(this.value)">
      <p>Angle 1: <span id="angleValue1">90</span>°</p>
      
      <h3>Servo 2 Control</h3>
      <input type="range" id="servoSlider2" min="0" max="180" value="90" onchange="updateServo2(this.value)">
      <p>Angle 2: <span id="angleValue2">90</span>°</p>

      <h3>Servo 3 Control</h3>
      <input type="range" id="servoSlider3" min="0" max="180" value="90" onchange="updateServo3(this.value)">
      <p>Angle 3: <span id="angleValue3">90</span>°</p>

      <h3>Servo 4 Control</h3>
      <input type="range" id="servoSlider4" min="0" max="180" value="90" onchange="updateServo4(this.value)">
      <p>Angle 4: <span id="angleValue4">90</span>°</p>
      
      <button id="recordBtn" onclick="toggleRecord()">Record</button>
      <button id="replayBtn" onclick="replayMovements()">Replay</button>
      
      <script>
        let isRecording = false;

        function updateServo1(angle) {
          document.getElementById("angleValue1").innerText = angle;
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/servo1?angle=" + angle, true);
          xhr.send();
        }

        function updateServo2(angle) {
          document.getElementById("angleValue2").innerText = angle;
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/servo2?angle=" + angle, true);
          xhr.send();
        }

        function updateServo3(angle) {
          document.getElementById("angleValue3").innerText = angle;
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/servo3?angle=" + angle, true);
          xhr.send();
        }

        function updateServo4(angle) {
          document.getElementById("angleValue4").innerText = angle;
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/servo4?angle=" + angle, true);
          xhr.send();
        }

        function toggleRecord() {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/toggleRecord", true);
          xhr.send();
          
          let btn = document.getElementById("recordBtn");
          if (!isRecording) {
            btn.innerText = "Stop";
            btn.style.backgroundColor = "gray";
          } else {
            btn.innerText = "Record";
            btn.style.backgroundColor = "red";
          }
          isRecording = !isRecording;
        }

        function replayMovements() {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/replay", true);
          xhr.send();
        }

        function saveSequence() {
          let sequenceName = document.getElementById("sequenceName").value;
          if (sequenceName === "") {
            alert("Please enter a sequence name.");
            return;
          }
          sendRequest("/save?name=" + sequenceName);
          alert("Sequence Saved: " + sequenceName);
          loadSequenceList();
        }

        function loadSequence() {
          let sequenceName = document.getElementById("sequenceList").value;
          if (sequenceName === "") {
            alert("Please select a sequence to load.");
            return;
          }
          sendRequest("/load?name=" + sequenceName);
          alert("Sequence Loaded: " + sequenceName);
        }

        function deleteSequence() {
          let sequenceName = document.getElementById("sequenceList").value;
          if (sequenceName === "") {
            alert("Please select a sequence to delete.");
            return;
          }
          sendRequest("/delete?name=" + sequenceName);
          alert("Sequence Deleted: " + sequenceName);
          loadSequenceList();
        }

        function loadSequenceList() {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/listSequences", true);
          xhr.onload = function () {
            if (xhr.status === 200) {
              let sequences = JSON.parse(xhr.responseText);
              let sequenceList = document.getElementById("sequenceList");
              sequenceList.innerHTML = "";
              sequences.forEach((sequence) => {
                let option = document.createElement("option");
                option.value = sequence;
                option.innerText = sequence;
                sequenceList.appendChild(option);
              });
            }
          };
          xhr.send();
        }

        // Load sequence list on page load
        window.onload = loadSequenceList;
      </script>
    </body>
    </html>
  )rawliteral";
}

void smoothMove(Servo &servo, int startAngle, int targetAngle, int steps, int delayMs) {
  float stepSize = (targetAngle - startAngle) / (float)steps;
  for (int i = 0; i <= steps; i++) {
    int newAngle = startAngle + (stepSize * i);
    servo.write(newAngle);
    delay(delayMs);
  }
}


void saveSequence(String sequenceName) {
  if (stepCount == 0) {
    Serial.println("No sequence recorded to save.");
    return;
  }

  String fileName = "/" + sequenceName + ".json";
  File file = SPIFFS.open(fileName, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  // Create JSON array
  DynamicJsonDocument doc(2048);
  JsonArray servo1Arr = doc.createNestedArray("servo1");
  JsonArray servo2Arr = doc.createNestedArray("servo2");
  JsonArray servo3Arr = doc.createNestedArray("servo3");
  JsonArray servo4Arr = doc.createNestedArray("servo4");

  for (int i = 0; i < stepCount; i++) {
    servo1Arr.add(servo1Angles[i]);
    servo2Arr.add(servo2Angles[i]);
    servo3Arr.add(servo3Angles[i]);
    servo4Arr.add(servo4Angles[i]);
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to file");
  } else {
    Serial.println("Sequence saved successfully!");
  }

  file.close();
}


void loadSequence(String sequenceName) {
  String fileName = "/" + sequenceName + ".json";
  File file = SPIFFS.open(fileName, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  JsonArray servo1Arr = doc["servo1"];
  JsonArray servo2Arr = doc["servo2"];
  JsonArray servo3Arr = doc["servo3"];
  JsonArray servo4Arr = doc["servo4"];

  stepCount = servo1Arr.size();
  for (int i = 0; i < stepCount; i++) {
    servo1Angles[i] = servo1Arr[i];
    servo2Angles[i] = servo2Arr[i];
    servo3Angles[i] = servo3Arr[i];
    servo4Angles[i] = servo4Arr[i];
  }

  Serial.println("Sequence loaded successfully!");
  file.close();
}

void deleteSequence(String sequenceName) {
  String fileName = "/" + sequenceName + ".json";
  if (SPIFFS.remove(fileName)) {
    Serial.println("Sequence deleted successfully.");
  } else {
    Serial.println("Failed to delete sequence.");
  }
}

void listSequences() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file) {
    Serial.print("File: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}


void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Attach both servos
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);
  servo3.attach(servo3Pin);
  servo4.attach(servo4Pin);

  //Set initial angles
  smoothMove(servo1, 80, 90, 50, 10);
  delay(500);
  smoothMove(servo2, 110, 120, 50, 10);
  delay(500);
  smoothMove(servo3, 0, 0, 50, 10);
  delay(500);
  smoothMove(servo4, 20, 40, 10, 10);
  delay(500);


  // Connect to Wi-Fi
  WiFi.begin("SPANDAN 2.4", "Asahakol@1963");
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  // ✅ Print IP address

  // Serve web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getHTML());
  });

  // Control Servo 1
  server.on("/servo1", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("angle")) {
      String angleParam = request->getParam("angle")->value();
      int angle = angleParam.toInt();
      int currentAngle1 = servo1.read();
      smoothMove(servo1, currentAngle1, angle, 50, 10);
      // servo1.write(angle);
      Serial.printf("Servo 1 angle: %d\n", angle);
      
      // Record angle if recording is active
      if (isRecording && stepCount < maxSteps) {
        servo1Angles[stepCount] = angle;
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // Control Servo 2
  server.on("/servo2", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("angle")) {
      String angleParam = request->getParam("angle")->value();
      int angle = angleParam.toInt();
      int currentAngle2 = servo2.read();
      smoothMove(servo2, currentAngle2, angle, 50, 10);
      // servo2.write(angle);
      Serial.printf("Servo 2 angle: %d\n", angle);
      
      // Record angle if recording is active
      if (isRecording && stepCount < maxSteps) {
        servo2Angles[stepCount] = angle;
        stepCount++;
      }
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/servo3", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("angle")) {
      String angleParam = request->getParam("angle")->value();
      int angle = angleParam.toInt();
      int currentAngle = servo3.read();
      smoothMove(servo3, currentAngle, angle, 50, 10);
      // servo3.write(angle);
      Serial.printf("Servo 3 angle: %d\n", angle);
      
      // Record angle if recording is active
      if (isRecording && stepCount < maxSteps) {
        servo3Angles[stepCount] = angle;
        stepCount++;
      }
    }
    request->send(200, "text/plain", "OK");
  });

server.on("/servo4", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("angle")) {
      String angleParam = request->getParam("angle")->value();
      int angle = angleParam.toInt();
      int currentAngle = servo4.read();
      // smoothMove(servo4, currentAngle, angle, 10, 10);
      servo4.write(angle);
      Serial.printf("Servo 4 angle: %d\n", angle);
      
      // Record angle if recording is active
      if (isRecording && stepCount < maxSteps) {
        servo4Angles[stepCount] = angle;
        stepCount++;
      }
    }
    request->send(200, "text/plain", "OK");
  });
  // Toggle recording
  server.on("/toggleRecord", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isRecording) {
      stepCount = 0;
      Serial.println("Recording started...");
    } else {
      Serial.printf("Recording stopped. Total steps: %d\n", stepCount);
    }
    isRecording = !isRecording;
    request->send(200, "text/plain", "OK");
  });

  // Replay recorded movements
  server.on("/replay", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isReplaying && stepCount > 0) {
      Serial.println("Replaying recorded movements...");
      isReplaying = true;
      replayIndex = 0;
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("name")) {
    String name = request->getParam("name")->value();
    saveSequence(name);
  }
  request->send(200, "text/plain", "Sequence Saved");
});

server.on("/load", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("name")) {
    String name = request->getParam("name")->value();
    loadSequence(name);
  }
  request->send(200, "text/plain", "Sequence Loaded");
});

server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("name")) {
    String name = request->getParam("name")->value();
    deleteSequence(name);
  }
  request->send(200, "text/plain", "Sequence Deleted");
});

server.on("/listSequences", HTTP_GET, [](AsyncWebServerRequest *request) {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  String fileList = "[";
  while (file) {
    String fileName = String(file.name()).substring(1); // Remove "/"
    if (fileName.endsWith(".json")) {
      if (fileList.length() > 1) {
        fileList += ",";
      }
      fileList += "\"" + fileName.substring(0, fileName.length() - 5) + "\""; // Remove .json
    }
    file = root.openNextFile();
  }
  fileList += "]";
  request->send(200, "application/json", fileList);
});


  server.begin();
}

void loop() {
  if (isReplaying) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= replayInterval) {
      previousMillis = currentMillis;

      if (replayIndex < stepCount) {
        // Get current angles
        int currentAngle1 = servo1.read();
        int currentAngle2 = servo2.read();
        int currentAngle3 = servo3.read();
        int currentAngle4 = servo4.read();

        // Smoothly move to the recorded angles
        smoothMove(servo1, currentAngle1, servo1Angles[replayIndex], 50, 10);
        delay(1000);
        smoothMove(servo2, currentAngle2, servo2Angles[replayIndex], 50, 10);
        delay(1000);
        smoothMove(servo3, currentAngle3, servo3Angles[replayIndex], 50, 10);
        delay(1000);
        smoothMove(servo4, currentAngle4, servo4Angles[replayIndex], 10, 10);
        delay(1000);

        Serial.printf("Replaying step %d: Servo1=%d, Servo2=%d\n, Servo3=%d\n, Servo4=%d\n", 
                      replayIndex, servo1Angles[replayIndex], servo2Angles[replayIndex], servo3Angles[replayIndex], servo4Angles[replayIndex]);
        replayIndex++;
      } else {
        isReplaying = false;
        Serial.println("Replay finished.");
      }
    }
  }
}