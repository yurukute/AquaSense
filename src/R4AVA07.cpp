#include <R4AVA07.h>
#include <arpa/inet.h>

#define READ  0x03
#define WRITE 0x06
#define BROADCAST 0xFF
#define STN_ADDR  0x000E
#define BAUD_RATE 0x000F
// Since the module has 7 channels, read data can be up to 14 bytes.
// plus 1 byte addr, 1 byte function, 1 byte data size and 2 byte CRC.
#define BUFFER_SIZE 19
#define ID_MAX 247

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
    return (ch >= 1 && ch <= CH_MAX);
}

int R4AVA07::send(uint8_t rs485_addr, uint8_t func, uint32_t data) {
    uint8_t msg[8] = {0x00};
    uint8_t respone[BUFFER_SIZE];
    int read_size;

    msg[0] = rs485_addr;
    msg[1] = READ;
    *(u_int32_t *)(&msg[2]) = htonl(data);
    *(u_int16_t *)(&msg[6]) = calculateCRC(&msg[0], 6);
    
    digitalWrite(rst, HIGH);
    delay(1);

    rs485->write((byte *) msg, sizeof(msg));
    rs485->flush();

    digitalWrite(rst, LOW);
    delay(1);

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
    else if (func == READ) {
        for (auto i = 0; i < respone[2]; i++) {
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

int R4AVA07::begin(Stream* rs485_port, uint8_t rst_pin = 0) {
    rs485 = rs485_port;
    rst = rst_pin;
    pinMode(rst, OUTPUT);

    // Find ID
    send(BROADCAST, READ, STN_ADDR << 16 | 0x01);
    ID = read_data[0];
    if (!ID) { // ID not found.
        DEBUG_PRINT("Cannot find device address.");
        return -1;
    } else DEBUG_PRINT("Found at: " + ID);
    // Find baud rate
    send(ID, READ, BAUD_RATE << 16 | 0x01);
    baud = read_data[0];
    return 0;
}

std::vector<float>  R4AVA07::readVoltage(uint32_t ch, uint8_t number) {
    if (isValid(ch) == false) {
        DEBUG_PRINT("Invalid channel: " + ch);
        return {-1};
    }
    if (isValid(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " + number);
        return {-1};
    }
    // Channel 1-7 indicated at 0x0000-0x0006.
    if (send(ID, READ, (ch-1) << 16 | number) < 1) {
        DEBUG_PRINT("Cannot read voltage values.");
        return {-1};
    }
    std::vector<float> voltage;
    for (auto i = 0; i < number; i++) {
        voltage.push_back((float) read_data[i] / 100.0);
    }
    return voltage;
}

std::vector<float> R4AVA07::getVoltageRatio(uint32_t ch,
                                            uint8_t number) {
    if (isValid(ch) == false) {
        DEBUG_PRINT("Invalid channel: " + ch);
        return {-1};
    }
    if (isValid(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " + number);
        return {-1};
    }
    // Channel 1-7 indicated at 0x0007-0x000D.
    if (send(ID, READ, (ch+6) << 16 | number) < 1) {
        DEBUG_PRINT("Cannot read voltage ratios.");
        return {-1};
    }
    std::vector<float> ratio;
    for (auto i = 0; i < number; i++) {
        ratio.push_back((float) read_data[i] / 1000.0);
    }
    return ratio;
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