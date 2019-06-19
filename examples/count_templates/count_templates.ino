#if defined(ARDUINO_ARCH_ESP32)
    #include <HardwareSerial.h>
#elif
    #include <SoftwareSerial.h>
#endif

#include <GT5X.h>

/* Count templates */

#if defined(ARDUINO_ARCH_ESP32)
    /* select UART1 */
    HardwareSerial fserial(1);
#elif
    /* RX = 2, TX = 3 */
    SoftwareSerial fserial(2, 3);
#endif

GT5X finger(&fserial);
GT5X_DeviceInfo info;

void setup() {
    Serial.begin(9600);
    Serial.println("COUNT TEMPLATES test");

    #if defined(ARDUINO_ARCH_ESP32)
        /* RX = IO16, TX = IO17 */
        fserial.begin(9600, SERIAL_8N1, 16, 17);
    #elif
        fserial.begin(9600);
    #endif

    if (finger.begin(&info)) {
        Serial.println("Found fingerprint sensor!");
        Serial.print("Firmware Version: "); Serial.println(info.fwversion, HEX);
    } else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }

    while (Serial.read() != -1);
    Serial.println("Send any character to start.\n");
    while (Serial.available() == 0) yield();
}

void loop() {
    uint16_t count;
    uint16_t rc = finger.get_enrolled_count(&count);
    if (rc != GT5X_OK) {
        Serial.println("Could not get count!");
        return;
    }
    
    Serial.print(count); Serial.println(" templates in database.\r\n");
    while (1) yield();
}
