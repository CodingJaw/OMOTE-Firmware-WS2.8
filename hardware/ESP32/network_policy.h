#pragma once
#include <stdint.h>
#include <cstring>
namespace omote_network {
inline bool wifiConfigured(const char *ssid,const char *password) {
  return ssid && *ssid && std::strcmp(ssid,"YourWifiSSID")!=0 && password && std::strcmp(password,"YourWifiPassword")!=0;
}
inline bool brokerConfigured(const char *host) {
  return host && *host && std::strcmp(host,"IPAddressOfYourBroker")!=0;
}
// One budget shared by loop() and publish paths. Duplicate disconnect events
// cannot cause immediate retries; uint32_t subtraction also handles millis wrap.
class RetryGate {
  uint32_t interval,last=0;
  bool attempted=false;
public:
  explicit RetryGate(uint32_t milliseconds):interval(milliseconds) {}
  void mark(uint32_t now) {last=now;attempted=true;}
  bool take(uint32_t now) {
    if(attempted && uint32_t(now-last)<interval) return false;
    mark(now);return true;
  }
};
}
