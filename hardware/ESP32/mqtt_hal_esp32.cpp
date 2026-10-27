#include <sstream>
#include <atomic>
#include "network_policy.h"
#include "WiFi.h"
#include <PubSubClient.h>
#include "mqtt_hal_esp32.h"
#if (ENABLE_KEYBOARD_BLE == 1)
#include "keyboard_ble_hal_esp32.h"
#endif
#include "secrets.h"

#if (ENABLE_WIFI_AND_MQTT == 1)
WiFiClient espClient;
PubSubClient mqttClient(espClient);
std::atomic<bool> isWifiConnected{false};
std::atomic<bool> wifiRetryPending{false};
bool wifiEnabled=false;
bool mqttEnabled=false;
omote_network::RetryGate wifiRetry(10000),mqttRetry(10000);

tAnnounceWiFiconnected_cb thisAnnounceWiFiconnected_cb = NULL;
void set_announceWiFiconnected_cb_HAL(tAnnounceWiFiconnected_cb pAnnounceWiFiconnected_cb) {
  thisAnnounceWiFiconnected_cb = pAnnounceWiFiconnected_cb;  
}

tAnnounceSubscribedTopics_cb thisAnnounceSubscribedTopics_cb = NULL;
void set_announceSubscribedTopics_cb_HAL(tAnnounceSubscribedTopics_cb pAnnounceSubscribedTopics_cb) {
  thisAnnounceSubscribedTopics_cb = pAnnounceSubscribedTopics_cb;  
}

bool getIsWifiConnected_HAL() {
  return isWifiConnected;
}

// The event task only reports state. Never reconfigure a connecting station
// from its disconnect callback: Arduino also emits events during reconfiguration.
void WiFiEvent(WiFiEvent_t event) {
  if(event==ARDUINO_EVENT_WIFI_STA_GOT_IP || event==ARDUINO_EVENT_WIFI_STA_GOT_IP6) {
    wifiRetryPending=false;
    if(!isWifiConnected.exchange(true)) {
      if(thisAnnounceWiFiconnected_cb) thisAnnounceWiFiconnected_cb(true);
      Serial.printf("WiFi connected, IP address: %s\n",WiFi.localIP().toString().c_str());
    }
  } else if(event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED || event==ARDUINO_EVENT_WIFI_STA_LOST_IP) {
    wifiRetryPending=true;
    if(isWifiConnected.exchange(false) && thisAnnounceWiFiconnected_cb) thisAnnounceWiFiconnected_cb(false);
  }
}

void init_mqtt_HAL() {
  wifiEnabled=omote_network::wifiConfigured(WIFI_SSID,WIFI_PASSWORD);
  mqttEnabled=omote_network::brokerConfigured(MQTT_SERVER);
  if(thisAnnounceWiFiconnected_cb) thisAnnounceWiFiconnected_cb(false);
  if(!wifiEnabled) {
    Serial.println("WiFi not configured: local GUI/BLE remain available; set src/secrets_override.h to enable networking");
    return;
  }
  WiFi.onEvent(WiFiEvent);
  WiFi.setAutoReconnect(false); // This module owns the single retry schedule.
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("OMOTE");
  wifiRetry.mark(millis());
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  WiFi.setSleep(true);
  if(!mqttEnabled) Serial.println("MQTT not configured: broker connections disabled");
  mqttClient.setBufferSize(512);
  mqttClient.setServer(MQTT_SERVER,MQTT_SERVER_PORT);
  mqttClient.setSocketTimeout(1);
  espClient.setConnectionTimeout(1000);
}

