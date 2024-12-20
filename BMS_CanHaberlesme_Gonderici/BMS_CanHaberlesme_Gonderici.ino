#include <SPI.h>
#include <mcp2515.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

MCP2515 mcp2515(53); 
struct can_frame canMsg;

// Analog pin tanımlamaları
const int PIN_VOLTAGE_A0 = A0; // Pil 1 gerilim ölçümü
const int PIN_VOLTAGE_A1 = A1; // Pil 2 gerilim ölçümü
const int PIN_VOLTAGE_A2 = A2; // Pil 3 gerilim ölçümü
const int PIN_VOLTAGE_A3 = A3; // Pil 4 gerilim ölçümü
// Hücre gerilimleri ve sensörler
float cellVoltages[4] = {0};

// Akım ölçümü parametreleri
const int nSamples = 1000;
const float vcc = 3.7;
const int adcMax = 1023;
const float sens = 0.66;

// Akım ölçüm ve gönderimi
float avgCurrent() {
  float val = 0;
  for (int i = 0; i < nSamples; i++) {
    val += analogRead(A4);
    delay(1);
  }
  return val / adcMax / nSamples;
}

// SoC hesaplama fonksiyonu
float calculateSOC(float sumVoltage) {
  float soc = 0;

  if (sumVoltage >= 16.8) {
    soc = 100.0;
  } else if (sumVoltage <= 12.0) {
    soc = 0.0;
  } else {
    soc = ((sumVoltage - 12.0) / (16.8 - 12.0)) * 100.0;
  }

  return soc;
}
void setup() {
  Serial.begin(9600);
  SPI.begin();

  if (mcp2515.reset() != MCP2515::ERROR_OK) {
    Serial.println("CAN Modülü reset edilemedi!");
  }
  if (mcp2515.setBitrate(CAN_250KBPS, MCP_16MHZ) != MCP2515::ERROR_OK) {
    Serial.println("CAN Modülü hız ayarı başarısız!");
  }
  if (mcp2515.setNormalMode() != MCP2515::ERROR_OK) {
    Serial.println("CAN Modülü normal moda geçemedi!");
  }

  sensors.begin();
  Serial.println("Gönderici(BMS) Başlatıldı!");
}

// Hücre voltajlarını oku
float readSensorData() {
  cellVoltages[0] = (analogRead(PIN_VOLTAGE_A0) / 1023.0) * 5.0;
  cellVoltages[1] = (analogRead(PIN_VOLTAGE_A1) / 1023.0) * 5.0;
  cellVoltages[2] = (analogRead(PIN_VOLTAGE_A2) / 1023.0) * 5.0;
  cellVoltages[3] = (analogRead(PIN_VOLTAGE_A3) / 1023.0) * 5.0;

  float sumVoltage = 0;
  float minVoltage = cellVoltages[0];
  float maxVoltage = cellVoltages[0];

  for (int i = 0; i < 4; i++) {
    sumVoltage += cellVoltages[i];
    if (cellVoltages[i] < minVoltage) {
      minVoltage = cellVoltages[i];
    }
    if (cellVoltages[i] > maxVoltage) {
      maxVoltage = cellVoltages[i];
    }
  }
  // Toplam, min ve max voltaj bilgilerini yazdır
  Serial.print("Toplam pil voltajı: ");
  Serial.println(sumVoltage, 2);

  Serial.print("Minimum voltaj: ");
  Serial.println(minVoltage, 2);

  Serial.print("Maksimum voltaj: ");
  Serial.println(maxVoltage, 2);
  
  return sumVoltage;
}


// Hücre verilerini CAN ile gönder
void sendDataToAKS() {
  canMsg.can_id = 0x101;
  canMsg.can_dlc = 8;

  for (int i = 0; i < 4; i++) {
    uint16_t scaledVoltage = (uint16_t)(cellVoltages[i] * 100);
    canMsg.data[i * 2] = (scaledVoltage >> 8) & 0xFF;
    canMsg.data[i * 2 + 1] = scaledVoltage & 0xFF;
  }

  if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.println("BYS Verisi Gönderildi.");
  } else {
    Serial.println("BYS Verisi Gönderilemedi!");
  }
}

// Sıcaklık ölçüm ve gönderimi
void sendTemperature() {
  sensors.requestTemperatures();
  if (sensors.getTempCByIndex(0) == DEVICE_DISCONNECTED_C) {
    Serial.println("Sensör bağlantısı kesildi veya okunamadı!");
    return;
  }

  int scaledTemperature = sensors.getTempCByIndex(0) * 100;
  canMsg.can_id = 0x100;
  canMsg.can_dlc = 2;
  canMsg.data[0] = (scaledTemperature >> 8) & 0xFF;
  canMsg.data[1] = scaledTemperature & 0xFF;

  if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print("Sıcaklık Gönderildi: ");
    Serial.print(sensors.getTempCByIndex(0));
    Serial.println(" C");
  } else {
    Serial.println("Sıcaklık Gönderilemedi!");
  }
}

void sendCurrent() {
  float cur = (vcc / 2 - vcc * avgCurrent()) / sens;
  Serial.print("Current: ");
  Serial.println(cur);

  canMsg.can_id = 0x123;
  canMsg.can_dlc = 2;
  canMsg.data[0] = (int(cur * 1000) >> 8) & 0xFF;
  canMsg.data[1] = int(cur * 1000) & 0xFF;

  if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.println("Veri gönderildi!");
  } else {
    Serial.println("Veri gönderilemedi!");
  }
}
void sendSoC(float sumVoltage) {
  float soc = calculateSOC(sumVoltage); // SoC'yi hesapla

  // SoC'yi CAN mesajı olarak gönder
  canMsg.can_id = 0x200; // Örnek CAN ID
  canMsg.can_dlc = 2;    // Veri uzunluğu (2 byte)
  uint16_t scaledSOC = soc * 100; // SoC'yi %0-100 aralığından tam sayı (0-10000) yap
  canMsg.data[0] = (scaledSOC >> 8) & 0xFF; // Üst byte
  canMsg.data[1] = scaledSOC & 0xFF;        // Alt byte

  // Mesajı gönder ve sonucu kontrol et
  if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print("Gönderilen SoC: ");
    Serial.println(soc);
  } else {
    Serial.println("SoC verisi gönderilemedi!");
  }
}

void loop() {
  float sumVoltage = readSensorData();
  readSensorData();
  sendDataToAKS();
  sendTemperature();
  sendCurrent();
  sendSoC(sumVoltage);
  delay(2000);
}
