#if defined(ARDUINO_ARCH_ESP32)
    #include <HardwareSerial.h>
#elif
    #include <SoftwareSerial.h>
#endif

#include <GT5X.h>

/* 1:1 matching */

#if defined(ARDUINO_ARCH_ESP32)
    /* select UART1 */
    HardwareSerial fserial(1);
#elif
    /* RX = 2, TX = 3 */
    SoftwareSerial fserial(2, 3);
#endif

GT5X finger(&fserial);
GT5X_DeviceInfo info;

void setup()
{
    Serial.begin(9600);
    Serial.println("1:1 MATCH test");

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

    /* turn on led for print capture */
    finger.set_led(true);
}

void loop()
{
    while (Serial.read() != -1);  // clear buffer
    
    Serial.println("Enter the finger ID # you want to verify...");
    uint16_t fid = 0;
    while (true) {
        while (! Serial.available()) yield();
        char c = Serial.read();
        if (! isdigit(c)) break;
        fid *= 10;
        fid += c - '0';
        yield();
    }
    
    Serial.println("Place the finger to be verified.");
    
    verify_finger(fid);
    Serial.println();
}

void verify_finger(uint16_t fid) {    
    while (!finger.is_pressed())
        yield();

    uint16_t rc = finger.capture_finger();  
    if (rc != GT5X_OK)
        return;
    
    rc = finger.verify_finger_with_template(fid);
    if (rc != GT5X_OK) {
        Serial.println("1:1 match failed!");
        return;
    }
    
    Serial.print("1:1 match success @ ID "); Serial.println(fid);
}
