#ifndef AMVIF08_H
#define AMVIF08_H
#include <Arduino.h>
#include <map>
#include <vector>
#include "sys/_stdint.h"
#include "ModbusRTU.h"
#endif
#define AMVIF08LIB_VERSION "1.0.0"


class AMVIF08 : ModbusDevice {
    std::map<short, uint16_t> baudrate {
        {1200, 0},
        {2400, 1},
        {4800, 2},
        {9600, 3},
        {19200, 4},
        {38400, 5},
        {57600, 6},
        {115200, 7}
    };

  private:
    std::string name = "AMVIF08";
    unsigned short prod_id = 2048;
    uint8_t addr;
    short return_time;
    short baud;
    char parity;

    bool isValid(unsigned short ch);
    
  public:
    int begin(Stream* rs485_port, uint8_t rst_pin = 0);
    // Read channel's voltage
    std::vector<float> readVoltage(uint16_t ch,
                                   uint8_t number = 0x01);
    // Return channel's voltage ratio
    std::vector<float> getVoltageRatio(uint16_t ch,
                                       uint8_t number = 0x01);
    // Return device name
    std::string getName()         { return name; }
    // Return product's ID
    unsigned short getProductID() { return prod_id; }
    // Return time interal for response in ms
    short getReturnTime()         { return return_time; };
    // Return slave's address
    uint8_t getAddr()             { return addr; };
    // Return current baud rate
    short getBaud()               { return baud; }
    // Return parity type
    char getParity()              { return parity; }

    // Set channel's voltage ratio
    short setVoltageRatio(uint16_t ch, float ratio);
    // Factory reset
    short factoryReset();
    // Set time interval for command return
    short setReturnTime(uint16_t msec);
    // Set slave's address
    short setAddr(uint16_t new_addr);
    // Change serial  baud rate
    short setBaudRate(unsigned short baud = 9600);
    // Change parity check type
    short setParity(char type);
};