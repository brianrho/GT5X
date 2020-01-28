#include <SoftwareSerial.h>
#include <GT5X.h>

/* Given 2 IDs #X and #Y, this example performs 3 tasks:
 * - Extract the template at ID #X (if one exists) to a buffer
 * - Delete the template at ID #X
 * - Finally, transfer the template from the buffer to ID #Y
 */

/*  pin #2 is IN from sensor
 *  pin #3 is OUT from arduino (3.3V I/O!)
 */
SoftwareSerial fserial(2, 3);

GT5X finger(&fserial);
GT5X_DeviceInfo ginfo;

void setup()
{
    Serial.begin(9600);
    Serial.println("MOVE TEMPLATE test");
    fserial.begin(9600);

    if (finger.begin(&ginfo)) {
        Serial.println("Found fingerprint sensor!");
        Serial.print("Firmware Version: "); Serial.println(ginfo.fwversion, HEX);
    } else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }
}

uint8_t template_buf[GT5X_TEMPLATESZ];

void loop()
{
    Serial.println();
    Serial.println("Enter the source template ID.");
    uint16_t src_fid = get_input();
    
    Serial.println();
    Serial.println("Enter the destination template ID. ");
    uint16_t dest_fid = get_input();
    
    Serial.print("ID "); Serial.print(src_fid); 
    Serial.print(" ===> ID "); Serial.println(dest_fid);
    
    if (get_template(src_fid, template_buf, GT5X_TEMPLATESZ) && delete_finger(src_fid)) {
        set_template(dest_fid, template_buf, GT5X_TEMPLATESZ);
    }
}

uint16_t get_input(void) {
    uint16_t val = 0;
    
    while (Serial.read() != -1);  // clear buffer
    while (true) {
        while (! Serial.available()) yield();
        char c = Serial.read();
        if (! isdigit(c)) break;
        val *= 10;
        val += c - '0';
        yield();
    }
    
    return val;
}

bool get_template(uint16_t fid, uint8_t * buffer, uint16_t to_read) {    
    uint16_t rc = finger.get_template(fid);
    switch (rc) {
        case GT5X_OK:
            break;
        case GT5X_NACK_INVALID_POS:
        case GT5X_NACK_IS_NOT_USED:
        case GT5X_NACK_DB_IS_EMPTY:
            Serial.println("ID is unused!");
            return false;
        default:
            Serial.print("Error code: 0x"); Serial.println(rc, HEX);
            return false;
    }
    
    Serial.print("Getting template for ID "); Serial.println(fid); 
    Serial.println("\r\n------------------------------------------------");
    bool ret = finger.read_raw(GT5X_OUTPUT_TO_BUFFER, buffer, to_read);
    
    if (!ret) {
        Serial.println("Template read failed!");
        return false;
    }
    
    /* just for pretty-printing */
    uint8_t num_rows = to_read / 16;
    uint8_t num_cols = 16;
    
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            Serial.print(buffer[row * num_cols + col], HEX);
            Serial.print(" ");
        }
        Serial.println();
        yield();
    }
    
    for (uint8_t remn = 0; remn < to_read % (num_rows * num_cols); remn++) {
        Serial.print(buffer[num_rows * num_cols + remn], HEX);
        Serial.print(" ");
    }
    
    Serial.println("\r\n------------------------------------------------");
    Serial.print(GT5X_TEMPLATESZ); Serial.println(" bytes read.");
    
    return true;
}

bool set_template(uint16_t fid, uint8_t * buffer, uint16_t to_write) {
    /* check for any duplicates, by default */
    uint16_t rc = finger.set_template(fid);
    switch (rc) {
        case GT5X_OK:
            break;
        case GT5X_NACK_INVALID_POS:
            Serial.println("ID is invalid!");
            return false;
        default:
            Serial.print("Error code: 0x"); Serial.println(rc, HEX);
            return false;
    }
    
    Serial.print("Transferring template to ID "); Serial.println(fid); 
    
    /* now upload the template to the sensor, expect a response */
    rc = finger.write_raw(buffer, to_write, true);
    
    switch (rc) {
        case GT5X_OK:
            Serial.println("Transfer complete."); 
            return true;
        case GT5X_NACK_COMM_ERR:
        case GT5X_NACK_DEV_ERR:
            Serial.println("Comms/device error!");
            return false;
        default:
            Serial.print("Print already exists at ID "); Serial.println(rc);
            return false;
    }
}

bool delete_finger(uint16_t fid) {    
    uint16_t rc = finger.delete_id(fid);
    switch (rc) {
        case GT5X_OK:
            Serial.print("ID "); Serial.print(fid); 
            Serial.println(" deleted.");
            return true;
        case GT5X_NACK_INVALID_POS:
            Serial.println("ID not used!");
            return false;
        case GT5X_NACK_DB_IS_EMPTY:
            Serial.println("Database is empty!");
            return false;
        default:
            Serial.print("Error code: 0x"); Serial.println(rc, HEX);
            return false;
    }
}
