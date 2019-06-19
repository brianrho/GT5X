#include <SoftwareSerial.h>
#include <GT5X.h>

/* Get and print fingerprint templates */

/*  pin #2 is IN from sensor
 *  pin #3 is OUT from arduino (3.3V I/O!)
 */
SoftwareSerial fserial(2, 3);

GT5X finger(&fserial);
GT5X_DeviceInfo info;

void setup()
{
    Serial.begin(9600);
    Serial.println("GET TEMPLATE test");
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
    
    Serial.println("Enter a finger ID # to get the template...");
    uint16_t fid = 0;
    while (true) {
        while (! Serial.available()) yield();
        char c = Serial.read();
        if (! isdigit(c)) break;
        fid *= 10;
        fid += c - '0';
        yield();
    }
    
    get_template(fid);
    Serial.println();
}

uint8_t template_buf[GT5X_TEMPLATESZ];

void get_template(uint16_t fid) {    
    uint16_t rc = finger.get_template(fid);
    switch (rc) {
        case GT5X_OK:
            break;
        case GT5X_NACK_INVALID_POS:
        case GT5X_NACK_IS_NOT_USED:
        case GT5X_NACK_DB_IS_EMPTY:
            Serial.println("ID is unused!");
            return;
        default:
            Serial.print("Error code: 0x"); Serial.println(rc, HEX);
            return;
    }
    
    Serial.print("Getting template for ID "); Serial.println(fid); 
    Serial.println("\r\n------------------------------------------------");
    bool ret = finger.read_raw(GT5X_OUTPUT_TO_BUFFER, template_buf, GT5X_TEMPLATESZ);
    
    if (!ret) {
        Serial.println("Template read failed!");
        return;
    }
    
    /* just for pretty-printing */
    uint8_t num_rows = GT5X_TEMPLATESZ / 16;
    uint8_t num_cols = 16;
    
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            Serial.print(template_buf[row * num_cols + col], HEX);
            Serial.print(" ");
        }
        Serial.println();
        yield();
    }
    
    for (uint8_t remn = 0; remn < GT5X_TEMPLATESZ % (num_rows * num_cols); remn++) {
        Serial.print(template_buf[num_rows * num_cols + remn], HEX);
        Serial.print(" ");
    }
    
    Serial.println("\r\n------------------------------------------------");
    Serial.print(GT5X_TEMPLATESZ); Serial.println(" bytes read.");
}