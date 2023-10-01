#include <Arduino.h>
#include <Artila-Matrix310.h>
#include <VernierLib32.h>
#include <R4AVA07.h>
#include <algorithm>
#include <functional>
#include <vector>

#define VIN_CH 0
#define TMP_CH 1
#define ODO_CH 2
#define FPH_CH 3

const int interval       = 500;
const int sample_rate    = 10;    // 10 samples per second
const float Rl           = 5860;  // ADC resistance

std::vector<float> voltage_avg(CH_MAX, 0);
SSTempSensor TMP;
ODOSensor ODO;
R4AVA07 ADC;

void setup() {
    pinMode(DO1, OUTPUT);
    digitalWrite(DO1, LOW);
    delay(1500); // Wait for relay
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, COM1_RX, COM1_TX);
    Serial.print("Connecting to R4AVA07...");
    while(ADC.begin(&Serial1, COM1_RTS) < 0) {
        Serial.print(".");
    };
    Serial.println();
}

float readTemp() {
    float vin  = voltage_avg[VIN_CH];
    float vout = voltage_avg[TMP_CH];
    // Schematic:
    //   Rt: Thermistor
    //   R1: Divider resistor (Vernier's BTA protoboard)
    //   Rl: Load resistor (R4AVA07 ADC)
    //
    // [GND] --- [Rt] ----- | ----- [R1] -----[VCC (5v)]
    //       |              |
    //       |-- [Rl] ----- |
    //                      |
    //               [Analog input]
    // Parallel resistance:
    // Rp   = Rt*Rl/(Rt+Rl)
    // Vout = Vin*Rp/(R1+Rp)
    float pad = TMP.getDividerResistance();
    float Rp = 1/(vin/(pad*vout) - 1/Rl - 1/pad);
    return TMP.calculateTemp(Rp);
}

float readODO() {
    float vout = voltage_avg[ODO_CH];
    return ODO.readSensor(vout);
}

void readVoltage(int start, int num) {
    for (auto i = 0; i < sample_rate; i++) {
        std::transform(voltage_avg.begin(), voltage_avg.end(),
                       ADC.readVoltage(start,num).begin(),
                       voltage_avg.begin(), std::plus<float>());
    }
    for (auto &v : voltage_avg) {
        v /= sample_rate;
    }
}

void loop() {
    readVoltage(1, 3);
    Serial.println("Voltage read:\n\tCH1\tCH2\tCH3");
    for (auto v : voltage_avg) {
        Serial.printf("\t%.2f", v);
    }
    Serial.println();
    Serial.printf("Temperature: %04.2f\n", readTemp());
    Serial.printf("Dissolved oxygen: %04.2f\n", readODO());
    delay(interval);
}