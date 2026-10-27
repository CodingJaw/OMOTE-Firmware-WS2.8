#include "hardware/ESP32/network_policy.h"
#include <cassert>
#include <cstdio>
int main() {
  using namespace omote_network;
  assert(!wifiConfigured("YourWifiSSID","YourWifiPassword"));
  assert(!wifiConfigured("ExampleAP","YourWifiPassword"));
  assert(!wifiConfigured("","configured"));
  assert(!wifiConfigured(nullptr,"configured"));
  assert(wifiConfigured("ExampleAP","configured"));
  assert(wifiConfigured("OpenAP","")); // Open networks are legitimate.
  assert(!brokerConfigured("IPAddressOfYourBroker"));
  assert(!brokerConfigured("")); assert(!brokerConfigured(nullptr));
  assert(brokerConfigured("192.0.2.1"));
  RetryGate retries(10000);
  assert(retries.take(100));
  for(uint32_t now=101;now<10100;++now) assert(!retries.take(now));
  assert(retries.take(10100));
  assert(!retries.take(10100)); // Duplicate callback/publish gets no second attempt.
  retries.mark(0xfffffff0u);
  assert(!retries.take(10));
  assert(retries.take(uint32_t(0xfffffff0u+10000u)));
  puts("Network placeholder, retry throttle and rollover checks passed");
}
