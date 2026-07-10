#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <U8g2lib.h>

// Heltec V3 LoRa Pin Definitions
#define SS      8
#define RST     12
#define DIO1    14
#define BUSY    13

// Custom Component Pins
#define VIB_MOTOR_PIN 1   // Connect haptic motor here
#define RX_LED_PIN    2   // Connect status LED here

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 21, /* clock=*/ 18, /* data=*/ 17);

void setup() {
  pinMode(VIB_MOTOR_PIN, OUTPUT);
  pinMode(RX_LED_PIN, OUTPUT);
  
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  
  SPI.begin(9, 11, 10, 8);
  LoRa.setPins(SS, RST, DIO1);
  
  if (!LoRa.begin(915E6)) {
    u8g2.drawStr(0, 12, "LoRa Init Failed!");
    u8g2.sendBuffer();
    while (1);
  }
  
  LoRa.setSpreadingFactor(10);
  
  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "Receiver Listening...");
  u8g2.drawStr(0, 26, "Status: Safe");
  u8g2.sendBuffer();
  
  // Flash status LED once to indicate clear boot
  digitalWrite(RX_LED_PIN, HIGH);
  delay(500);
  digitalWrite(RX_LED_PIN, LOW);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  // constantly checks the radio chip's incoming buffer
  // if no radio wave has been received, it == 0
  // if packet is detected it parses the packet and returns the length of the message in bytes
  
  if (packetSize) {
    String incomingMsg = "";
    while (LoRa.available()) {
      // loop runs as long as there are unread text bytes waiting in the radio chip
      incomingMsg += (char)LoRa.read();
      // LoRa.read pulls one character byte out, converts it to char, and appends it to incomingMsg
    }
    
    // Verify security token key matches the transmitter
    if (incomingMsg == "EF_PIRATE_ALARM_TRIGGERED") {
      u8g2.clearBuffer();
      u8g2.drawStr(0, 12, "!!! PIRATE ALERT !!!");
      u8g2.drawStr(0, 26, "CHECK CAMP IMMEDIATELY");
      u8g2.sendBuffer();
      
      // Vibration  & flash loop, occurs 6 times
      for (int i = 0; i < 6; i++) {
        digitalWrite(VIB_MOTOR_PIN, HIGH); // pushes 3.3 V to coin motor, causing it to spin at a maximum RPM
        digitalWrite(RX_LED_PIN, HIGH); // turns LED on
        delay(300); // holds motor spining and LED glow for 300 ms
        digitalWrite(VIB_MOTOR_PIN, LOW); // cuts voltage
        digitalWrite(RX_LED_PIN, LOW); // turns off light
        delay(150); // stops everything for a clean break
      }
      
      delay(2000);
      
      u8g2.clearBuffer();
      u8g2.drawStr(0, 12, "Receiver Listening...");
      u8g2.drawStr(0, 26, "Status: Alert Sent");
      u8g2.sendBuffer();
    }
  }
}