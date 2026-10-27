#include "board.h"
#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>
#include <esp_heap_caps.h>
#include "../ESP32/displaydriver.h"
#include "../ESP32/sleep_hal_esp32.h"

namespace {
class Display : public lgfx::LGFX_Device {
  lgfx::Bus_SPI bus;
  lgfx::Panel_ST7789 panel;
public:
  Display() {
    auto b=bus.config();
    b.spi_host=SPI2_HOST; b.spi_mode=0; b.freq_write=40000000;
    b.pin_sclk=waveshare::lcdSck; b.pin_mosi=waveshare::lcdMosi;
    b.pin_miso=-1; b.pin_dc=waveshare::lcdDc; b.dma_channel=SPI_DMA_CH_AUTO;
    bus.config(b); panel.setBus(&bus);
    auto p=panel.config();
    p.pin_cs=waveshare::lcdCs; p.pin_rst=waveshare::lcdReset;
    p.memory_width=240; p.memory_height=320; p.panel_width=240; p.panel_height=320;
    p.offset_x=0; p.offset_y=0; p.offset_rotation=0;
    p.invert=true; p.rgb_order=false; p.readable=false;
    panel.config(p); setPanel(&panel);
  }
} display;
volatile bool touchPending=true;
bool pressed=false;
uint16_t lastX=0,lastY=0;
void IRAM_ATTR touchInterrupt() { touchPending=true; }
bool touchWrite(uint16_t reg, const uint8_t *data=nullptr, size_t n=0) {
  Wire1.beginTransmission(waveshare::touchAddress);
  Wire1.write(uint8_t(reg>>8)); Wire1.write(uint8_t(reg));
  if(n) Wire1.write(data,n);
  return Wire1.endTransmission()==0;
}
bool touchRead(uint16_t reg,uint8_t *data,size_t n) {
  if(!touchWrite(reg)) return false;
  if(Wire1.requestFrom(waveshare::touchAddress,n)!=n) return false;
  for(size_t i=0;i<n;++i) data[i]=Wire1.read();
  return true;
}
}

lv_disp_draw_buf_t draw_buf;
bool lvgl_flush_async=false;
bool TouchInitSuccessful=false;
void setup_tft() {
  Serial.println("Waveshare display: configuring backlight GPIO5");
  if(!ledcAttach(waveshare::backlight,20000,8)) {
    Serial.println("ERROR: backlight PWM attach failed");
    abort();
  }
  ledcWrite(waveshare::backlight,0);
}
void setup_wire() { Wire.begin(waveshare::sda,waveshare::scl,400000); Wire.setTimeOut(50); }
void init_tft() {
  Serial.printf("Waveshare display: initializing ST7789, SCK=%d MOSI=%d CS=%d DC=%d RST=%d\n",
                waveshare::lcdSck,waveshare::lcdMosi,waveshare::lcdCs,waveshare::lcdDc,waveshare::lcdReset);
  if(!display.init()) {
    Serial.println("ERROR: LovyanGFX display initialization failed");
    ledcWrite(waveshare::backlight,128);
    abort();
  }
  display.setRotation(0); display.fillScreen(TFT_BLACK);
  // Keep the first rendered screen visible even if later BLE/network startup
  // blocks before loop() can update brightness. GPIO5 is active high.
  ledcWrite(waveshare::backlight,128);
  Serial.println("Waveshare display: SPI driver initialized (no panel readback); backlight at 50%");
  // A default focus group also permits button navigation on non-touch units.
  lv_group_set_default(lv_group_create());
  Wire1.begin(waveshare::touchSda,waveshare::touchScl,400000); Wire1.setTimeOut(50);
  pinMode(waveshare::touchInterrupt,INPUT);
  pinMode(waveshare::touchReset,OUTPUT);
  digitalWrite(waveshare::touchReset,HIGH); delay(50);
  digitalWrite(waveshare::touchReset,LOW); delay(5);
  digitalWrite(waveshare::touchReset,HIGH); delay(50);
  attachInterrupt(waveshare::touchInterrupt,touchInterrupt,RISING);
  TouchInitSuccessful=touchChipResponds_display_specific();
  Serial.printf("Waveshare display setup complete; CST328 %s\n",TouchInitSuccessful?"ready":"absent (use BOOT/PWR navigation)");
}
void init_lvgl_buffer() {
  constexpr size_t pixels=240*32;
  auto *buf=static_cast<lv_color_t*>(heap_caps_malloc(pixels*sizeof(lv_color_t),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!buf) buf=static_cast<lv_color_t*>(heap_caps_malloc(pixels*sizeof(lv_color_t),MALLOC_CAP_8BIT));
  if(!buf) { Serial.println("ERROR: LVGL draw-buffer allocation failed"); abort(); }
  Serial.printf("Waveshare display: LVGL draw buffer ready (%u bytes)\n",unsigned(pixels*sizeof(lv_color_t)));
  lv_disp_draw_buf_init(&draw_buf,buf,nullptr,pixels);
}
void my_disp_flush(lv_disp_drv_t *drv,const lv_area_t *area,lv_color_t *pixels) {
  display.startWrite();
  display.setAddrWindow(area->x1,area->y1,area->x2-area->x1+1,area->y2-area->y1+1);
  display.pushPixels(reinterpret_cast<uint16_t*>(pixels),(area->x2-area->x1+1)*(area->y2-area->y1+1),LV_COLOR_16_SWAP==0);
  display.endWrite(); lv_disp_flush_ready(drv);
}
bool touchChipResponds_display_specific() {
  uint8_t id[2]={};
  if(!touchWrite(0xd101)) return false;
  bool valid=touchRead(0xd1fe,id,2) && id[0]==0xca && id[1]==0xca;
  return touchWrite(0xd109) && valid;
}
bool touchChipResponds() { return touchChipResponds_display_specific(); }
void my_touchpad_read_display_specific(lv_indev_drv_t*,lv_indev_data_t *data) {
  noInterrupts(); bool pending=touchPending; touchPending=false; interrupts();
  if(pending) {
    uint8_t count=0,frame[5]={},clear=0;
    bool ok=touchRead(0xd005,&count,1);
    if(ok && (count&15)) ok=touchRead(0xd000,frame,sizeof(frame));
    bool ack=touchWrite(0xd005,&clear,1);
    pressed=ok && ack && waveshare::decodeTouch(frame,count&15,lastX,lastY);
  }
  data->point.x=lastX; data->point.y=lastY;
  data->state=pressed?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
}
void draw_touchpoint(uint16_t x,uint16_t y) { display.fillCircle(x,y,3,TFT_RED); }
