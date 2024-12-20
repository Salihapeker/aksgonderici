#include <SPI.h>
#include <mcp2515.h>

#define BUTTON_PIN 38

struct can_frame canMsg;
MCP2515 mcp2515(53);

void setup(){
  pinMode(BUTTON_PIN, INPUT_PULLUP);

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
  Serial.println("Buton durumu gonderiliyor...");
  }

void loop(){
  int buttonState = digitalRead(BUTTON_PIN);
  canMsg.can_id = 0x100;
  canMsg.can_dlc =1;
  canMsg.data[0] = buttonState;

  if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK){
  Serial.print("Gonderildi! Buton Durumu: ");
  Serial.println(buttonState == LOW ? "0" : "1");
  }
  else{
  Serial.println("Mesaj gonderilemedi!");
  }
  delay(1000);
}