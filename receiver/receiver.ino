#include <esp_now.h>
#include <WiFi.h>
#include <string.h>

typedef struct struct_message {
  char label[32];
  int count;
  float temperature;
  bool alarm;
} struct_message;

struct_message incoming;

void onReceive(const esp_now_recv_info_t* info,
               const uint8_t* data, int len) {
  if (len != sizeof(incoming)) {
    Serial.println("Unexpected packet size");
    return;
  }

  memcpy(&incoming, data, sizeof(incoming));

  Serial.println("\n--- Message Received ---");

  Serial.print("Sender: ");
  Serial.println(incoming.label);

  Serial.print("Sender MAC: ");
  for (int i = 0; i < 6; i++) {
    if (i > 0) Serial.print(":");

    if (info->src_addr[i] < 16) {
      Serial.print("0");
    }

    Serial.print(info->src_addr[i], HEX);
  }
  Serial.println();

  Serial.print("Count: ");
  Serial.println(incoming.count);

  Serial.print("Temperature: ");
  Serial.println(incoming.temperature);

  Serial.print("Alarm: ");
  Serial.println(incoming.alarm ? "YES" : "NO");

  Serial.println("------------------------");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  Serial.println("Receiver ready. Waiting for messages...");
}

void loop() {}