std::string subscribeTopicOMOTEtest = "OMOTE/test";
// For connecting to one or several BLE clients
std::string subscribeTopicOMOTE_BLEstartAdvertisingForAll        = "OMOTE/BLE/startAdvertisingForAll";
std::string subscribeTopicOMOTE_BLEstartAdvertisingWithWhitelist = "OMOTE/BLE/startAdvertisingWithWhitelist";
std::string subscribeTopicOMOTE_BLEstartAdvertisingDirected      = "OMOTE/BLE/startAdvertisingDirected";
std::string subscribeTopicOMOTE_BLEstopAdvertising               = "OMOTE/BLE/stopAdvertising";
std::string subscribeTopicOMOTE_BLEprintConnectedClients         = "OMOTE/BLE/printConnectedClients";
std::string subscribeTopicOMOTE_BLEdisconnectAllClients          = "OMOTE/BLE/disconnectAllClients";
std::string subscribeTopicOMOTE_BLEprintBonds                    = "OMOTE/BLE/printBonds";
std::string subscribeTopicOMOTE_BLEdeleteBonds                   = "OMOTE/BLE/deleteBonds";

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  std::string topicReceived(topic);
  std::string strPayload(reinterpret_cast<const char *>(payload), length);
  Serial.printf("MQTT: received topic %s with payload %s\r\n", topicReceived.c_str(), strPayload.c_str());

  if (topicReceived == subscribeTopicOMOTEtest) {
    // Do whatever you want here, if it is ESP32 hardware related.
    // ...

    // Or forward the topic to "void receiveMQTTmessage_cb" in the "commandHandler.cpp", if it is not ESP32 hardware related
    thisAnnounceSubscribedTopics_cb(topicReceived, strPayload);

  #if (ENABLE_KEYBOARD_BLE == 1)
  } else if (topicReceived == subscribeTopicOMOTE_BLEstartAdvertisingForAll) {
    keyboardBLE_startAdvertisingForAll_HAL();  
  } else if (topicReceived == subscribeTopicOMOTE_BLEstartAdvertisingWithWhitelist) {
    keyboardBLE_startAdvertisingWithWhitelist_HAL(strPayload);  
  } else if (topicReceived == subscribeTopicOMOTE_BLEstartAdvertisingDirected) {
    // the payload are two values, separated by comma: peerAddress and isRandomAddress 
    std::stringstream ss(strPayload);
    if (ss.good())  {
      std::string peerAddress;
      std::getline(ss, peerAddress, ',');
      
      if (ss.good())  {
        std::string isRandomAddressStr;
        std::getline(ss, isRandomAddressStr, ',');
        bool isRandomAddress = false;
        if (isRandomAddressStr == "true") {
          isRandomAddress = true;  
        }
        keyboardBLE_startAdvertisingDirected_HAL(peerAddress, isRandomAddress);  
      }
    }
  } else if (topicReceived == subscribeTopicOMOTE_BLEstopAdvertising) {
    keyboardBLE_stopAdvertising_HAL();  
  } else if (topicReceived == subscribeTopicOMOTE_BLEprintConnectedClients) {
    keyboardBLE_printConnectedClients_HAL();
  } else if (topicReceived == subscribeTopicOMOTE_BLEdisconnectAllClients) {
    keyboardBLE_disconnectAllClients_HAL();  
  } else if (topicReceived == subscribeTopicOMOTE_BLEprintBonds) {
    keyboardBLE_printBonds_HAL();  
  } else if (topicReceived == subscribeTopicOMOTE_BLEdeleteBonds) {
    keyboardBLE_deleteBonds_HAL();  
  #endif

  } else {
    // forward all other topics to the commandHandler
    thisAnnounceSubscribedTopics_cb(topicReceived, strPayload);

  }
}

void mqtt_subscribeTopics() {
  mqttClient.setCallback(&callback);

  mqttClient.subscribe(subscribeTopicOMOTEtest.c_str());
  mqttClient.subscribe(subscribeTopicOMOTE_BLEstartAdvertisingForAll.c_str());
  mqttClient.subscribe(subscribeTopicOMOTE_BLEstartAdvertisingWithWhitelist.c_str());
  mqttClient.subscribe(subscribeTopicOMOTE_BLEstartAdvertisingDirected.c_str());
  mqttClient.subscribe(subscribeTopicOMOTE_BLEstopAdvertising.c_str());
  mqttClient.subscribe(subscribeTopicOMOTE_BLEprintConnectedClients.c_str());
  mqttClient.subscribe(subscribeTopicOMOTE_BLEdisconnectAllClients.c_str());
  mqttClient.subscribe(subscribeTopicOMOTE_BLEprintBonds.c_str());
  mqttClient.subscribe(subscribeTopicOMOTE_BLEdeleteBonds.c_str());
  Serial.printf("  Successfully subscribed to MQTT topics\r\n");

}

bool checkMQTTconnection() {
  if(!wifiEnabled || !mqttEnabled || !WiFi.isConnected()) return false;
  if(mqttClient.connected()) return true;
  if(!mqttRetry.take(millis())) return false;
  std::string name=std::string(MQTT_CLIENTNAME)+"_esp32_"+WiFi.macAddress().c_str();
  if(mqttClient.connect(name.c_str(),MQTT_USER,MQTT_PASS)) {
    Serial.println("MQTT connected");
    mqtt_subscribeTopics();
  } else {
    Serial.println("MQTT unavailable; next attempt in 10 seconds");
  }
  return mqttClient.connected();
}

void mqtt_loop_HAL() {
  if(!wifiEnabled) return;
  if(!WiFi.isConnected() && wifiRetryPending.load() && wifiRetry.take(millis())) {
    wifiRetryPending=false;
    Serial.println("WiFi disconnected; retrying connection (10-second interval)");
    if(!WiFi.reconnect()) wifiRetryPending=true;
  }
  checkMQTTconnection();
  if(mqttClient.connected()) mqttClient.loop();
}

bool publishMQTTMessage_HAL(const char *topic, const char *payload){

  if (checkMQTTconnection()) {
    // Serial.printf("Sending mqtt payload to topic \"%s\": %s\r\n", topic, payload);
      
    if (mqttClient.publish(topic, payload)) {
      // Serial.printf("Publish ok\r\n");
      return true;
    }
    else {
      Serial.printf("Publish failed\r\n");
    }
  } else {
    Serial.printf("  Cannot publish mqtt message, because checkMQTTconnection failed (WiFi or mqtt is not connected)\r\n");
  }
  return false;
}

void wifi_shutdown_HAL() {
  wifiEnabled=false;
  mqttClient.disconnect();
  wifiRetryPending=false;
  isWifiConnected=false;
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

#endif
