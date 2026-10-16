#include <esp_now.h>
#include <WiFi.h>
#include <string.h>

uint8_t broadcastAddress[6] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

typedef struct struct_message {
  char label[32];
  int count;
  float temperature;
  bool alarm;
} struct_message;

struct_message outgoing = {};
esp_now_peer_info_t peerInfo = {};

void onSent(const wifi_tx_info_t* info,
            esp_now_send_status_t status) {
  Serial.println(
    status == ESP_NOW_SEND_SUCCESS
      ? "Delivery success"
      : "Delivery fail"
  );
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_send_cb(onSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
  }

  Serial.println("Sender 1 ready");
}

void loop() {
  static unsigned long lastSend = 0;

  if (millis() - lastSend >= 2000) {
    lastSend = millis();

    strcpy(outgoing.label, "SENDER-1");
    outgoing.count++;
    outgoing.temperature =
        25.0 + (outgoing.count % 5) * 0.5;
    outgoing.alarm = false;

    esp_now_send(
      broadcastAddress,
      (uint8_t*)&outgoing,
      sizeof(outgoing)
    );
  }
}