#include "R4AVA07.h"

#define READ  0x03
#define WRITE 0x06
#define BROADCAST 0xFF
#define STN_ADDR  0x000E
#define BAUD_RATE 0x000F
#define BUFFER_SIZE 19
#define ID_MAX 247

#ifdef DEBUG
#define DEBUG_PRINT(MSG)                        \
    Serial.printf("%-20s, line %4d: %s\n",      \
                  __FUNCTION__, __LINE__, MGS);
#else
#define DEBUG_PRINT(MSG)
#endif

// CRC calculation
unsigned short calculateCRC(unsigned char *ptr, int len) {
    unsigned short crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ CRCHi[(crc ^ *ptr++) & 0xff];
    }
    return (crc);
}

bool isValid(uint8_t ch) {
    return (ch <= 1 && ch <= 7);
}

int R4AVA07::send(uint8_t rs485_addr, uint8_t func, uint32_t data) {
    msg[0] = rs485_addr;
    msg[1] = READ;
    *(u_int32_t *)(&msg[2]) = data;
    *(u_int16_t *)(&msg[6]) = calculateCRC(&msg[0], 6);

    delay(500);
    digitalWrite(rst, HIGH);

    rs485.write((byte *) msg, sizeof(msg));
    rs485.flush();

    digitalWrite(rst, LOW);
    delay(500);

    if (func == READ)
        rs485.readBytes(respone, 5 + msg[5]*2);
    else // if (func == WRITE)
        rs485.readBytes(respone, sizeof(msg));
    rs485.flush();

    DEBUG_PRINT("Sent message: " + msg);
    DEBUG_PRINT("Returned message: " + respone);

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


void R4AVA07::begin(Stream& rs485_port, uint8_t rst_pin = 0) {
    rs485 = rs485_port;
    rst = rst_pin;

    // Find ID
    send(BROADCAST, READ, STN_ADDR << 16 | 0x01);
    ID = respone[3] << 8 | respone[4];

    // Find baud rate
    send(ID, READ, BAUD_RATE << 16 | 0x01);
    switch (respone[2 + msg[5]*2]) {
    case 0: baud = 1200;  break;
    case 1: baud = 2400;  break;
    case 2: baud = 4800;  break;
    case 3: baud = 9600;  break;
    case 4: baud = 19200; break;
    }
}

float R4AVA07::readVoltage(uint16_t ch) {
    if (ch < 1 || ch > 7) {
        DEBUG_PRINT("Invalid channel.");
        return -1;
    }
    // Channel 1-7 indicated at 0x0000-0x0006.
    float voltage = send(ID, READ, (uint32_t) (ch-1) << 16 | 0x01);
    return voltage / 100.0;
}

float R4AVA07::getVoltageRatio(uint16_t ch) {
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
        DEBUG_PRINT("Invalid ID (1-" ID_MAX ").");
        return -1;
    }
    
    if (send(ID, WRITE, (uint32_t) STN_ADDR << 16 | newID) < 0) {
        DEBUG_PRINT("Cannot set ID.");
        return -1;
    }
    
    ID = newID;
    return 0;
}

int R4AVA07::setVoltageRatio(uint8_t ch, float ratio) {
    // Channel 1-7 indicated at 0x0007-0x000D.
    uint16_t val = ratio * 1000;
    if (send(ID, WRITE, (uint32_t) (ch+6) << 16 | val) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio");
        return -1;
    }
    return 0;
}

int R4AVA07::changeBaud(uint16_t target_baud) {
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

    if (send(ID, WRITE, (uint32_t) BAUD_RATE << 16 | baud_code) < 1) {
        DEBUG_PRINT("Cannot change baud rate.");
    }

    baud = target_baud;
    return 0;
}

void R4AVA07::resetBaud() {
    send(ID, WRITE, (uint32_t) BAUD_RATE << 16 | 0x05);
}