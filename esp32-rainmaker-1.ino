#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <IRremote.h>
#include <ezButton.h>
#include <Rmaker.h>
#include <WiFiProv.h>

// GPIO assignments
#define RELAY_LIGHT1 4
#define RELAY_LIGHT2 5
#define RELAY_LIGHT3 18
#define RELAY_FAN    19
#define FAN_SPEED_UP 21
#define FAN_SPEED_DN 22
#define BUTTON1 23
#define BUTTON2 25
#define BUTTON3 26
#define BUTTON4 27
#define BUTTON5 32
#define BUTTON6 33
#define LDR_PIN 34
#define IR_RECEIVE_PIN 14

// EEPROM
#define EEPROM_SIZE 10

// Relay state variables
bool light1 = false, light2 = false, light3 = false, fan = true;

// IR
IRrecv irrecv(IR_RECEIVE_PIN);

// Buttons
ezButton button1(BUTTON1);
ezButton button2(BUTTON2);
ezButton button3(BUTTON3);
ezButton button4(BUTTON4);
ezButton button5(BUTTON5);
ezButton button6(BUTTON6);

// RainMaker parameters
static Switch *switch_light1, *switch_light2, *switch_light3, *switch_fan;

// Function to save state
void saveStateToEEPROM() {
  EEPROM.write(0, light1);
  EEPROM.write(1, light2);
  EEPROM.write(2, light3);
  EEPROM.write(3, fan);
  EEPROM.commit();
}

// Function to load state
void loadStateFromEEPROM() {
  light1 = EEPROM.read(0);
  light2 = EEPROM.read(1);
  light3 = EEPROM.read(2);
  fan = EEPROM.read(3);
}

// Relay control
void setRelays() {
  digitalWrite(RELAY_LIGHT1, light1);
  digitalWrite(RELAY_LIGHT2, light2);
  digitalWrite(RELAY_LIGHT3, light3);
  digitalWrite(RELAY_FAN, fan);
}

// Fan speed control (simple pulse)
void fanSpeedUp() {
  digitalWrite(FAN_SPEED_UP, HIGH);
  delay(200);
  digitalWrite(FAN_SPEED_UP, LOW);
}
void fanSpeedDown() {
  digitalWrite(FAN_SPEED_DN, HIGH);
  delay(200);
  digitalWrite(FAN_SPEED_DN, LOW);
}

// RainMaker write callback
void write_callback(Device *dev, param_t *param, const esp_rmaker_param_val_t val, void *priv) {
  const char *device_name = dev->getDeviceName();
  const char *param_name = param->getParamName();

  if (strcmp(device_name, "Light1") == 0 && strcmp(param_name, "Power") == 0) {
    light1 = val.val.b;
    digitalWrite(RELAY_LIGHT1, light1);
  } else if (strcmp(device_name, "Light2") == 0 && strcmp(param_name, "Power") == 0) {
    light2 = val.val.b;
    digitalWrite(RELAY_LIGHT2, light2);
  } else if (strcmp(device_name, "Light3") == 0 && strcmp(param_name, "Power") == 0) {
    light3 = val.val.b;
    digitalWrite(RELAY_LIGHT3, light3);
  } else if (strcmp(device_name, "Fan") == 0 && strcmp(param_name, "Power") == 0) {
    fan = val.val.b;
    digitalWrite(RELAY_FAN, fan);
  }

  saveStateToEEPROM();
  dev->updateParam(param, val);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  loadStateFromEEPROM();

  // Relay GPIOs
  pinMode(RELAY_LIGHT1, OUTPUT);
  pinMode(RELAY_LIGHT2, OUTPUT);
  pinMode(RELAY_LIGHT3, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(FAN_SPEED_UP, OUTPUT);
  pinMode(FAN_SPEED_DN, OUTPUT);

  // IR
  irrecv.enableIRIn();

  // Buttons
  button1.setDebounceTime(50);
  button2.setDebounceTime(50);
  button3.setDebounceTime(50);
  button4.setDebounceTime(50);
  button5.setDebounceTime(50);
  button6.setDebounceTime(50);

  setRelays();

  // OTA setup
  ArduinoOTA.begin();

  // RainMaker
  Node node;
  node = Node::createNode("ESP32-Node", "Switch");

  switch_light1 = new Switch("Light1", &light1);
  switch_light2 = new Switch("Light2", &light2);
  switch_light3 = new Switch("Light3", &light3);
  switch_fan    = new Switch("Fan", &fan);

  switch_light1->addCb(write_callback);
  switch_light2->addCb(write_callback);
  switch_light3->addCb(write_callback);
  switch_fan->addCb(write_callback);

  node.addDevice(switch_light1);
  node.addDevice(switch_light2);
  node.addDevice(switch_light3);
  node.addDevice(switch_fan);

  esp_rmaker_node_init(&node);
  esp_rmaker_start();

  // Provisioning
  const char *service_name = "PROV_ESP32";
  const char *pop = "123456";
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM, NETWORK_PROV_SECURITY_1, pop, service_name);
}

void loop() {
  ArduinoOTA.handle();

  button1.loop();
  button2.loop();
  button3.loop();
  button4.loop();
  button5.loop();
  button6.loop();

  if (button1.isPressed()) { light1 = !light1; digitalWrite(RELAY_LIGHT1, light1); switch_light1->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, light1); }
  if (button2.isPressed()) { light2 = !light2; digitalWrite(RELAY_LIGHT2, light2); switch_light2->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, light2); }
  if (button3.isPressed()) { light3 = !light3; digitalWrite(RELAY_LIGHT3, light3); switch_light3->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, light3); }
  if (button4.isPressed()) { fan = !fan; digitalWrite(RELAY_FAN, fan); switch_fan->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, fan); }
  if (button5.isPressed()) fanSpeedUp();
  if (button6.isPressed()) fanSpeedDown();

  if (irrecv.decode()) {
    String ir_code = String(irrecv.decodedIRData.command, HEX);
    Serial.println("IR Code: " + ir_code);
    // You can map your IR codes here
    irrecv.resume();
  }

  delay(100);
}