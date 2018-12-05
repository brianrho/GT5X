#include <SoftwareSerial.h>
#include <GT5X.h>

/* Search the fingerprint database for a print */

/*  pin #2 is IN from sensor
 *  pin #3 is OUT from arduino (3.3V I/O!)
 */
SoftwareSerial fserial(2, 3);

GT5X finger(&fserial);
GT5X_DeviceInfo info;

void setup()
{
    Serial.begin(9600);
    Serial.println("1:N MATCH test");
    fserial.begin(9600);

    if (finger.begin(&info)) {
        Serial.println("Found fingerprint sensor!");
        Serial.print("Firmware Version: "); Serial.println(info.fwversion);
    } else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
    
    Serial.println("Place a finger to search.");

    /* turn on led for print capture */
    finger.set_led(true);
}


void loop()
{
    if (!finger.is_pressed())
        return;

    uint16_t rc = finger.capture_finger();  
    if (rc != GT5X_OK)
        return;
    
    uint16_t fid;
    rc = finger.search_database(&fid);
    if (rc != GT5X_OK) {
        Serial.println("Print not found!");
        return;
    }
    
    Serial.print("Print at ID "); Serial.println(fid);
}