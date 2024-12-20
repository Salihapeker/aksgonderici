#include <mcp2515.h>
#include <SPI.h>

MCP2515 mcp2515(53);
struct can_frame canMsg;

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

  Serial.println("Başlatıldı! Voltaj alimi ve izolasyon direnci gonderimi gerceklesiyor...");
}

void loop() {
 static bool direnc_gonderimi =true;

  if (direnc_gonderimi) {
    // AKS'den gelen voltaj mesajını al
    int isolationResistance = 10;
    canMsg.can_id == 0x123;
    canMsg.can_dlc = 2;
    canMsg.data[0] = isolationResistance & 0xFF;        
    canMsg.data[1] = (isolationResistance >> 8) & 0xFF; 

      if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
        Serial.println("Direnç değeri gönderildi.");
        Serial.println(isolationResistance);
      } else {
        Serial.println("Direnç değeri gönderilemedi!");
      }
  
    direnc_gonderimi = false;
  }
  else {
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      if (canMsg.can_id == 0x100) { 
        int voltage = canMsg.data[0];  
        Serial.print("Alındı! Voltaj Değeri: ");
        Serial.println(voltage);
      }
    }

    direnc_gonderimi = true;
  }
  delay(1000);
}
