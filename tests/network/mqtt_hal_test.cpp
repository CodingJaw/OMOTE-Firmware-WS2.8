#define ENABLE_WIFI_AND_MQTT 1
#define ENABLE_KEYBOARD_BLE 0
#include "WiFi.h"
#include <cassert>
#include <cstdio>
uint32_t fakeNow=0;
FakeWiFi WiFi;
FakeSerial Serial;
// Compile the actual production HAL against a synthetic event/transport layer.
#include "hardware/ESP32/mqtt_hal_esp32.cpp"
int main() {
  init_mqtt_HAL();
  assert(WiFi.begins==1 && !WiFi.automatic);
  for(int i=0;i<1000;++i) WiFiEvent(ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  assert(WiFi.begins==1 && WiFi.reconnects==0); // Callback must never reconfigure Wi-Fi.
  for(fakeNow=0;fakeNow<10000;++fakeNow) mqtt_loop_HAL();
  assert(WiFi.reconnects==0);
  mqtt_loop_HAL();assert(WiFi.reconnects==1);
  WiFiEvent(ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  mqtt_loop_HAL();assert(WiFi.reconnects==1); // Duplicate event cannot bypass delay.
  fakeNow=20000;mqtt_loop_HAL();assert(WiFi.reconnects==2);
  WiFi.up=true;WiFiEvent(ARDUINO_EVENT_WIFI_STA_GOT_IP);
  mqtt_loop_HAL();assert(mqttClient.attempts==1);
  for(int i=0;i<100;++i) {mqtt_loop_HAL();publishMQTTMessage_HAL("test","value");}
  assert(mqttClient.attempts==1); // Publish and background loop share the retry budget.
  fakeNow=30000;mqtt_loop_HAL();assert(mqttClient.attempts==2);
  wifi_shutdown_HAL();
  WiFiEvent(ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  fakeNow=60000;mqtt_loop_HAL();
  assert(WiFi.reconnects==2 && mqttClient.attempts==2); // Shutdown stays shut down.
  puts("Production WiFi/MQTT callback, retry and shutdown regression checks passed");
}
