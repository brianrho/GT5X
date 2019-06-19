#include <SoftwareSerial.h>
#include <GT5X.h>

/* Empty database */

/*  pin #2 is IN from sensor
 *  pin #3 is OUT from arduino (3.3V I/O!)
 */
SoftwareSerial fserial(2, 3);

GT5X finger(&fserial);
GT5X_DeviceInfo info;

void setup()
{
    Serial.begin(9600);
    Serial.println("EMPTY test");
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
    Serial.println("Send any character to empty the database...");
    while (Serial.available() == 0) yield();
    empty_database();
    while (Serial.read() != -1);
}

void empty_database(void) {    
    uint16_t rc = finger.empty_database();
    switch (rc) {
        case GT5X_OK:
            Serial.println("Database empty.");
            break;
        case GT5X_NACK_DB_IS_EMPTY:
            Serial.println("Database is already empty!");
            break;
        default:
            Serial.print("Error code: 0x"); Serial.println(rc, HEX);
            break;
    }
    
    Serial.println();
}
