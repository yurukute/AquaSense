#include "sys/_stdint.h"
#include "ModbusRTU.h"
#include <map>
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/types.h>

#define BUFFER_SIZE 253

// CRC calculation
unsigned short calculateCRC(unsigned char *ptr, int len) {
    unsigned short crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ crc_table[(crc ^ *ptr++) & 0xff];
    }
    return (crc);
}

int ModbusDevice::send(uint8_t rs485_addr, uint8_t func, uint32_t data) {
    uint8_t msg[8] = {0x00};
    uint8_t respone[BUFFER_SIZE];
    int read_size;

    msg[0] = rs485_addr;
    msg[1] = func;
    *(uint32_t *)(&msg[2]) = htonl(data);
    *(uint16_t *)(&msg[6]) = calculateCRC(&msg[0], 6);
    
    digitalWrite(rst, HIGH);
    delay(100);

    rs485->write((byte *) msg, sizeof(msg));
    rs485->flush();

    digitalWrite(rst, LOW);
    delay(100);

    read_size = rs485->readBytes(respone, BUFFER_SIZE);
    rs485->flush();

#ifdef DEBUG
    char buffer[BUFFER_SIZE * 3];
    char *ptr = buffer;
    for (auto i = 0; i < sizeof(msg); i++){
        sprintf(ptr, "%02X ", msg[i]);
        ptr += 3;
    }
    *ptr = '\0';
    DEBUG_PRINT("Send message:\t" + buffer);
    if (read_size > 0) {
        ptr = buffer;
        for (auto i = 0; i < read_size; i++){
            sprintf(ptr, "%02X ", respone[i]);
            ptr += 3;
        }
        *ptr = '\0';
        DEBUG_PRINT("Return:\t" + buffer);
    }
#endif

    if (read_size == 0){
        DEBUG_PRINT("Send failed.");
    }
    else if (func == MODBUS_READ) {
        for (auto i = 0; i < respone[2]/2; i++) {
            // Starting from the 3rd byte, read 2 bytes each.
            auto pos = 2*i+3;
            read_data[i] = (uint16_t) respone[pos] << 8 | respone[pos+1];
        }
    }
    else {
        uint16_t *set_value = (uint16_t *)(&msg[4]);
        uint16_t *reg_value = (uint16_t *)(&respone[4]);
        if(*set_value != *reg_value) {
            return -1;
        }
    }
    return read_size;
}

int ModbusDevice::begin(Stream* rs485_port, uint8_t rst_pin = 0) {
    rs485 = rs485_port;
    rst = rst_pin;
    pinMode(rst, OUTPUT);
    return 0;
}
