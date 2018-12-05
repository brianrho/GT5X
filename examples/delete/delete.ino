#include <SoftwareSerial.h>
#include <GT5X.h>

/* Delete fingerprints */

/*  pin #2 is IN from sensor
 *  pin #3 is OUT from arduino (3.3V I/O!)
 */
SoftwareSerial fserial(2, 3);

GT5X finger(&fserial);
GT5X_DeviceInfo info;

void setup()
{
    Serial.begin(9600);
    Serial.println("DELETE test");
    fserial.begin(9600);

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
    
    Serial.println("Enter the finger ID # you want to delete...");
    uint16_t fid = 0;
    while (true) {
        while (! Serial.available()) yield();
        char c = Serial.read();
        if (! isdigit(c)) break;
        fid *= 10;
        fid += c - '0';
        yield();
    }
    
    delete_finger(fid);
    Serial.println();
}

void delete_finger(uint16_t fid) {    
    uint16_t rc = finger.delete_id(fid);
    switch (rc) {
        case GT5X_OK:
            Serial.print("ID "); Serial.print(fid); 
            Serial.println(" deleted.");
            break;
        case GT5X_NACK_INVALID_POS:
            Serial.println("ID not used!");
            break;
        case GT5X_NACK_DB_IS_EMPTY:
            Serial.println("Database is empty!");
            break;
        default:
            Serial.print("Error code: 0x"); Serial.println(rc, HEX);
            break;
    }
}