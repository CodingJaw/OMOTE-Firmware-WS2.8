#pragma once
#include <stdint.h>
#include <string>
using byte=uint8_t;
extern uint32_t fakeNow;
inline uint32_t millis() {return fakeNow;}
struct FakeSerial {
  template<class... Args> void printf(const char*,Args...) {}
  void println(const char*) {}
};
extern FakeSerial Serial;
enum WiFiEvent_t {ARDUINO_EVENT_WIFI_STA_GOT_IP,ARDUINO_EVENT_WIFI_STA_GOT_IP6,ARDUINO_EVENT_WIFI_STA_DISCONNECTED,ARDUINO_EVENT_WIFI_STA_LOST_IP};
constexpr int WIFI_STA=1,WIFI_OFF=0;
struct FakeIP {std::string toString() {return "192.0.2.2";}};
struct FakeWiFi {
  int begins=0,reconnects=0;
  bool up=false,automatic=true;
  void onEvent(void(*)(WiFiEvent_t)) {}
  void setAutoReconnect(bool value) {automatic=value;}
  void mode(int) {}
  void setHostname(const char*) {}
  void begin(const char*,const char*) {++begins;}
  void setSleep(bool) {}
  bool isConnected() {return up;}
  bool reconnect() {++reconnects;return true;}
  void disconnect() {up=false;}
  FakeIP localIP() {return {};}
  std::string macAddress() {return "00:00:00:00:00:00";}
};
extern FakeWiFi WiFi;
struct WiFiClient {void setConnectionTimeout(uint32_t) {}};
