#include <SoftwareSerial.h>
#include <GT5X.h>

/* Enroll fingerprints */

/*  pin #2 is IN from sensor (GREEN wire)
 *  pin #3 is OUT from arduino  (WHITE/YELLOW wire)
 */
SoftwareSerial fserial(2, 3);

GT5X finger(&fserial);
GT5X_DeviceInfo info;

void setup()
{
    Serial.begin(9600);
    Serial.println("ENROLL test");
    fserial.begin(9600);

    if (finger.begin(&info)) {
        Serial.println("Found fingerprint sensor!");
        Serial.print("Firmware Version: "); Serial.println(info.fwversion);
    } else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
}

void loop()
{
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
    while (Serial.read() != -1);  // clear buffer
}

bool is_enrolled(uint16_t fid) {
    uint16_t rc = finger.is_enrolled(fid);
    switch (rc) {
        case GT5X_OK:
            Serial.println("ID is used.");
            return true;
        case GT5X_NACK_IS_NOT_USED:
            Serial.println("ID unused.");
            return false;
        default:
            Serial.print("Error code: "); Serial.println(rc);
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
            Serial.print("Error code: "); Serial.println(p);
            return;
    }
    
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
                    Serial.print("Error code: "); Serial.println(p);
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
                Serial.print("Error code: "); Serial.println(p);
                return;
        }

        Serial.println("Remove finger.");
        while (finger.is_pressed()) {
            yield();
        }
        Serial.println();
    }

    Serial.print("Enroll complete.");
}