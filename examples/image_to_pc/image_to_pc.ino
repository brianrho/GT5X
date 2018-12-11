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
    
    while (!finger.is_pressed())
        delay(10);
    
    uint16_t rc = finger.get_image();  
    if (rc != GT5X_OK)
        return;

    /* header to indicate start of image stream to PC */
    Serial.println("Sending image...");
    Serial.write('\t');
    
    bool ret = finger.read_raw(GT5X_OUTPUT_TO_STREAM, &Serial, GT5X_IMAGESZ);
    
    if (!ret) {
        Serial.println("\r\nImage read failed!");
        return;
    }

    Serial.println();
    Serial.print(GT5X_IMAGESZ); Serial.println(" bytes read.");
    Serial.println("Image stream complete.");
}
