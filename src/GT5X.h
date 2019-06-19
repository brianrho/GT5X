/* Written by Brian Ejike (2018) brianrho94@gmail.com
 * Distributed under the terms of the MIT license */
 
#ifndef GT5X_H
#define GT5X_H

/* uncomment to enable debug output */
//#define GT5X_ENABLE_DEBUG

#define GT5X_BUFLEN     32

#define GT5X_TEMPLATESZ         498
#define GT5X_IMAGESZ            19200   /* 160 x 120 */
 
/* commands */   
#define GT5X_OPEN                           0x01    
#define GT5X_CLOSE                          0x02    
#define GT5X_USBINTCHECK                    0x03    
#define GT5X_CHANGEBAUDRATE                 0x04    
#define GT5X_SETIAPMODE                     0x05    
#define GT5X_CMOSLED                        0x12    
#define GT5X_GETENROLLCNT                   0x20    
#define GT5X_CHECKENROLLED                  0x21    
#define GT5X_STARTENROLL                    0x22    
#define GT5X_ENROLL1                        0x23    
#define GT5X_ENROLL2                        0x24    
#define GT5X_ENROLL3                        0x25    
#define GT5X_ISPRESSFINGER                  0x26    
#define GT5X_DELETEID                       0x40    
#define GT5X_DELETEALL                      0x41    
#define GT5X_VERIFY1_1                      0x50    
#define GT5X_IDENTIFY1_N                    0x51    
#define GT5X_VERIFYTEMPLATE1_1              0x52    
#define GT5X_IDENTIFYTEMPLATE1_N            0x53    
#define GT5X_CAPTUREFINGER                  0x60    
#define GT5X_MAKETEMPLATE                   0x61    
#define GT5X_GETIMAGE                       0x62    
#define GT5X_GETRAWIMAGE                    0x63    
#define GT5X_GETTEMPLATE                    0x70    
#define GT5X_SETTEMPLATE                    0x71    
#define GT5X_GETDATABASESTART               0x72    
#define GT5X_GETDATABASEEND                 0x73    
#define GT5X_UPGRADEFIRMWARE                0x80    
#define GT5X_UPGRADEISOCDIMAGE              0x81    
#define GT5X_ACK                            0x30    
#define GT5X_NACK                           0x31    

/* NACK error codes */

#define GT5X_OK                             0x1000
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

/* command header stuff */
#define GT5X_CMD_START_CODE1                0x55
#define GT5X_CMD_START_CODE2                0xAA

#define GT5X_DATA_START_CODE1               0x5A
#define GT5X_DATA_START_CODE2               0xA5

#define GT5X_DEVICEID                       0x0001

#define GT5X_PARAM_CMD_LEN                  6

/* returned whenever we time out while reading */
#define GT5X_TIMEOUT                        0xFFFF

/* default uart read timeout */
#define GT5X_DEFAULT_TIMEOUT                1000

class Stream;

typedef struct { 
    uint32_t fwversion; 
    uint32_t iso_max_size;
    uint8_t sn[16];
} GT5X_DeviceInfo;

/* possible destinations for template/image data read from the module */
enum {
    GT5X_OUTPUT_TO_STREAM,
    GT5X_OUTPUT_TO_BUFFER
};

class GT5X {
    public:
        GT5X(Stream * ss);
        bool begin(GT5X_DeviceInfo * info = NULL);
        bool end(void);
        
        /* all output params and error codes are within 2 bytes
           so uint16_t is good enough */
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
        
        uint16_t get_template(uint16_t fid);
        uint16_t get_image(void);
        uint16_t set_template(uint16_t fid, uint8_t check_duplicate = true);
        
        bool read_raw(uint8_t outType, void * out, uint16_t to_read);
        uint16_t write_raw(uint8_t * data, uint16_t len, bool expect_response = false);
        
    private:
        void write_cmd_packet(uint16_t cmd, uint32_t params);
        uint16_t get_cmd_response(uint32_t * params);
        uint16_t get_data_response(uint8_t * data, uint16_t len, Stream * outStream = NULL);
        
        Stream * port;
        GT5X_DeviceInfo devinfo;
        uint8_t buffer[GT5X_BUFLEN];
};

#endif

