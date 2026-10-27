// Standalone LCD bring-up: no OMOTE, LVGL, BLE, touch, ADC or PWM initialization.
#include <Arduino.h>
#include <SPI.h>
#include <driver/gpio.h>
#include <initializer_list>
#include "../waveshare/board.h"

namespace {
SPIClass lcd(FSPI);
constexpr uint32_t clockHz=20000000;
void command(uint8_t reg,std::initializer_list<uint8_t> data={}) {
  lcd.beginTransaction(SPISettings(clockHz,MSBFIRST,SPI_MODE0));
  digitalWrite(waveshare::lcdCs,LOW); digitalWrite(waveshare::lcdDc,LOW);
  lcd.transfer(reg); digitalWrite(waveshare::lcdDc,HIGH);
  for(uint8_t value:data) lcd.transfer(value);
  digitalWrite(waveshare::lcdCs,HIGH); lcd.endTransaction();
}
void initializePanel() {
  digitalWrite(waveshare::lcdReset,LOW); delay(50);
  digitalWrite(waveshare::lcdReset,HIGH); delay(170);
  // Values from Waveshare Display_ST7789.cpp. RGB565 pixels below are sent
  // high byte first, so clear RAMCTRL ENDIAN (bit 3): E8 -> E0.
  command(0x11); delay(120);
  command(0x36,{0x00}); command(0x3a,{0x05}); command(0xb0,{0x00,0xe0});
  command(0xb2,{0x0c,0x0c,0x00,0x33,0x33}); command(0xb7,{0x75});
  command(0xbb,{0x1a}); command(0xc0,{0x2c}); command(0xc2,{0x01,0xff});
  command(0xc3,{0x13}); command(0xc4,{0x20}); command(0xc6,{0x0f});
  command(0xd0,{0xa4,0xa1}); command(0xd6,{0xa1});
  command(0xe0,{0xd0,0x0d,0x14,0x0d,0x0d,0x09,0x38,0x44,0x4e,0x3a,0x17,0x18,0x2f,0x30});
  command(0xe1,{0xd0,0x09,0x0f,0x08,0x07,0x14,0x37,0x44,0x4d,0x38,0x15,0x16,0x2c,0x2e});
  command(0x21); command(0x29); delay(120);
}
void colorBars() {
  command(0x2a,{0,0,0,239}); command(0x2b,{0,0,1,63}); command(0x2c);
  constexpr uint16_t colors[]={0xf800,0x07e0,0x001f,0xffff};
  uint8_t row[480];
  lcd.beginTransaction(SPISettings(clockHz,MSBFIRST,SPI_MODE0));
  digitalWrite(waveshare::lcdCs,LOW); digitalWrite(waveshare::lcdDc,HIGH);
  for(int y=0;y<320;++y) {
    uint16_t color=colors[y/80];
    for(int x=0;x<240;++x) {row[x*2]=color>>8;row[x*2+1]=color&255;}
    lcd.writeBytes(row,sizeof(row));
  }
  digitalWrite(waveshare::lcdCs,HIGH); lcd.endTransaction();
}
}
void setup() {
  gpio_set_level(gpio_num_t(waveshare::powerLatch),1);
  pinMode(waveshare::powerLatch,OUTPUT);digitalWrite(waveshare::powerLatch,HIGH);
  // Digital output bypasses LEDC entirely. A working backlight should glow
  // even when the LCD SPI bus/controller does not respond.
  pinMode(waveshare::backlight,OUTPUT);digitalWrite(waveshare::backlight,HIGH);
  Serial.begin(115200);
  uint32_t started=millis();while(!Serial && millis()-started<2000) delay(10);
  Serial.println("WAVESHARE DISPLAY TEST: backlight GPIO5 HIGH; no BLE/LVGL/PWM");
  pinMode(waveshare::lcdCs,OUTPUT);digitalWrite(waveshare::lcdCs,HIGH);
  pinMode(waveshare::lcdDc,OUTPUT);pinMode(waveshare::lcdReset,OUTPUT);
  lcd.begin(waveshare::lcdSck,-1,waveshare::lcdMosi,waveshare::lcdCs);
  initializePanel();colorBars();
  Serial.println("Expected: horizontal RED, GREEN, BLUE, WHITE bands, top to bottom");
}
void loop() {
  delay(1000);
  Serial.printf("DISPLAY TEST running: uptime=%lu ms; backlight=%d\n",millis(),digitalRead(waveshare::backlight));
}
