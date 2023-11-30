#ifndef R4AVA07_H
#define R4AVA07_H
#include <Arduino.h>
#include <map>
#include <vector>
#include "sys/_stdint.h"
#include "ModbusRTU.h"
#endif
#define R4AVA07LIB_VERSION "1.0.0"


class R4AVA07 : ModbusDevice {
    
    std::map<short, uint16_t> baudrate {
        {1200, 0},
        {2400, 1},
        {4800, 2},
        {9600, 3},
        {19200, 4}
    };

  private:
    short ID;
    short baud;
    bool isValid(uint8_t ch);

  public:
    int begin(Stream* rs485_port, uint8_t rst_pin);
    // Return slave's ID
    short getID()   { return ID; };
    // Return current baud rate
    short getBaud() { return baud; };
    // Read channel's voltage 
    std::vector<float> readVoltage(uint32_t ch, uint8_t number = 0x01);
    // Return channel's voltage ratio
    std::vector<float> getVoltageRatio(uint32_t ch, uint8_t number = 0x01);

    // Set slave's ID
    int setID(short new_id);
    // Set channel's voltage ratio
    int setVoltageRatio(short ch, float val);
    // Change serial  baud rate
    int setBaudRate(short target_baud);
    // Reset serial baud rate
    void resetBaud();
};