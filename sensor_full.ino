#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AntaresESPHTTP.h>

#define PH 35
#define TDS 32
#define Turbidity 34
#define Suhu 18
#define Trigger 4
#define Echo 5
#define debit_air 27
#define VoltageSensor 36

const float maxVoltage = 3.3;           // Tegangan maksimum ADC ESP32
const int adcResolution = 4095;         // Resolusi ADC ESP32 (12-bit)
const float R1 = 30000;
const float R2 = 7500;

// Variabel Antares
#define ACCESSKEY "1ae01381c63e2d54:93a2cacfa9a654c9"
#define projectName "Hydrats"
#define deviceName "monitoringBendungan"

// Inisialisasi objek Antares
AntaresESPHTTP antares(ACCESSKEY);

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

const char* ssid = "arzula";
const char* password = "12345678";

WiFiClient espClient;
PubSubClient client(espClient);

// Variabel Sensor
long duration;
int distance;
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

// Kalibrasi pH
const float PH4 = 1.8;
const float PH7 = 2.86;
const float PH_STEP = (PH4 - PH7) / 3;

// DS18B20 untuk suhu
OneWire oneWire(Suhu);
DallasTemperature DS18B20(&oneWire);

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT Broker.");
    } else {
      Serial.print("Failed to connect. State=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected.");
}

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Pin konfigurasi
  pinMode(PH, INPUT);
  pinMode(TDS, INPUT);
  pinMode(Turbidity, INPUT);
  pinMode(Suhu, INPUT);
  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);
  pinMode(debit_air, INPUT_PULLUP);
  pinMode(VoltageSensor, INPUT);


  // DS18B20 dan sensor debit air
  DS18B20.begin();
  attachInterrupt(digitalPinToInterrupt(debit_air), pulseCounter, FALLING);
}

void publishSensorData(const char* topic, String value) {
  client.publish(topic, value.c_str());
  Serial.print("Published to ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(value);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Sensor DS18B20
  DS18B20.requestTemperatures();
  float temperature = DS18B20.getTempCByIndex(0);
  publishSensorData("esp32/hydrats/innovillage/water_temperature", String(temperature, 2));

  // Sensor pH
  int phValue = analogRead(PH);
  float phvoltage = phValue * (3.3 / 4095.0);
  float pH = 7.0 + ((phvoltage - PH7) / PH_STEP);
  pH = constrain(pH, 0.0, 14.0); // Batas nilai pH
  publishSensorData("esp32/hydrats/innovillage/ph", String(pH, 2));

  // Sensor TDS
  int tdsValue = analogRead(TDS);
  float tdsVoltage = (tdsValue * 3.3) / 4095.0;
  float tdsPPM = (tdsVoltage * 133.42) / (1 + (tdsVoltage * 1.0094));
  publishSensorData("esp32/hydrats/innovillage/tds", String(tdsPPM, 2));

  // Sensor Turbidity
  int turbidityValue = analogRead(Turbidity);
  float turbidity = map(turbidityValue, 0, 4095, 0, 100);
  publishSensorData("esp32/hydrats/innovillage/turbidity", String(turbidity, 2));

  // Sensor Ultrasonik
  digitalWrite(Trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger, LOW);
  duration = pulseIn(Echo, HIGH);
  distance = duration * 0.034 / 2;
  publishSensorData("esp32/hydrats/innovillage/distance", String(distance));

  // Sensor Debit Air
  flowRate = (1000.0 / (millis() - pulseCount)) * pulseCount / 4.5;
  pulseCount = 0;
  publishSensorData("esp32/hydrats/innovillage/water_flow", String(flowRate, 2));

  // Penentuan kualitas air
  String kualitasAir;
  if (turbidity >= 0 && turbidity <= 5) {
    kualitasAir = "Air sangat jernih (air minum)";
  } else if (turbidity > 5 && turbidity <= 50) {
    kualitasAir = "Air cukup jernih (air sungai bersih)";
  } else if (turbidity > 50 && turbidity <= 200) {
    kualitasAir = "Air sedang keruh (air limbah ringan)";
  } else if (turbidity > 200 && turbidity <= 1000) {
    kualitasAir = "Air sangat keruh (air limbah industri atau air sungai berlumpur)";
  } else {
    kualitasAir = "Air dengan tingkat kekeruhan sangat tinggi (lumpur atau cairan pekat)";
  }
  publishSensorData("esp32/hydrats/innovillage/water_quality", kualitasAir);


    // Sensor Tegangan
  int voltageValue = analogRead(VoltageSensor); // Membaca nilai ADC
  float tegangan = (voltageValue * maxVoltage / adcResolution) ;
  float voltage = tegangan / (R2/(R1+R2));
  publishSensorData("esp32/hydrats/innovillage/voltage", String(voltage, 2));


  antares.add("Kekeruhan", String(turbidity, 2));
  antares.add("Jarak", String(distance, 2));
  antares.add("Kualitas Air", kualitasAir); // Tetap string
  antares.add("Suhu", String(temperature, 2));
  antares.add("PH", String(pH, 2));
  antares.add("Debit air", String(flowRate, 2));
  antares.add("Kandungan Air", String(tdsPPM, 2));
  antares.add("Tegangan", String(voltage, 2));
 

  antares.send(projectName, deviceName);

  delay(2000); // Delay 2 detik
}
