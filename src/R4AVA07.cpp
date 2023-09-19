#include "R4AVA07.h"
#include <arpa/inet.h>
#include <cstring>

#define READ  0x03
#define WRITE 0x06
#define BROADCAST 0xFF
#define STN_ADDR  0x000E
#define BAUD_RATE 0x000F
// Since the module has 7 channels, read data can be up to 14 bytes.
// plus 1 byte addr, 1 byte function, 1 byte data size and 2 byte CRC.
#define BUFFER_SIZE 19
#define ID_MAX 247
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(MSG)                                 \
    Serial.printf("%s, line %4d: ", __func__, __LINE__); \
    Serial.println((String)MSG);
#else
#define DEBUG_PRINT(MSG)
#endif

// CRC calculation
unsigned short calculateCRC(unsigned char *ptr, int len) {
    unsigned short crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ crc_table[(crc ^ *ptr++) & 0xff];
    }
    return (crc);
}

bool isValid(uint8_t ch) {
    return (ch <= 1 && ch <= 7);
}

int R4AVA07::send(uint8_t rs485_addr, uint8_t func, uint32_t data) {
    uint8_t   msg[8] = {0x00};
    uint8_t *respone = (uint8_t *)malloc(BUFFER_SIZE);

    msg[0] = rs485_addr;
    msg[1] = READ;
    *(u_int32_t *)(&msg[2]) = htonl(data);
    *(u_int16_t *)(&msg[6]) = calculateCRC(&msg[0], 6);
    
    delay(500);
    digitalWrite(rst, HIGH);

    rs485->write((byte *) msg, sizeof(msg));
    rs485->flush();

    digitalWrite(rst, LOW);
    delay(500);

    int read_size = (func == READ ? 5 + msg[5]*2 : sizeof(msg));
    rs485->readBytes(respone, read_size);
    rs485->flush();

#ifdef DEBUG
    DEBUG_PRINT("Sent message: ");
    for (int i = 0; i < 8; i++){
        Serial.printf("%02X ", msg[i]);
    }
    Serial.println();

    DEBUG_PRINT("Returned message: ");
    for (int i = 0; i < read_size; i++){
        Serial.printf("%02X ", respone[i]);
    }
    Serial.println();
#endif

    if (func == READ) {
        uint16_t value = (uint16_t) respone[3] << 8 | respone[4];
        return value;
    }

    uint16_t *set_value = (uint16_t *)(&msg[4]);
    uint16_t *reg_value = (uint16_t *)(&respone[4]);

    if (func == WRITE && *set_value != *reg_value) {
        return -1;
    }
    return 0;
}

void R4AVA07::begin(Stream* rs485_port, uint8_t rst_pin = 0) {
    rs485 = rs485_port;
    rst = rst_pin;

    // Find ID
    ID = send(BROADCAST, READ, STN_ADDR << 16 | 0x01);

    // Find baud rate
    baud = send(ID, READ, BAUD_RATE << 16 | 0x01);
}

float R4AVA07::readVoltage(short ch) {
    if (ch < 1 || ch > 7) {
        DEBUG_PRINT("Invalid channel.");
        return -1;
    }
    // Channel 1-7 indicated at 0x0000-0x0006.
    float voltage = send(ID, READ, (uint32_t) (ch-1) << 16 | 0x01);
    return voltage / 100.0;
}

float R4AVA07::getVoltageRatio(short ch) {
    if (ch < 1 || ch > 7) {
        DEBUG_PRINT("Invalid channel.");
        return -1;
    }
    // Channel 1-7 indicated at 0x0007-0x000D.
    float ratio = send(ID, READ, (uint32_t) (ch+6) << 16 | 0x01);
    return ratio / 1000.0;
}

int R4AVA07::setID(short newID) {
    if (newID < 1 || newID > ID_MAX) {
        DEBUG_PRINT("Invalid ID (1-" + ID_MAX + ").");
        return -1;
    }
    
    if (send(ID, WRITE, STN_ADDR << 16 | newID) < 0) {
        DEBUG_PRINT("Cannot set ID.");
        return -1;
    }
    
    ID = newID;
    return 0;
}

int R4AVA07::setVoltageRatio(short ch, float ratio) {
    // Channel 1-7 indicated at 0x0007-0x000D.
    uint16_t val = ratio * 1000;
    if (send(ID, WRITE, (uint32_t) (ch+6) << 16 | val) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio");
        return -1;
    }
    return 0;
}

int R4AVA07::setBaudRate(uint16_t target_baud) {
    uint16_t baud_code;
    switch (target_baud) {
    case 1200:  baud_code = 0; break;
    case 2400:  baud_code = 1; break;
    case 4800:  baud_code = 2; break;
    case 9600:  baud_code = 3; break;
    case 19200: baud_code = 4; break;
    default:
        DEBUG_PRINT("Not supported baud rate");
        return -1;
    }

    if (send(ID, WRITE, BAUD_RATE << 16 | baud_code) < 1) {
        DEBUG_PRINT("Cannot change baud rate.");
    }

    baud = target_baud;
    return 0;
}

void R4AVA07::resetBaud() {
    send(ID, WRITE, BAUD_RATE << 16 | 0x05);
}