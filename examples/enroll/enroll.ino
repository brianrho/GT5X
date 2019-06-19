/* In case you have an ESP32 */
#if defined(ARDUINO_ARCH_ESP32)
    #include <HardwareSerial.h>
#elif
    #include <SoftwareSerial.h>
#endif

#include <GT5X.h>

/* Enroll fingerprints */

#if defined(ARDUINO_ARCH_ESP32)
    /* select UART1 for ESP32 */
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
    Serial.println("ENROLL test");

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
}

void loop()
{
    while (Serial.read() != -1);  // clear buffer
    
    Serial.println("Enter the finger ID # you want to enroll...");
    uint16_t fid = 0;
    while (true) {
        while (! Serial.available()) yield();
        char c = Serial.read();
        if (! isdigit(c)) break;
        fid *= 10;
        fid += c - '0';
        yield();
    }
    
    if (is_enrolled(fid))
        return;
    
    enroll_finger(fid);
    Serial.println();
}

bool is_enrolled(uint16_t fid) {
    uint16_t rc = finger.is_enrolled(fid);
    switch (rc) {
        case GT5X_OK:
            Serial.println("ID is used!");
            return true;
        case GT5X_NACK_IS_NOT_USED:
            Serial.print("ID "); Serial.print(fid);
            Serial.println(" is free.");
            return false;
        default:
            Serial.print("Error code: 0x"); Serial.println(rc, HEX);
            return true;
    }
}

void enroll_finger(uint16_t fid) {    
    uint16_t p = finger.start_enroll(fid);
    switch (p) {
        case GT5X_OK:
            Serial.print("Enrolling ID #"); Serial.println(fid);
            break;
        default:
            Serial.print("Error code: 0x"); Serial.println(p, HEX);
            return;
    }

    /* turn on led for print capture */
    finger.set_led(true);

    /* scan finger 3 times */
    for (int scan = 1; scan < 4; scan++) {
        Serial.println("Place your finger.");
        p = finger.capture_finger(true);
        
        while (p != GT5X_OK) {
            p = finger.capture_finger(true);
            switch (p) {
                case GT5X_OK:
                    Serial.println("Image taken.");
                    break;
                case GT5X_NACK_FINGER_IS_NOT_PRESSED:
                    Serial.println(".");
                    break;
                default:
                    Serial.print("Error code: 0x"); Serial.println(p, HEX);
                    return;
            }
            yield();
        }

        p = finger.enroll_scan(scan);
        switch (p) {
            case GT5X_OK:
                Serial.print("Scan "); Serial.print(scan);
                Serial.println(" complete.");
                break;
            default:
                Serial.print("Error code: 0x"); Serial.println(p, HEX);
                return;
        }

        Serial.println("Remove finger.");
        while (finger.is_pressed()) {
            yield();
        }
        Serial.println();
    }

    /* wr're done so turn it off */
    finger.set_led(false);
    
    Serial.println("Enroll complete.");
}
