#include <Arduino.h>
#include <Artila-Matrix310.h>
#include <VernierLib32.h>
#include <R4AVA07.h>

#define VIN_CH 0
#define TMP_CH 1
#define Rl 5860.0

const int interval = 2000;
const int sample_rate = 10; // 10 samples per second

std::vector<float> voltage_avg(CH_MAX, 0);
SSTempSensor TMP;
R4AVA07 ADC;

void setup() {
    pinMode(DO1, OUTPUT);
    digitalWrite(DO1, LOW);
    delay(1500); // Wait for relay
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, COM1_RX, COM1_TX);
    ADC.begin(&Serial1, COM1_RTS);
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
    Serial.printf("Vout: %.2f, Therm: %.0f\n", vout, Rp);
    return TMP.calculateTemp(Rp);
}

void loop() {
    voltage_avg = ADC.readVoltage(1, 2);
    Serial.printf("Temperature: %.2f\n", readTemp());
    delay(1000);
}