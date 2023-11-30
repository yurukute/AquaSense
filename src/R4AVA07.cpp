#include <R4AVA07.h>

#define BROADCAST  0xFF
#define ADDR_REG   0x000E
#define BAUD_REG   0x000F
#define ID_MAX 247
#define CH_MAX 7

bool R4AVA07::isValid(uint8_t ch) {
    return (ch >= 1 && ch <= CH_MAX);
}

int R4AVA07::begin(Stream* rs485_port, uint8_t rst_pin = 0) {
    ModbusDevice::begin(rs485_port, rst_pin);

    // Find ID
    send(BROADCAST, MODBUS_READ, ADDR_REG << 16 | 0x01);
    ID = read_data[0];
    if (!ID) { // ID not found.
        DEBUG_PRINT("Cannot find device address.");
        return -1;
    } else DEBUG_PRINT("Found at: " + ID);

    baud = 9600;
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
    if (send(ID, MODBUS_READ, (ch-1) << 16 | number) < 1) {
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
    if (send(ID, MODBUS_READ, (ch+6) << 16 | number) < 1) {
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
    
    if (send(ID, MODBUS_WRITE, ADDR_REG << 16 | newID) < 0) {
        DEBUG_PRINT("Cannot set ID.");
        return -1;
    }
    
    ID = newID;
    return 0;
}

int R4AVA07::setVoltageRatio(short ch, float ratio) {
    // Channel 1-7 indicated at 0x0007-0x000D.
    uint16_t val = ratio * 1000;
    if (send(ID, MODBUS_WRITE, (uint32_t) (ch+6) << 16 | val) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio");
        return -1;
    }
    return 0;
}

int R4AVA07::setBaudRate(short target_baud) {
    uint16_t baud_code;
    try {
        baud_code = baudrate.at(target_baud);
    }
    catch(std::out_of_range) {
        DEBUG_PRINT("Invalid baudrate.");
        return -1;
    }
    if (send(ID, MODBUS_WRITE, BAUD_REG << 16 | baud_code) < 1) {
        DEBUG_PRINT("Cannot change baud rate.");
    }

    baud = target_baud;
    return 0;
}

void R4AVA07::resetBaud() {
    send(ID, MODBUS_WRITE, BAUD_REG << 16 | 0x05);
}
