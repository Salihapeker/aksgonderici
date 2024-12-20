#include <mcp2515.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <DS1302.h>

#define SENSOR_PIN 2  
volatile int pulseSayac = 0;  
unsigned long lastTime = 0;
int tekerlekCevresi = 1.8;
int hiz = 0.0;

#define DS1302_CLK 6
#define DS1302_DAT 7
#define DS1302_RST 8
DS1302 rtc(DS1302_RST, DS1302_DAT, DS1302_CLK);


MCP2515 mcp2515(10);  
struct can_frame canMsg;

void sayacPulse() {
  pulseSayac++;  
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

  Serial.println("Gonderici baslatildi...");

  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), sayacPulse, FALLING); 
   rtc.halt(false);  // RTC modülünü çalıştırır
  rtc.writeProtect(false);  // RTC modülündeki yazma korumasını kapatır
 Time t(2024, 12, 20, 12, 0, 0, 4);  // Thursday = 4
  rtc.time(t);
 
  
}

void loop() {
 unsigned long currentTime = millis();
  if (currentTime - lastTime >= 1000) {  
   hiz = (pulseSayac * tekerlekCevresi * 60) / 100;
    Serial.print("Motor Hizi: ");
    Serial.print(hiz);
    Serial.println(" km/h");
    pulseSayac = 0;   
    lastTime = currentTime;  
  }

  Time t = rtc.time();
  uint8_t saat = t.hr;
  uint8_t dakika = t.min;
  uint8_t saniye = t.sec;
  
    canMsg.can_id = 0x100;
    canMsg.can_dlc = 5;
    canMsg.data[0] = (int)hiz >> 8;
    canMsg.data[1] = (int)hiz & 0xFF;
    canMsg.data[2] = saat;
    canMsg.data[3] = dakika;
    canMsg.data[4] = saniye;

    mcp2515.sendMessage(&canMsg);
    Serial.print("Gonderilen Hiz: ");
    Serial.print(hiz);
    Serial.print(" km/h, Saat: ");
    Serial.print(saat);
    Serial.print(":");
    Serial.print(dakika);
    Serial.print(":");
    Serial.println(saniye);
  
}
