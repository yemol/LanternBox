#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println();
    Serial.println("[FT02] Boot");
    Serial.println("[FT02] ESP32-S3 online");
}

void loop() {
    static unsigned long heartbeat = 0;

    Serial.printf("[FT02] Heartbeat %lu\n", heartbeat++);
    delay(1000);
}