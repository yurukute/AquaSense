#ifndef VernierLib32_h
#define VernierLib32_h
#include <math.h>
#endif
#define VERNLIB32_VERSION "1.0.0"

/* Custom library based on Vernier's VernierLib.
   Compatible with Artila Matrix-310.
*/

class Vernier {
  protected:
    float slope;
    float intercept;
    char sensorUnit[7];
  public:
    virtual ~Vernier() = 0;
    // Return sensor's current unit of measurement.
    char* getSensorUnit()           { return sensorUnit; };
    // Calculate the sensor value from measured voltage.
    float readSensor(float voltage) { return slope * voltage + intercept; };
    // Calculate the sensor value from ADC count.
    float readSensor(int rawADC) { return slope * rawADC*5.0/1023 + intercept; };
};

// Stainless Steel Temperature sensor.
class SSTempSensor : public Vernier {
  private:
    // Steinhart-Hart coefficients:
    const double K0 = 0.00102119;
    const double K1 = 0.000222468;
    const double K2 = 0.000000133342;
    // Sensor thermistor:
    const int therm = 20000;
    // Schematic:
    // [GND] -- [Thermistor] ----- | -- [Pad resistor] --[Vcc (5v)]
    //                             |
    //                       Analog input
    // Balance resistor (pad resistor):
    float pad = 15000;
    // Return temperature value using Steinhart-Hart equation
    float calculateTemp(float resistance);
  public:
    SSTempSensor();
    // Return balance resistance.
    int getBalanceResistance()       { return pad; };
    // Set balance resistor value.
    void setBalanceResistance(int r) { pad = r; };
    //  Switch between Celcius and Fahrenheit, default is Celcius.
    void switchUnit();
    float readSensor(float voltage);
    float readSensor(int rawADC);
};

// Optical Dissolved Oxygen sensor.
class ODOSensor : public Vernier {
  public:
    ODOSensor();
    //  Switch between mg/L, default is mg/L.
    void switchUnit();
};

// Tris-compatible Flat pH sensor.
class FPHSensor : public Vernier {
  public:
    FPHSensor();
};

// END OF FILE