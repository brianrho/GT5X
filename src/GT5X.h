/* Written by Brian Ejike (2018) brianrho94@gmail.com
 * Distributed under the terms of the MIT license */
 
#ifndef GT5X_H
#define GT5X_H

/* uncomment to enable debug output */
//#define GT5X_ENABLE_DEBUG

#define GT5X_BUFLEN     32

/* 12-byte command packet */
#define GT5X_NOT_SET                        0x00    // Default value for enum. Scanner will return error if sent this.
#define GT5X_OPEN                           0x01    // Open Initialization
#define GT5X_CLOSE                          0x02    // Close Termination
#define GT5X_USBINTCHECK                    0x03    // UsbInternalCheck Check if the connected USB device is valid
#define GT5X_CHANGEBAUDRATE                 0x04    // Change UART baud rate
#define GT5X_SETIAPMODE                     0x05    // SetIAPMode Enter IAP Mode In this mode, FW Upgrade is available
#define GT5X_CMOSLED                        0x12    // CmosLed Control
#define GT5X_GETENROLLCNT                   0x20    // Get enrolled fingerprint count
#define GT5X_CHECKENROLLED                  0x21    // Check whether the specified ID is already enrolled
#define GT5X_STARTENROLL                    0x22    // Start an enrollment
#define GT5X_ENROLL1                        0x23    // Make 1st template for an enrollment
#define GT5X_ENROLL2                        0x24    // Make 2nd template for an enrollment
#define GT5X_ENROLL3                        0x25    // Make 3rd template for an enrollment, merge into one template, save
#define GT5X_ISPRESSFINGER                  0x26    // Check if a finger is placed on the sensor
#define GT5X_DELETEID                       0x40    // Delete the fingerprint with the specified ID
#define GT5X_DELETEALL                      0x41    // Delete all fingerprints from the database
#define GT5X_VERIFY1_1                      0x50    // Verify captured fingerprint image with a specified ID
#define GT5X_IDENTIFY1_N                    0x51    // Search database for a captured finger
#define GT5X_VERIFYTEMPLATE1_1              0x52    // Match a given template with another template in the database
#define GT5X_IDENTIFYTEMPLATE1_N            0x53    // Search database for a given template
#define GT5X_CAPTUREFINGER                  0x60    // Capture a fingerprint image (256 x 256) with the sensor
#define GT5X_MAKETEMPLATE                   0x61    // Make template for transmission
#define GT5X_GETIMAGE                       0x62    // Download the captured fingerprint image(256x256)
#define GT5X_GETRAWIMAGE                    0x63    // Capture & download raw fingerprint image(320x240)
#define GT5X_GETTEMPLATE                    0x70    // Download the template at a specified ID
#define GT5X_SETTEMPLATE                    0x71    // Upload a template to a specified ID
#define GT5X_GETDATABASESTART               0x72    // Start database download, obsolete
#define GT5X_GETDATABASEEND                 0x73    // End database download, obsolete
#define GT5X_UPGRADEFIRMWARE                0x80    // Not supported
#define GT5X_UPGRADEISOCDIMAGE              0x81    // Not supported
#define GT5X_ACK                            0x30    // Acknowledge
#define GT5X_NACK                           0x31    // Non-acknowledge

/*
	Response_Packet represents the returned data from the finger print scanner 
*/

#define GT5X_OK                             0x0000
#define GT5X_NACK_TIMEOUT                   0x1001
#define GT5X_NACK_INVALID_BAUDRATE          0x1002
#define GT5X_NACK_INVALID_POS               0x1003
#define GT5X_NACK_IS_NOT_USED               0x1004
#define GT5X_NACK_IS_ALREADY_USED           0x1005
#define GT5X_NACK_COMM_ERR                  0x1006
#define GT5X_NACK_VERIFY_FAILED             0x1007
#define GT5X_NACK_IDENTIFY_FAILED           0x1008
#define GT5X_NACK_DB_IS_FULL                0x1009
#define GT5X_NACK_DB_IS_EMPTY               0x100A
#define GT5X_NACK_TURN_ERR                  0x100B
#define GT5X_NACK_BAD_FINGER                0x100C
#define GT5X_NACK_ENROLL_FAILED             0x100D
#define GT5X_NACK_IS_NOT_SUPPORTED          0x100E
#define GT5X_NACK_DEV_ERR                   0x100F
#define GT5X_NACK_CAPTURE_CANCELED          0x1010
#define GT5X_NACK_INVALID_PARAM             0x1011
#define GT5X_NACK_FINGER_IS_NOT_PRESSED     0x1012
#define GT5X_INVALID                        0xFFFF

#define GT5X_CMD_START_CODE1                0x55
#define GT5X_CMD_START_CODE2                0xAA

#define GT5X_DATA_START_CODE1               0x5A
#define GT5X_DATA_START_CODE2               0xA5

#define GT5X_DEVICEID                       0x0001

#define GT5X_PARAM_CMD_LEN                  6

/* returned whenever we time out while reading */
#define GT5X_TIMEOUT                        0xFFFF

#define GT5X_DEFAULT_TIMEOUT                1000

class Stream;

typedef struct { 
    uint32_t fwversion; 
    uint32_t iso_max_size;
    uint8_t sn[16];
} GT5X_DeviceInfo;

class GT5X {
    public:
        GT5X(Stream * ss);
        bool begin(GT5X_DeviceInfo * info = NULL);
        bool end(void);
        
        uint16_t set_led(bool state);
        uint16_t set_baud_rate(uint32_t baud);
        uint16_t get_enrolled_count(uint16_t * fcnt);
        uint16_t is_enrolled(uint16_t fid);
        uint16_t start_enroll(uint16_t fid);
        uint16_t enroll_scan(uint8_t pass);
        bool is_pressed(void);
        uint16_t delete_id(uint16_t fid);
        uint16_t empty_database(void);
        uint16_t verify_finger_with_template(uint16_t fid);
        uint16_t search_database(uint16_t * fid);
        uint16_t capture_finger(bool highquality = false);
        
    private:
        void write_cmd_packet(uint16_t cmd, uint32_t params);
        uint16_t get_cmd_response(uint32_t * params);
        uint16_t get_data_response(uint8_t * data, uint16_t len, Stream * outStream = NULL);
        
        Stream * port;
        GT5X_DeviceInfo devinfo;
        uint8_t buffer[GT5X_BUFLEN];
};

#endif

