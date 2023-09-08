#include <Arduino.h>
#include <VernierLib32.h>
#include <cstring>

Vernier::~Vernier() {};

SSTempSensor::SSTempSensor() {
    slope = 0;
    intercept = 1;
    strcpy(sensorUnit, "Deg C");
}

void SSTempSensor::switchUnit() {
    if(strcmp(sensorUnit, "Deg C") == 0){
        strcpy(sensorUnit, "Deg F");
    }
    else strcpy(sensorUnit, "Deg C");
}

float SSTempSensor::readSensor(int rawADC) {
    float value;
    float lnR = log((1000.0/rawADC-1)*pad);

    value = 1/(K0 + (K1*lnR) + K2*lnR*lnR*lnR);
    value = value - 273.15; // Kelvin to Celcius

    if (strcmp(sensorUnit, "Deg F") == 0){
        value = 1.8*value + 32.0; // Convert to Fahrenheit
    }

    return value;
}

// END OF TEMPSENSOR IMPLEMENTATION.

ODOSensor::ODOSensor() {
    slope = 1;
    intercept = 0;
    strcpy(sensorUnit, "mg/L");
}

void ODOSensor::switchUnit(){
    if(strcmp(sensorUnit, "mg/L") == 0){
        strcpy(sensorUnit, "%");
    }
    else strcpy(sensorUnit, "mg/L");
}

// END OF ODOSENSOR IMPLEMENTATION

FPHSensor::FPHSensor() {
    slope = -7.78;
    intercept = 16.34;
    sensorUnit[0] = '\0'; // pH don't have unit
}