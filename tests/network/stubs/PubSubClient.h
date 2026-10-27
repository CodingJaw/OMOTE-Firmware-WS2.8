#pragma once
#include "WiFi.h"
struct PubSubClient {
  int attempts=0;
  bool up=false;
  explicit PubSubClient(WiFiClient&) {}
  void setBufferSize(unsigned) {}
  void setServer(const char*,int) {}
  void setSocketTimeout(unsigned) {}
  void setCallback(void(*)(char*,uint8_t*,unsigned)) {}
  bool subscribe(const char*) {return true;}
  bool connected() {return up;}
  bool connect(const char*,const char*,const char*) {++attempts;return false;}
  void loop() {}
  bool publish(const char*,const char*) {return up;}
  void disconnect() {up=false;}
};
