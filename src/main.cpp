#include <Arduino.h>
#include <ESP32Servo.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

Servo servo1, servo2, servo3, servo4; // Create servo objects

#define SERVO1_PIN 26
#define SERVO2_PIN 27
#define SERVO3_PIN 25
#define SERVO4_PIN 33

void moveServo();
void moveAllServos();
void menu();

void setup() {
    Serial.begin(115200);
    SerialBT.begin("ESP32-RobotArm"); // Start Serial Monitor
    servo3.attach(SERVO3_PIN, 600, 2400);
    servo4.attach(SERVO4_PIN, 600, 2400);
    servo2.attach(SERVO2_PIN, 600, 2400);
    servo1.attach(SERVO1_PIN, 600, 2400);
    
    menu();
}

void loop() {
    if (SerialBT.available()) {
        int input = SerialBT.parseInt();
        switch (input) {
            case 1: moveServo(); break;
            case 2: moveAllServos(); break;
            default: menu();
        }
    }
}

void moveServo() {
    // char buffer[20];
    int servoNum, angle;
    SerialBT.println("Enter command: <servo_number> <angle>");
    SerialBT.println("Example: 1 90  (Moves Servo 1 to 90°)");
    servoNum = SerialBT.parseInt();
    angle = SerialBT.parseInt();
    // SerialBT.readStringUntil('\n').toCharArray(buffer, 20);
    // sscanf(buffer, "%d %d", &servoNum, &angle);

    // Ensure angle is within servo limits
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
        


    switch (servoNum) {
        case 1: servo1.write(angle); break;
        case 2: servo2.write(angle); break;
        case 3: servo3.write(angle); break;
        case 4: servo4.write(angle); break;
        default: SerialBT.println("Invalid Servo Number!");
    }
    SerialBT.printf("Servo %d Moved %d\n", servoNum, angle);
}

void menu() {
    SerialBT.println("1. Move 1 Servo");
    SerialBT.println("2. Move All Servos");
}

void moveAllServos() {
    // char buffer[20];
    int angle1, angle2, angle3, angle4;
    SerialBT.println("Enter command: <angle1> <angle2> <angle3> <angle4>");
    SerialBT.println("Example: 90 90 90 90  (Moves all servos to 90°)");

    angle1 = SerialBT.parseInt();
    angle2 = SerialBT.parseInt();
    angle3 = SerialBT.parseInt();
    angle4 = SerialBT.parseInt();

    // SerialBT.readStringUntil('\n').toCharArray(buffer, 20);
    // sscanf(buffer, "%d %d %d %d", &angle1, &angle2, &angle3, &angle4);

    // Ensure angle is within servo limits
    if (angle1 < 0) angle1 = 0;
    if (angle1 > 180) angle1 = 180;
    if (angle2 < 0) angle2 = 0;
    if (angle2 > 180) angle2 = 180;
    if (angle3 < 0) angle3 = 0;
    if (angle3 > 180) angle3 = 180;
    if (angle4 < 0) angle4 = 0;
    if (angle4 > 180) angle4 = 180;

    servo1.write(angle1);
    servo2.write(angle2);
    servo3.write(angle3);
    servo4.write(angle4);
    SerialBT.printf("Servos Moved to %d %d %d %d\n", angle1, angle2, angle3, angle4);
}

