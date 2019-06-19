/* Written by Brian Ejike (2018) brianrho94@gmail.com
 * Distributed under the terms of the MIT license */
 
#include <Arduino.h>
#include "GT5X.h"

#if defined(GT5X_ENABLE_DEBUG)
    #define GT5X_DEFAULT_STREAM          Serial

    #define GT5X_DEBUG_PRINT(x)          GT5X_DEFAULT_STREAM.print(x)
    #define GT5X_DEBUG_PRINTLN(x)        GT5X_DEFAULT_STREAM.println(x)
    #define GT5X_DEBUG_DEC(x)            GT5X_DEFAULT_STREAM.print(x)
    #define GT5X_DEBUG_DECLN(x)          GT5X_DEFAULT_STREAM.println(x)
    #define GT5X_DEBUG_HEX(x)            GT5X_DEFAULT_STREAM.print(x, HEX)
    #define GT5X_DEBUG_HEXLN(x)          GT5X_DEFAULT_STREAM.println(x, HEX) 
#else
    #define GT5X_DEBUG_PRINT(x)
    #define GT5X_DEBUG_PRINTLN(x)
    #define GT5X_DEBUG_DEC(x)          
    #define GT5X_DEBUG_DECLN(x)
    #define GT5X_DEBUG_HEX(x)
    #define GT5X_DEBUG_HEXLN(x)
#endif

typedef enum {
    GT5X_STATE_READ_HEADER,
    GT5X_STATE_READ_DEVID,
    GT5X_STATE_READ_PARAMS,
    GT5X_STATE_READ_RESPONSE,
    GT5X_STATE_READ_DATA,
    GT5X_STATE_READ_CHECKSUM
} GT5X_State;

void GT5X::write_cmd_packet(uint16_t cmd, uint32_t params) {   
    uint8_t preamble[] = {GT5X_CMD_START_CODE1, GT5X_CMD_START_CODE2, 
                          (uint8_t)GT5X_DEVICEID, (uint8_t)(GT5X_DEVICEID >> 8)};
    
    uint16_t chksum = 0;
    for (int i = 0; i < sizeof(preamble); i++) {
        chksum += preamble[i];
    }
    
    memcpy(buffer, &params, sizeof(params));
    memcpy(buffer + sizeof(params), &cmd, sizeof(cmd));
    
    for (int i = 0; i < GT5X_PARAM_CMD_LEN; i++) {
        chksum += buffer[i];
    }
    
    port->write(preamble, sizeof(preamble));
    port->write(buffer, GT5X_PARAM_CMD_LEN);
    port->write((uint8_t *)&chksum, 2);
}

/* Any output parameter (or error code) is stored right back into params
   and the Response ACK/NACK is returned */
   
