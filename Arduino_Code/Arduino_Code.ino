#include "mine_DUCO_S1.h"

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  if(Serial.find("start\n")) { // Wait for start word
    unsigned char prefix[40], target[40];
    Serial.readBytesUntil('\n', prefix, 42);
    Serial.readBytesUntil('\n', target, 42);
    int diff = Serial.parseInt();
    
    unsigned long StartTime = micros(); // Start time measurement

    int result = mine_DUCO_S1(prefix, target, diff);
    
    unsigned long EndTime = micros(); // End time measurement
    unsigned long ElapsedTime = EndTime - StartTime; // Calculate elapsed time

    // Send result back to the program with share time
    Serial.print(result);
    Serial.print(',');
    Serial.println(ElapsedTime);

    // Blink built-in LED
    digitalWrite(LED_BUILTIN, HIGH);
    delay(40);
    digitalWrite(LED_BUILTIN, LOW);
  }
}
