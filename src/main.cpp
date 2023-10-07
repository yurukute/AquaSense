#include <Arduino.h>
#include <Artila-Matrix310.h>
#include <VernierLib32.h>
#include <R4AVA07.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <algorithm>
#include <functional>
#include <vector>

#define VIN_CH 0
#define TMP_CH 1
#define ODO_CH 2
#define FPH_CH 3

const float Rl = 5930.434783; // ADC resistance
const int sample_rate = 10;   // 10 samples per read
const int read_num    = 4;    // Number of voltage inputs

// WIFI
const char *wifi_ssid = "ANABAS";
const char *wifi_pass = "carodong@2023";

// MQTT Broker
const char *mqtt_broker   = "broker.emqx.io";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int   mqtt_port     = 1883;

std::vector<float> voltage_avg(read_num, 0);
R4AVA07 ADC;
WiFiClient matrix310;
PubSubClient client(matrix310);

// Sensor objects
SSTempSensor TMP;
ODOSensor    ODO;
FPHSensor    FPH;

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.printf("Message arrived in topic: %s\n", topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

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
    Serial.println("done.");

    Serial.print("Connecting to WiFi..");
    WiFi.begin(wifi_ssid, wifi_pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("done.");

    Serial.print("Connecting to MQTT broker...");
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    while (!client.connected()) {
        String id = (String) "matrix310-" + WiFi.macAddress();
        client.connect(id.c_str(), mqtt_username, mqtt_password);
        int state = client.state();
        if (state != 0) {
            Serial.printf("Failed with state (%d)\n", state);
            Serial.print("Retrying...");
        }
    }
    Serial.println("done.");
}

float readTemp() {
    float vin  = voltage_avg[VIN_CH];
    float vout = voltage_avg[TMP_CH];
    if (vout == 0.0) {
        return NAN;
    }
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
    float R1 = TMP.getDividerResistance();
    float Rp = 1/(vin/(R1*vout) - 1/Rl - 1/R1);
    return TMP.calculateTemp(Rp);
}

float readODO() {
    float vout = voltage_avg[ODO_CH];
    if (vout == 0.0) {
        return NAN;
    }
    return ODO.readSensor(vout);
}

float readFPH() {
    float vout = voltage_avg[FPH_CH];
    if (vout == 0.0) {
        return NAN;
    }
    return FPH.readSensor(vout);
}

void readVoltage() {
    std::fill(voltage_avg.begin(), voltage_avg.end(), 0);
    for (auto i = 0; i < sample_rate; i++) {
        std::transform(voltage_avg.begin(), voltage_avg.end(),
                       ADC.readVoltage(1, read_num).begin(),
                       voltage_avg.begin(), std::plus<float>());
    }
    // Calculate average
    for (auto &v : voltage_avg) {
        v /= sample_rate;
    }
}

void loop() {
    String msg;

    readVoltage();
    Serial.printf("Voltage read:\n\tCH1\tCH2\tCH3\tCH4\n");
    for (auto v : voltage_avg) {
        Serial.printf("\t%.2f", v);
    }
    Serial.println();

    msg = String(readTemp());
    client.publish("matrix310/vernier/fph-bta", msg.c_str());
    Serial.println("Temperature: " + msg);

    msg = String(readODO());
    client.publish("matrix310/vernier/odo-bta", msg.c_str());
    Serial.println("Dissolved oxygen: " + msg);

    msg = String(readFPH());
    client.publish("matrix310/vernier/fph-bta", msg.c_str());
    Serial.println("pH: " + msg);

    delay(500);
}