uint16_t GT5X::get_cmd_response(uint32_t * params) {
    GT5X_State state = GT5X_STATE_READ_HEADER;
    const uint16_t COMBINED_PACKET_HEADER = ((uint16_t)GT5X_CMD_START_CODE1 << 8) | GT5X_CMD_START_CODE2;
    
    uint16_t header = 0;
    uint16_t rcode = 0;
    uint32_t last_read = millis();
    
    while ((uint32_t)(millis() - last_read) < GT5X_DEFAULT_TIMEOUT) {
        yield();
        
        switch (state) {
            case GT5X_STATE_READ_HEADER: {
                if (port->available() == 0)
                    continue;
                
                last_read = millis();
                uint8_t byte = port->read();
                header <<= 8; header |= byte;
                if (header != COMBINED_PACKET_HEADER)
                    break;
                
                state = GT5X_STATE_READ_DEVID;
                header = 0;
                
                GT5X_DEBUG_PRINTLN("\r\n[+]Got header");
                break;
            }
            case GT5X_STATE_READ_DEVID: {
                if (port->available() < 2)
                    continue;
                
                last_read = millis();
                uint16_t devid;
                port->readBytes((uint8_t *)&devid, 2);
                
                /* check device id */
                if (devid != GT5X_DEVICEID) {
                    state = GT5X_STATE_READ_HEADER;
                    GT5X_DEBUG_PRINTLN("[+]Wrong device ID");
                    break;
                }
                
                state = GT5X_STATE_READ_PARAMS;
                GT5X_DEBUG_PRINT("[+]ID: 0x"); GT5X_DEBUG_HEXLN(devid);
                
                break;
            }
            case GT5X_STATE_READ_PARAMS:
                if (port->available() < 4)
                    continue;
                
                /* store output parameter or error code */
                last_read = millis();
                port->readBytes((uint8_t *)params, 4);
                
                state = GT5X_STATE_READ_RESPONSE;
                GT5X_DEBUG_PRINT("[+]Params: 0x"); GT5X_DEBUG_HEXLN(*params);
                
                break;
            case GT5X_STATE_READ_RESPONSE: {
                if (port->available() < 2)
                    continue;
                
                /* read ACK/NACK */
                last_read = millis();
                port->readBytes((uint8_t *)&rcode, 2);
                
                state = GT5X_STATE_READ_CHECKSUM;
                GT5X_DEBUG_PRINT("[+]Response code: "); GT5X_DEBUG_DECLN(rcode);
                break;
            }
            case GT5X_STATE_READ_CHECKSUM: {
                if (port->available() < 2)
                    continue;
                
                last_read = millis();
                uint16_t temp;
                port->readBytes((uint8_t *)&temp, 2);
                
                uint16_t chksum = GT5X_CMD_START_CODE1 + GT5X_CMD_START_CODE2
                                  + (uint8_t)GT5X_DEVICEID + (GT5X_DEVICEID >> 8);
                
                /* 4 from size of params */
                for (int i = 0; i < 4; i++) {
                    chksum += ((uint8_t *)params)[i];
                }
                
                chksum += rcode >> 8;
                chksum += (uint8_t)rcode;
                
                /* compare chksum */
                if (temp != chksum) {
                    state = GT5X_STATE_READ_HEADER;
                    GT5X_DEBUG_PRINTLN("\r\n[+]Wrong chksum");
                    continue;
                }
                
                GT5X_DEBUG_PRINTLN("\r\n[+]Read complete");
                return rcode;
            }
        }
    }
    
    GT5X_DEBUG_PRINTLN("[+]Timeout.");
    return GT5X_TIMEOUT;
}

uint16_t GT5X::get_data_response(uint8_t * data, uint16_t len, Stream * outStream) {
    GT5X_State state = GT5X_STATE_READ_HEADER;
    const uint16_t COMBINED_PACKET_HEADER = ((uint16_t)GT5X_DATA_START_CODE1 << 8) | GT5X_DATA_START_CODE2;
    
    uint16_t header = 0;
    
    uint16_t remn = len;
    uint32_t last_read = millis();
    
    while ((uint32_t)(millis() - last_read) < GT5X_DEFAULT_TIMEOUT) {
        yield();
        
        switch (state) {
            case GT5X_STATE_READ_HEADER: {
                if (port->available() == 0)
                    continue;
                
                last_read = millis();
                uint8_t byte = port->read();
                header <<= 8; header |= byte;
                if (header != COMBINED_PACKET_HEADER)
                    break;
                
                state = GT5X_STATE_READ_DEVID;
                header = 0;
                
                GT5X_DEBUG_PRINTLN("\r\n[+]Got header");
                break;
            }
            case GT5X_STATE_READ_DEVID: {
                if (port->available() < 2)
                    continue;
                
                last_read = millis();
                uint16_t devid;
                port->readBytes((uint8_t *)&devid, 2);
                
                if (devid != GT5X_DEVICEID) {
                    state = GT5X_STATE_READ_HEADER;
                    GT5X_DEBUG_PRINTLN("[+]Wrong device ID");
                    break;
                }
                
                state = GT5X_STATE_READ_DATA;
                GT5X_DEBUG_PRINT("[+]ID: 0x"); GT5X_DEBUG_HEXLN(devid);
                
                break;
            }
            case GT5X_STATE_READ_DATA: {
                uint16_t avail = port->available();
                uint16_t to_read;
                
                if (avail == 0)
                    continue;
                
                last_read = millis();
                
                if (outStream == NULL) {
                    to_read = (avail < remn) ? avail : remn;
                    port->readBytes(data, to_read);
                    data += to_read;
                }
                else {
                    to_read = (avail < remn) ? avail : remn;
                    to_read = (to_read < GT5X_BUFLEN) ? to_read : GT5X_BUFLEN;
                    port->readBytes(buffer, to_read);
                    outStream->write(buffer, to_read);
                }
                
                remn -= to_read;
                
                if (remn == 0) {
                    state = GT5X_STATE_READ_CHECKSUM;
                    GT5X_DEBUG_PRINT("[+]Read len: "); GT5X_DEBUG_DECLN(len);
                }
                
                break;
            }
            case GT5X_STATE_READ_CHECKSUM: {
                if (port->available() < 2)
                    continue;
                
                last_read = millis();
                uint16_t temp;
                port->readBytes((uint8_t *)&temp, 2);
                
                if (outStream == NULL) {
                    uint16_t chksum = GT5X_DATA_START_CODE1 + GT5X_DATA_START_CODE2
                                      + (uint8_t)GT5X_DEVICEID + (GT5X_DEVICEID >> 8);
                    
                    /* walk backwards thru the data and add */                    
                    for (int i = 0; i < len; i++) {
                        chksum += *(--data);
                    }
                    
                    if (temp != chksum) {
                        state = GT5X_STATE_READ_HEADER;
                        GT5X_DEBUG_PRINTLN("\r\n[+]Wrong chksum");
                        continue;
                    }
                }
                
                GT5X_DEBUG_PRINTLN("\r\n[+]Read complete");
                return len;
            }
        }
    }
    
    GT5X_DEBUG_PRINTLN();
    return GT5X_TIMEOUT;
}

