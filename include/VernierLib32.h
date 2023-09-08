#ifndef VernierLib32_h
#define VernierLib32_h
#include <math.h>
#endif
#define LIB_LIB_VERSION "1.0"

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
    char* getSensorUnit()        { return sensorUnit; };
    float readSensor(int rawADC) { return slope * rawADC + intercept; };
};

// Stainless Steel Temperature sensor.
class SSTempSensor : public Vernier {
  private:
    const double K0 = 0.00102119;
    const double K1 = 0.000222468;
    const double K2 = 0.0000000876741;
    const int therm = 20000;

    int pad;

  public:
    SSTempSensor();
    // Return balance resistance.
    int getBalanceResistance()       { return pad; };
    // Set balance resistor value.
    void setBalanceResistance(int r) { pad = r; };
    //  Switch between Celcius and Fahrenheit, default is Celcius.
    void switchUnit();
    // Return the temperature in current unit.
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