#include <WiFi.h>
#include <OneWire.h>
#include <PubSubClient.h>
#include <DallasTemperature.h>


//informasi ubidots
const char *mqttServer = "industrial.api.ubidots.com";
const int mqttPort = 1883;
const char *mqttClientName = "ESP32"; // Nama unik untuk client
const char *UBIDOTS_TOKEN = "BBUS-q5OTEmcvhcpnA0Do4VA6ZGOA2dStUH";
const char *DEVICE_LABEL = "Serasi";

const char* ssid = "Arzula";
const char* password = "kolontong";

//PIN SENSOR
#define PH 35
#define TDS 32
#define Turbidity 34
#define Suhu 18
#define Trigger 4
#define Echo 5
#define debit_air 27
#define volcur 29

//sensor water flow
long currentMillis = 0;
long previousMills= 0;
int interval = 1000;
boolean leadState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulseSec = 0;
float flowRate;
unsigned int flowMiliLitres;
unsigned long totalMiliLitres;

WiFiClient espClient;
PubSubClient client(espClient);

//kalibrasi sensor PH
const float PH4 = 1.8;  // Tegangan untuk pH 4.0
const float PH7 = 2.86;  // Tegangan untuk pH 7.0
const float PH_STEP = (PH4 - PH7) / 3;

// DS18B20 sensor
OneWire oneWire(Suhu);
DallasTemperature DS18B20(&oneWire);

long duration;
int distance;

void setup_wifi()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  // Cek status koneksi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi Connected");

}

void reconnect() {
  // Reconnect jika koneksi ke MQTT terputus
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect(mqttClientName, UBIDOTS_TOKEN, "")) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void send_ubidots(const char *variable, float value) {
  char topic[150];
  char payload[150];

  sprintf(topic, "/v1.6/devices/%s", DEVICE_LABEL);
  sprintf(payload, "{\"%s\": %.2f}", variable, value);

  client.publish(topic, payload);
  Serial.print("Published: ");
  Serial.println(payload);
}

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

void send_ubidots(float temperature, float tds, float turbidity, float ph, float flowRate,float jarak)
{
   char payload[512];
  snprintf(payload, sizeof(payload),
           "{\"pH\": %.2f, \"TDS\": %.2f, \"Turbidity\": %.2f, \"Temperature\": %.2f, \"FlowRate\": %.2f, \"Distance\": %.2f}",
           ph, tds, turbidity, temperature, flowRate, jarak);

  char topic[128];
  snprintf(topic, sizeof(topic), "/v1.6/devices/%s", DEVICE_LABEL);

  client.publish(topic, payload);
  Serial.println("Data sent to Ubidots:");
  Serial.println(payload);
}
void setup() {
  Serial.begin(115200);
  setup_wifi();

  client.setServer(mqttServer, mqttPort);

  pinMode(PH, INPUT);
  pinMode(Turbidity, INPUT);
  pinMode(TDS, INPUT);
  pinMode(Suhu, INPUT);
  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);

  DS18B20.begin();
  attachInterrupt(digitalPinToInterrupt(debit_air), pulseCounter, FALLING);

  Serial.println("Setup Selesai");
  delay(1000);
}

void loop() {

    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    // DS18B20 sensor
    DS18B20.requestTemperatures();
    float temperature = DS18B20.getTempCByIndex(0);
    Serial.print("DS18B20 Temperature: ");
    Serial.print(temperature, 2);
    Serial.println("°C");

    // pH sensor
    int nilai_analog_PH = analogRead(PH);
    Serial.print("Nilai ADC pH: ");
    Serial.println(nilai_analog_PH);
    
    float voltage = nilai_analog_PH * (3.3 / 4095.0); // Konversi ke voltase
    Serial.print("Tegangan pH: ");
    Serial.println(voltage, 3);

    // Hitung pH dengan rumus berdasarkan kalibrasi
    float pH = 7.0 + ((voltage - PH7) / PH_STEP);
    Serial.print("pH: ");
    Serial.println(pH, 2);
    
    // Pastikan pH dalam rentang yang realistis
    if (pH < 0.0) pH = 0.0; // Minimum pH
    if (pH > 14.0) pH = 14.0; // Maksimum pH

    //Sensor TDS
    int tdsValue = analogRead(TDS);
    float tdsVoltage = (tdsValue * 3.3) / 4095.0; 
    float tdsPPM = (tdsVoltage * 133.42) / (1 + (tdsVoltage * 1.0094)); // Convert to ppm
    Serial.print("TDS PPM: ");
    Serial.println(tdsPPM, 2);

    // Sensor Turbidity
    int sensorValue = analogRead(Turbidity);
    float turbidity = map(sensorValue, 0, 4095, 0, 100); // Contoh konversi ke skala 0-100 NTU
    // Tampilkan nilai ke Serial Monitor
    Serial.print("Nilai Analog: ");
    Serial.print(sensorValue);
    Serial.print(" | Kekeruhan (NTU): ");
    Serial.println(turbidity);
    

    // Sensor Ultrasonik
  digitalWrite(Trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger, LOW);

  duration = pulseIn(Echo, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Jarak: ");
  Serial.print(distance);
  Serial.println(" cm");

    //sensor debit air
    currentMillis = millis();
    if (currentMillis - previousMills > interval)
    {
      pulseSec = pulseCount;
      pulseCount = 0;

      flowRate = ((1000.0 / (millis() - previousMills)) * pulseSec) / calibrationFactor;
      previousMills = millis();

      flowMiliLitres = (flowRate / 60) * 1000;

      totalMiliLitres += flowMiliLitres;

      Serial.print("Flow rate: ");
      Serial.print(int(flowRate)); 
      Serial.print("L/min");
      Serial.print("\t");      

      Serial.print("Output Liquid Quantity: ");
      Serial.print(totalMiliLitres);
      Serial.print("mL / ");
      Serial.print(totalMiliLitres / 1000);
      Serial.println("L");


    send_ubidots(temperature, tdsPPM, turbidity, pH, flowRate, distance);

    }

    delay(2000); 

}
