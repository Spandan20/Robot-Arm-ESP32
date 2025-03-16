#include <Arduino.h>
#include <ESP32Servo.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

Servo servo1, servo2, servo3, servo4; // Create servo objects

#define SERVO1_PIN 18
#define SERVO2_PIN 19
#define SERVO3_PIN 21
#define SERVO4_PIN 22

void setup() {
    Serial.begin(115200);
    SerialBT.begin("ESP32-RobotArm"); // Start Serial Monitor
    servo3.attach(SERVO3_PIN, 600, 2400);
    servo4.attach(SERVO4_PIN, 600, 2400);
    servo2.attach(SERVO2_PIN, 600, 2400);
    servo1.attach(SERVO1_PIN, 600, 2400);
    
    SerialBT.println("Enter command: <servo_number> <angle>");
    SerialBT.println("Example: 1 90  (Moves Servo 1 to 90°)");
}

void loop() {
    if (SerialBT.available()) {
        char buffer[20];
        int servoNum, angle;
        SerialBT.readStringUntil('\n').toCharArray(buffer, 20);
        sscanf(buffer, "%d %d", &servoNum, &angle);

        // Ensure angle is within servo limits
        if (angle < 0) angle = 0;
        if (angle > 180) angle = 180;

        switch (servoNum) {
          case 1: 
          servo1.write(angle); 
          SerialBT.printf("Servo 1 Moved %d\n",angle); 
          break;
      case 2: 
          servo2.write(angle); 
          SerialBT.printf("Servo 2 Moved %d\n",angle); 
          break;
      case 3: 
          servo3.write(angle); 
          SerialBT.printf("Servo 3 Moved %d\n",angle); 
          break; 
      case 4: 
          servo4.write(angle);  
          SerialBT.printf("Servo 4 Moved %d\n",angle); 
          break;
      default: 
          SerialBT.println("Invalid Servo Number!");
        }
    }
}