GT5X::GT5X(Stream * ss) : port(ss) 
{
    
}

bool GT5X::begin(GT5X_DeviceInfo * info) {
    uint16_t cmd = GT5X_OPEN;
    uint32_t params = 1;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    
    if (rc != GT5X_ACK)
        return false;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    rc = get_data_response((uint8_t *)&devinfo, sizeof(GT5X_DeviceInfo));
    if (rc != sizeof(GT5X_DeviceInfo))
        return false;
    
    if (info != NULL) {
        memcpy(info, &devinfo, sizeof(GT5X_DeviceInfo));
    }
    
    return true;
}

bool GT5X::end(void) {
    uint16_t cmd = GT5X_CLOSE;
    uint32_t params = 0;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    return (rc == GT5X_ACK);
}

uint16_t GT5X::set_led(bool state) {
    uint16_t cmd = GT5X_CMOSLED;
    uint32_t params = state ? 1 : 0;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

/* trust the device to handle invalid rates, 
 * will need to call end() and begin() after this */
uint16_t GT5X::set_baud_rate(uint32_t baud) {
    uint16_t cmd = GT5X_CHANGEBAUDRATE;
    uint32_t params = baud;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    /* returns the NACK error code, same purpose in other functions */
    return params;
}

/* get number of enrolled templates */
uint16_t GT5X::get_enrolled_count(uint16_t * fcnt) {
    uint16_t cmd = GT5X_GETENROLLCNT;
    uint32_t params = 0;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK) {
        *fcnt = params;
        return GT5X_OK;
    }
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

/* IDs 0-2999, if using GT-521F52
 * IDs 0-199, if using GT-521F32/GT-511C3 */
uint16_t GT5X::is_enrolled(uint16_t fid) {
    uint16_t cmd = GT5X_CHECKENROLLED;
    uint32_t params = fid;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

/** Starts the enrollment process
 *  IDs 0-2999, if using GT-521F52
 *  0-199, if using GT-521F32/GT-511C3
 */
uint16_t GT5X::start_enroll(uint16_t fid) {
    uint16_t cmd = GT5X_STARTENROLL;
    uint32_t params = fid;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

/** Scan finger for enrollment
 *  
 */
uint16_t GT5X::enroll_scan(uint8_t pass) {
    uint16_t cmd;
    
    switch (pass) {
        case 1:
            cmd = GT5X_ENROLL1;
            break;
        case 2:
            cmd = GT5X_ENROLL2;
            break;
        default:
            cmd = GT5X_ENROLL3;
            break;
    }
    
    uint32_t params = 0;
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

bool GT5X::is_pressed(void) {
    uint16_t cmd = GT5X_ISPRESSFINGER;
    uint32_t params = 0;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK) {
        return params == 0;
    }
    else
        return rc;
}

uint16_t GT5X::delete_id(uint16_t fid) {
    uint16_t cmd = GT5X_DELETEID;
    uint32_t params = fid;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

uint16_t GT5X::empty_database(void) {
    uint16_t cmd = GT5X_DELETEALL;
    uint32_t params = 0;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

/* For 1:1 matching */
uint16_t GT5X::verify_finger_with_template(uint16_t fid) {
    uint16_t cmd = GT5X_VERIFY1_1;
    uint32_t params = fid;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

uint16_t GT5X::search_database(uint16_t * fid) {
    uint16_t cmd = GT5X_IDENTIFY1_N;
    uint32_t params = 0;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK) {
        *fid = params;
        return GT5X_OK;
    }
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

uint16_t GT5X::capture_finger(bool highquality) {
    uint16_t cmd = GT5X_CAPTUREFINGER;
    uint32_t params = highquality ? 1 : 0;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

uint16_t GT5X::get_template(uint16_t fid) {
    uint16_t cmd = GT5X_GETTEMPLATE;
    uint32_t params = fid;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

uint16_t GT5X::get_image(void) {
    uint16_t cmd = GT5X_GETRAWIMAGE;
    uint32_t params = 0;
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

uint16_t GT5X::set_template(uint16_t fid, uint8_t check_duplicate) {
    uint16_t cmd = GT5X_SETTEMPLATE;
    uint32_t params = check_duplicate ? fid : (fid & 0xff000000);
    
    write_cmd_packet(cmd, params);
    uint16_t rc = get_cmd_response(&params);
    if (rc == GT5X_ACK)
        return GT5X_OK;
    else if (rc == GT5X_TIMEOUT)
        return rc;
    
    return params;
}

bool GT5X::read_raw(uint8_t outType, void * out, uint16_t to_read) {
    Stream * outStream;
    uint8_t * outBuf;
    
    if (outType == GT5X_OUTPUT_TO_BUFFER)
        outBuf = (uint8_t *)out;
    else if (outType == GT5X_OUTPUT_TO_STREAM)
        outStream = (Stream *)out;
    else
        return false;
    
    uint16_t rc;
    
    if (outType == GT5X_OUTPUT_TO_BUFFER)
        rc = get_data_response(outBuf, to_read);
    else if (outType == GT5X_OUTPUT_TO_STREAM)
        rc = get_data_response(NULL, to_read, outStream);
    
    /* check the length */
    if (rc != to_read) {
        GT5X_DEBUG_PRINT("Read data failed: ");
        GT5X_DEBUG_PRINTLN(rc);
        return false;
    }
    
    return true;
}

uint16_t GT5X::write_raw(uint8_t * data, uint16_t len, bool expect_response) {
    uint8_t preamble[] = {GT5X_DATA_START_CODE1, GT5X_DATA_START_CODE2, 
                          (uint8_t)GT5X_DEVICEID, (uint8_t)(GT5X_DEVICEID >> 8)};
    
    uint16_t chksum = 0;
    for (int i = 0; i < sizeof(preamble); i++) {
        chksum += preamble[i];
    }
    
    for (int i = 0; i < len; i++) {
        chksum += data[i];
    }
    
    port->write(preamble, sizeof(preamble));
    port->write(data, len);
    port->write((uint8_t *)&chksum, 2);
    
    if (expect_response) {
        uint32_t params = 0;
        uint16_t rc = get_cmd_response(&params);
        if (rc == GT5X_ACK)
            return GT5X_OK;
        else if (rc == GT5X_TIMEOUT)
            return rc;
        
        return params;
    }
    
    return GT5X_OK;
}
