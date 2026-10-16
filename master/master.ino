#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLEAddress.h>

// BLE service and characteristic UUIDs.
// These must match both slave sketches.
static BLEUUID serviceUUID(
  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
);

static BLEUUID characteristicUUID(
  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
);

// Hardware
const int BUTTON_PIN = 4;

// Slave device names
const char* SLAVE1_NAME = "BLE-SLAVE-1";
const char* SLAVE2_NAME = "BLE-SLAVE-2";

// Scan settings
const int SCAN_SECONDS = 5;

// Saved BLE addresses
String slave1Address = "";
String slave2Address = "";

// BLE clients and characteristics
BLEClient* client1 = nullptr;
BLEClient* client2 = nullptr;

BLERemoteCharacteristic* characteristic1 = nullptr;
BLERemoteCharacteristic* characteristic2 = nullptr;

// Button and LED command state
bool ledsAreOn = false;
bool lastButtonReading = HIGH;
bool buttonState = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// --------------------------------------------------
// Scan for both BLE slaves
// --------------------------------------------------
bool findSlaves() {
  Serial.println();
  Serial.println("Scanning for BLE slave devices...");

  BLEScan* scan = BLEDevice::getScan();

  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(99);

  BLEScanResults* results = scan->start(SCAN_SECONDS, false);

  if (results == nullptr) {
    Serial.println("Scan failed.");
    scan->clearResults();
    return false;
  }

  int count = results->getCount();

  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice device = results->getDevice(i);

    if (!device.haveName()) {
      continue;
    }

    String deviceName = String(device.getName().c_str());
    String deviceAddress = String(
      device.getAddress().toString().c_str()
    );

    Serial.print("Found: ");
    Serial.print(deviceName);
    Serial.print(" | Address: ");
    Serial.println(deviceAddress);

    if (deviceName == SLAVE1_NAME) {
      slave1Address = deviceAddress;
    }
    else if (deviceName == SLAVE2_NAME) {
      slave2Address = deviceAddress;
    }
  }

  scan->clearResults();

  Serial.println("Scan completed.");

  if (slave1Address.length() > 0 &&
      slave2Address.length() > 0) {
    Serial.println("Both slaves found!");
    return true;
  }

  if (slave1Address.length() == 0) {
    Serial.println("BLE-SLAVE-1 was not found.");
  }

  if (slave2Address.length() == 0) {
    Serial.println("BLE-SLAVE-2 was not found.");
  }

  return false;
}

// --------------------------------------------------
// Connect to a slave
// --------------------------------------------------
bool connectToSlave(
  BLEClient*& client,
  BLERemoteCharacteristic*& characteristic,
  String address,
  const char* slaveName
) {
  if (address.length() == 0) {
    Serial.print(slaveName);
    Serial.println(": No address available.");
    return false;
  }

  Serial.print("Connecting to ");
  Serial.print(slaveName);
  Serial.println("...");

  if (client == nullptr) {
    client = BLEDevice::createClient();
  }

  BLEAddress bleAddress(address);

  if (!client->connect(bleAddress)) {
    Serial.print("Failed to connect to ");
    Serial.println(slaveName);
    return false;
  }

  Serial.print("Connected to ");
  Serial.println(slaveName);

  BLERemoteService* service =
    client->getService(serviceUUID);

  if (service == nullptr) {
    Serial.print("Service not found on ");
    Serial.println(slaveName);

    client->disconnect();
    return false;
  }

  characteristic =
    service->getCharacteristic(characteristicUUID);

  if (characteristic == nullptr) {
    Serial.print("Characteristic not found on ");
    Serial.println(slaveName);

    client->disconnect();
    return false;
  }

  Serial.print("Ready to control ");
  Serial.println(slaveName);

  return true;
}

// --------------------------------------------------
// Send ON/OFF command
// --------------------------------------------------
void sendCommand(
  BLEClient* client,
  BLERemoteCharacteristic* characteristic,
  const char* command,
  const char* slaveName
) {
  if (client == nullptr ||
      characteristic == nullptr ||
      !client->isConnected()) {

    Serial.print(slaveName);
    Serial.println(" is disconnected.");
    return;
  }

  characteristic->writeValue(
    (uint8_t*)command,
    strlen(command),
    true
  );

  Serial.print("Sent ");
  Serial.print(command);
  Serial.print(" to ");
  Serial.println(slaveName);
}

// --------------------------------------------------
// Reconnect to slaves when needed
// --------------------------------------------------
void reconnectSlaves() {
  // Rescan if either address is missing.
  if (slave1Address.length() == 0 ||
      slave2Address.length() == 0) {

    Serial.println(
      "A slave address is missing. Rescanning..."
    );

    findSlaves();
  }

  // Reconnect to Slave 1 if necessary.
  if (client1 == nullptr ||
      !client1->isConnected() ||
      characteristic1 == nullptr) {

    characteristic1 = nullptr;

    if (client1 != nullptr && client1->isConnected()) {
      client1->disconnect();
    }

    connectToSlave(
      client1,
      characteristic1,
      slave1Address,
      SLAVE1_NAME
    );
  }

  // Reconnect to Slave 2 if necessary.
  if (client2 == nullptr ||
      !client2->isConnected() ||
      characteristic2 == nullptr) {

    characteristic2 = nullptr;

    if (client2 != nullptr && client2->isConnected()) {
      client2->disconnect();
    }

    connectToSlave(
      client2,
      characteristic2,
      slave2Address,
      SLAVE2_NAME
    );
  }
}

// --------------------------------------------------
// Setup
// --------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("==============================");
  Serial.println("BLE MASTER");
  Serial.println("==============================");

  BLEDevice::init("BLE-MASTER");

  // Scan for both slaves.
  findSlaves();

  // Attempt to connect to each slave independently.
  connectToSlave(
    client1,
    characteristic1,
    slave1Address,
    SLAVE1_NAME
  );

  connectToSlave(
    client2,
    characteristic2,
    slave2Address,
    SLAVE2_NAME
  );

  Serial.println();
  Serial.println("Master ready.");
  Serial.println("Press the button to toggle both LEDs.");
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------
void loop() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      // INPUT_PULLUP: LOW means pressed.
      if (buttonState == LOW) {
        ledsAreOn = !ledsAreOn;

        const char* command =
          ledsAreOn ? "ON" : "OFF";

        Serial.println();
        Serial.print("Button pressed. Command: ");
        Serial.println(command);

        reconnectSlaves();

        sendCommand(
          client1,
          characteristic1,
          command,
          SLAVE1_NAME
        );

        sendCommand(
          client2,
          characteristic2,
          command,
          SLAVE2_NAME
        );
      }
    }
  }

  lastButtonReading = reading;

  delay(10);
}