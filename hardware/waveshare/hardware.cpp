#include "board.h"
#include <Arduino.h>
#include <Wire.h>
#include <SD_MMC.h>
#include <driver/gpio.h>
#include <lvgl.h>
#include "../ESP32/include_hal_esp32.h"

int DEFAULT_MOTION_THRESHOLD=80;
int DEFAULT_SLEEP_TIMEOUT=60000;
Wakeup_reasons wakeup_reason=WAKEUP_BY_RESET;
namespace {
uint32_t lastActivity=0,sleepTimeout=60000;
uint8_t brightness=180,motionThreshold=80;
bool motionWake=true,poweredOff=false,powerReleased=false;
uint8_t imuAddress=0;
int previousAccel[3]={};
bool accelValid=false;
uint32_t lastAccelRead=0;
waveshare::DebouncedButton bootButton,powerButton;
bool powerDown() { return powerButton.update(digitalRead(waveshare::powerKey)==LOW,millis()); }

bool readImu(uint8_t reg,uint8_t *data,size_t n) {
  Wire.beginTransmission(imuAddress); Wire.write(reg);
  if(Wire.endTransmission(false)!=0 || Wire.requestFrom(imuAddress,n)!=n) return false;
  for(size_t i=0;i<n;++i) data[i]=Wire.read();
  return true;
}
bool writeImu(uint8_t reg,uint8_t value) {
  Wire.beginTransmission(imuAddress); Wire.write(reg); Wire.write(value);
  return Wire.endTransmission()==0;
}
void encoderRead(lv_indev_drv_t*,lv_indev_data_t *data) {
  static bool bootDown=false;
  bool boot=bootButton.update(digitalRead(waveshare::bootKey)==LOW,millis());
  data->enc_diff=(boot && !bootDown)?1:0;
  bootDown=boot;
  // Emit Enter only after a short release. Holding PWR to shut down must not
  // activate or repeatedly send the focused remote control's command.
  static bool wasDown=false;
  static uint32_t pressedAt=0;
  bool down=powerDown();
  if(down && !wasDown) pressedAt=millis();
  bool click=powerReleased && !down && wasDown && millis()-pressedAt<1000;
  wasDown=powerReleased && down;
  data->state=click?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
  if(boot || down || click) setLastActivityTimestamp_HAL();
}
}
void init_hardware_general_HAL() {
  // Arduino 3 digitalWrite ignores pins not yet registered as GPIO. Preload
  // the hardware output latch via IDF, then enable output without a LOW pulse.
  gpio_set_level(gpio_num_t(waveshare::powerLatch),1);
  pinMode(waveshare::powerLatch,OUTPUT);
  digitalWrite(waveshare::powerLatch,HIGH);
  pinMode(waveshare::powerKey,INPUT_PULLUP); pinMode(waveshare::bootKey,INPUT_PULLUP);
  bootButton.reset(digitalRead(waveshare::bootKey)==LOW,millis());
  powerButton.reset(digitalRead(waveshare::powerKey)==LOW,millis());
  pinMode(waveshare::backlight,OUTPUT); digitalWrite(waveshare::backlight,LOW);
  setCpuFrequencyMhz(240);
}
void init_userled_HAL() {} // GPIO45 is LCD MOSI, not the OMOTE status LED.
void update_userled_HAL() {}
void init_battery_HAL() {
  analogReadResolution(12); analogSetPinAttenuation(waveshare::batteryAdc,ADC_11db);
}
void get_battery_status_HAL(int *mv,int *percent,bool *charging) {
  *mv=waveshare::batteryMillivolts(analogReadMilliVolts(waveshare::batteryAdc));
  *percent=waveshare::batteryPercent(*mv);
  *charging=false; // The charger indicator is not exposed as readable telemetry.
}
void init_sleep_HAL() { lastActivity=millis(); }
void setLastActivityTimestamp_HAL() { lastActivity=millis(); }
uint32_t get_lastActivityTimestamp() { return lastActivity; }
uint32_t get_sleepTimeout_HAL() { return sleepTimeout; }
void set_sleepTimeout_HAL(uint32_t value) { sleepTimeout=value?value:DEFAULT_SLEEP_TIMEOUT; }
bool get_wakeupByIMUEnabled_HAL() { return motionWake; }
void set_wakeupByIMUEnabled_HAL(bool value) { motionWake=value; }
uint8_t get_motionThreshold_HAL() { return motionThreshold; }
void set_motionThreshold_HAL(uint8_t value) { motionThreshold=value?value:DEFAULT_MOTION_THRESHOLD; }
void init_IMU_HAL() {
  for(uint8_t address:{uint8_t(0x6b),uint8_t(0x6a)}) {
    imuAddress=address; uint8_t id=0;
    if(readImu(0,&id,1) && id==0x05) {
      // QMI8658 CTRL1 auto-increment; 4 g / 30 Hz accel, gyro off.
      if(writeImu(0x08,0) && writeImu(0x02,0x40) && writeImu(0x03,0x18) && writeImu(0x08,0x01)) {
        Serial.printf("QMI8658 ready at 0x%02x\n",imuAddress); return;
      }
    }
  }
  imuAddress=0; Serial.println("QMI8658 not found; motion wake unavailable");
}
void check_activity_HAL() {
  if(poweredOff) return;
  // Polling standby keeps touch, motion and BLE alive; no unverified deep-sleep IRQ setup.
  if(imuAddress && motionWake && millis()-lastAccelRead>=35) {
    lastAccelRead=millis(); uint8_t sample[6]={};
    if(readImu(0x35,sample,sizeof(sample))) {
      int motion=0;
      for(int i=0;i<3;++i) {
        int mg=int(int16_t(uint16_t(sample[2*i])|(uint16_t(sample[2*i+1])<<8)))*1000/8192;
        motion+=abs(mg-previousAccel[i]); previousAccel[i]=mg;
      }
      if(accelValid && motion>motionThreshold) setLastActivityTimestamp_HAL();
      accelValid=true;
    }
  }
  static uint32_t powerDownAt=0;
  bool down=powerDown();
  if(!down) { powerReleased=true; powerDownAt=0; }
  else if(powerReleased) {
    setLastActivityTimestamp_HAL();
    if(!powerDownAt) powerDownAt=millis();
    if(millis()-powerDownAt>=3000) {
      save_preferences_HAL(); poweredOff=true;
      ledcWrite(waveshare::backlight,0); digitalWrite(waveshare::powerLatch,LOW);
      Serial.println("Battery power released (USB power may remain)");
    }
  }
}
uint8_t get_backlightBrightness_HAL() { return brightness; }
void set_backlightBrightness_HAL(uint8_t value) { brightness=value; }
void update_backlightBrightness_HAL() {
  uint32_t idle=millis()-lastActivity;
  static uint32_t fadeStart=millis();
  uint32_t elapsed=millis()-fadeStart;
  uint32_t fade=elapsed<brightness?elapsed:brightness;
  uint32_t duty=poweredOff || idle>=sleepTimeout?0:idle>=sleepTimeout-(sleepTimeout>2000?2000:0)?brightness*3/10:fade;
  ledcWrite(waveshare::backlight,duty);
}
void init_keys_HAL() {
  static lv_indev_drv_t drv;
  lv_indev_drv_init(&drv); drv.type=LV_INDEV_TYPE_ENCODER; drv.read_cb=encoderRead;
  lv_indev_set_group(lv_indev_drv_register(&drv),lv_group_get_default());
}
void keys_getKeys_HAL(void*,unsigned long) {} // No OMOTE matrix/TCA8418 on this board.
void update_keyboardBrightness_HAL() {}
uint8_t get_keyboardBrightness_HAL() { return 0; }
void set_keyboardBrightness_HAL(uint8_t) {}
void init_SD_HAL() {
  pinMode(waveshare::sdD3,OUTPUT); digitalWrite(waveshare::sdD3,HIGH);
  if(!SD_MMC.setPins(waveshare::sdClock,waveshare::sdCommand,waveshare::sdData)) {
    Serial.println("SD pin setup failed"); return;
  }
  if(!SD_MMC.begin("/sdcard",true,false)) Serial.println("SD card not mounted (optional)");
}
// The stock board has no infrared transmitter or receiver. Never drive OMOTE IR pins.
void init_infraredSender_HAL() { Serial.println("Waveshare: no onboard IR; IR commands need external hardware support"); }
void sendIRcode_HAL(int,std::list<std::string>,std::string) { Serial.println("IR command unavailable: no configured IR transmitter"); }
void start_infraredReceiver_HAL() {}
void shutdown_infraredReceiver_HAL() {}
void infraredReceiver_loop_HAL() {}
bool get_irReceiverEnabled_HAL() { return false; }
void set_irReceiverEnabled_HAL(bool) {}
void set_announceNewIRmessage_cb_HAL(tAnnounceNewIRmessage_cb) {}
