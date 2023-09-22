#include <Arduino.h>
#include <Artila-Matrix310.h>
#include <VernierLib32.h>
#include <R4AVA07.h>

#define TMP_CH 1

const int interval    = 2000;
const int sample_rate = 10;     // 10 samples per second

SSTempSensor TMP;
R4AVA07 ADC;

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, COM1_RX, COM1_TX);
    ADC.begin(&Serial1, COM1_RTS);
}

float getSample(int ch, int rate) {
    float sample = 0;
    for (int i = 0; i < rate; i++) {
        sample += ADC.readVoltage(ch);
    };
    // Return average
    return sample / rate;
}

void loop() {
    float voltage = ADC.readVoltage(1);
    Serial.printf("Voltage: %f - Temp: %f %s", voltage, TMP.readSensor(voltage), TMP.getSensorUnit());
}