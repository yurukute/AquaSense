#include "AMVIF08.h"

#define BROADCAST  0xFF
#define RESET_REG  0x00FB
#define RETURN_REG 0x00FC
#define ADDR_REG   0x00FD
#define BAUD_REG   0x00FE
#define PARITY_REG 0x00FF
#define ADDR_MAX 247
#define CH_MAX 8

bool AMVIF08::isValid(unsigned short ch) {
    return ch >= 1 && ch <= CH_MAX;
}

int AMVIF08::begin(Stream* rs485_port, uint8_t rst_pin) {
    ModbusDevice::begin(rs485_port, rst_pin);

    // Find slave address
    send(BROADCAST, MODBUS_READ, ADDR_REG << 16 | 0x01);
    addr = read_data[0];
    if (!addr) { // Address not found.
        DEBUG_PRINT("Cannot find device address.");
        return -1;
    } else DEBUG_PRINT("Found at: " + addr);

    baud = 9600;
    return 0;
}


std::vector<float> AMVIF08::readVoltage(uint16_t ch,
                                        uint8_t number) { 
    if (isValid(ch) == false) {
        DEBUG_PRINT("Invalid channel: " + ch);
        return {-1};
    }
    if (isValid(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " + number);
        return {-1};
    }
    // Channel 1-7 indicated at 0x00A0-0x00A7.
    if (send(addr, MODBUS_READ, (ch+0xA0-1) << 16 | number) < 1) {
        DEBUG_PRINT("Cannot read voltage values.");
        return {-1};
    }
    std::vector<float> voltage;
    for (auto i = 0; i < number; i++) {
        voltage.push_back((float) read_data[i] / 100.0);
    }
    return voltage;
}

std::vector<float> AMVIF08::getVoltageRatio(uint16_t ch,
                                            uint8_t number) {
    if (isValid(ch) == false) {
        DEBUG_PRINT("Invalid channel: " + ch);
        return {-1};
    }
    if (isValid(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " + number);
        return {-1};
    }
    // Channel 1-7 indicated at 0x00C0-0x00C7.
    if (send(addr, MODBUS_READ, (ch+0xC0-1) << 16 | number) < 1) {
        DEBUG_PRINT("Cannot read voltage ratio.");
        return {-1};
    }
    std::vector<float> voltage_ratio;
    for (auto i = 0; i < number; i++) {
        voltage_ratio.push_back((float) read_data[i] / 100.0);
    }
    return voltage_ratio;
}

short AMVIF08::setVoltageRatio(uint16_t ch, float ratio) {
    // Channel 1-7 indicated at 0x00C0-0x00C7.
    uint16_t val = ratio * 1000;
    if (send(addr, MODBUS_WRITE, (uint32_t) (ch+0xC0-1) << 16 | val) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio");
        return -1;
    }
    return 0;
}

short AMVIF08::setAddr(uint16_t new_addr) {
    if (new_addr < 1 || new_addr > ADDR_MAX) {
        DEBUG_PRINT("Invalid ID (1-" + ADDR_MAX + ").");
        return -1;
    }
    
    if (send(addr, MODBUS_WRITE, ADDR_REG << 16 | new_addr) < 0) {
        DEBUG_PRINT("Cannot set ID.");
        return -1;
    }
    
    addr = new_addr;
    return 0;
}

short AMVIF08::setBaudRate(unsigned short target_baud) {
    uint16_t baud_code;
    try {
        baud_code = baudrate.at(target_baud);
    }
    catch(std::out_of_range) {
        DEBUG_PRINT("Invalid baudrate.");
        return -1;
    }
    if (send(addr, MODBUS_WRITE, BAUD_REG << 16 | baud_code) < 1) {
        DEBUG_PRINT("Cannot change baud rate.");
    }

    baud = target_baud;
    return 0;
}

short AMVIF08::setParity(char type) {
    uint16_t paritycode = 0;
    switch (type) {
    case 'N': paritycode = 0; break;
    case 'O': paritycode = 1; break;
    case 'E': paritycode = 2; break;
    default:
        DEBUG_PRINT("Invalid parity type.");
    }

    if (send(addr, MODBUS_WRITE, PARITY_REG << 16 | paritycode) < 0) {
        DEBUG_PRINT("Cannot set parity.");
        return -1;
    }
    parity = type;
    return 0;
}

short AMVIF08::setReturnTime(uint16_t msec) {
    if (msec > 1000) {
        DEBUG_PRINT("Invalid return time.");
        return -1;
    }

    if (send(addr, MODBUS_WRITE, RETURN_REG << 16 | msec) < 0) {
        DEBUG_PRINT("Cannot set return time.");
        return -1;
    }

    return_time = msec;
    return 0;
}

short AMVIF08::factoryReset() {
    if (send(BROADCAST, MODBUS_WRITE, RESET_REG << 16 | 0) < 0) {
        return -1;
    }
    addr = 1;
    return_time = 1000;
    baud = 9600;
    parity = 'N';
    return 0;
}
