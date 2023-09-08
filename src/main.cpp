#include <Arduino.h>
#include <Artila-Matrix310.h>
#include <VernierLib32.h>
#include <HX711.h>

const int interval    = 2000;
const int sample_rate = 10;     // 10 samples per second

HX711 ADC;
SSTempSensor TMP;

void setup() {
    Serial.begin(115200);
    ADC.begin(DI1, DO1);
}

void loop() {
    int rawADC = ADC.read_average(sample_rate);
    Serial.println((String) "rawADC: " + rawADC);
    Serial.println((String) "DI1: " + digitalRead(DI1));
    delay(interval);
}