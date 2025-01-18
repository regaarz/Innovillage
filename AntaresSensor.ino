
#include <AntaresESPHTTP.h>

// Pin untuk sensor turbidity
#define Turbidity 34

// Variabel Antares
#define ACCESSKEY "1ae01381c63e2d54:93a2cacfa9a654c9"
#define projectName "Hydrats"
#define deviceName "monitoringBendungan"
#define ssid  "Arzula"
#define password  "kolontong"

// Inisialisasi objek Antares
AntaresESPHTTP antares(ACCESSKEY);

// Fungsi untuk menghubungkan ke WiFi
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  // Tunggu hingga terhubung
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Konfigurasi Antares
  antares.setDebug(true); // Menampilkan informasi debug
  antares.wifiConnection(ssid, password);
}

void setup() {
  Serial.begin(115200);
  setup_wifi(); // Hubungkan ke WiFi
  pinMode(Turbidity, INPUT); // Atur pin sensor turbidity sebagai input
}

void loop() {
  // Baca nilai analog dari sensor turbidity
  int sensorValue = analogRead(Turbidity);
  float turbidity = map(sensorValue, 0, 4095, 0, 100); // Konversi ke skala 0-100 NTU

  // Tampilkan nilai ke Serial Monitor
  Serial.print("Nilai Analog: ");
  Serial.print(sensorValue);
  Serial.print(" | Kekeruhan (NTU): ");
  Serial.println(turbidity);

  // Tambahkan data ke payload Antares
  antares.add("Kekeruhan", turbidity);
  antares.send(projectName, deviceName);


  delay(5000); // Delay 5 detik sebelum pengiriman berikutnya
}
