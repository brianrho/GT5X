#include <SoftwareSerial.h>
#include <GT5X.h>

/* Send fingerprint images to pc */

/*  pin #2 is IN from sensor
 *  pin #3 is OUT from arduino (3.3V I/O!)
 */
SoftwareSerial fserial(2, 3);

GT5X finger(&fserial);
GT5X_DeviceInfo info;

void setup()
{
    Serial.begin(9600);
    Serial.println("GET IMAGE test");
    fserial.begin(9600);

    if (finger.begin(&info)) {
        Serial.println("Found fingerprint sensor!");
        Serial.print("Firmware Version: "); Serial.println(info.fwversion, HEX);
    } else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
}

void loop() {
    stream_image();
    while (1) yield();
}

void stream_image(void) {
    Serial.println("Place your finger.");

    /* turn on led for print capture */
    finger.set_led(true);

    uint16_t rc = finger.capture_finger();
    while (rc != GT5X_OK) {
        rc = finger.capture_finger();
        switch (rc) {
            case GT5X_OK:
                Serial.println("Image taken.");
                break;
            case GT5X_NACK_FINGER_IS_NOT_PRESSED:
                Serial.println(".");
                delay(10);
                break;
            default:
                Serial.print("Error code: 0x"); Serial.println(rc, HEX);
                return;
        }
    }
            
    rc = finger.get_image();  
    if (rc != GT5X_OK)
        return;
    
    /* header to indicate start of image stream to PC */
    Serial.println("Remove finger. \r\nSending image...");
    Serial.write('\t');
    
    bool ret = finger.read_raw(GT5X_OUTPUT_TO_STREAM, &Serial, GT5X_IMAGESZ);
    
    if (!ret) {
        Serial.println("\r\nImage read failed!");
        return;
    }

    Serial.println();
    Serial.print(GT5X_IMAGESZ); Serial.println(" bytes read.");
    Serial.println("Image stream complete.");

    /* turn it off */
    finger.set_led(false);
}