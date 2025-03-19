#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>

const int servo1Pin = 25;  // GPIO for Servo 1
const int servo2Pin = 26;  // GPIO for Servo 2

Servo servo1;
Servo servo2;

AsyncWebServer server(80);

bool isRecording = false;
bool isReplaying = false;
unsigned long previousMillis = 0;
const int replayInterval = 1000;  // Delay between replay movements (in ms)

// Arrays to store recorded angles
const int maxSteps = 500;  // Max steps that can be recorded
int servo1Angles[maxSteps];
int servo2Angles[maxSteps];
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
        #angleValue1, #angleValue2 { font-size: 24px; }
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


void setup() {
  Serial.begin(115200);

  // Attach both servos
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);

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

        // Smoothly move to the recorded angles
        smoothMove(servo1, currentAngle1, servo1Angles[replayIndex], 50, 10);
        smoothMove(servo2, currentAngle2, servo2Angles[replayIndex], 50, 10);

        Serial.printf("Replaying step %d: Servo1=%d, Servo2=%d\n", 
                      replayIndex, servo1Angles[replayIndex], servo2Angles[replayIndex]);
        replayIndex++;
      } else {
        isReplaying = false;
        Serial.println("Replay finished.");
      }
    }
  }
}
