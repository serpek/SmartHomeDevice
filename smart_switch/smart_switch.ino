#include <SocketIoClient.h>
#include <ESP8266WiFi.h> //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>

#define TRIGGER_PIN D0
#define RELAY_PIN D7

SocketIoClient webSocket;
WiFiClient wifiClient;
WiFiManager wifiManager;

//Device Attributes
String SSIDName = "BAMS_DEVICE_";
String ChipId = "ESP_";
String socketUrl = "/socket.io/?transport=websocket&type=device&token=";
uint8_t relayValue;

WiFiManagerParameter socket_server("SocketServer", "socket server", "server.serpek.com", 64);
WiFiManagerParameter socket_server_port("SocketServerPort", "socket server port", "3000", 32);
WiFiManagerParameter socket_hostname("Socket Hostname", "socket hostname", "socket", 32);
WiFiManagerParameter socket_channel("Socket Channel", "socket channel", "device", 32);
WiFiManagerParameter socket_token("Socket Token", "socket token", "5678", 32);

void reconnect();
void resetButton();
void InitSetting();
void listenDevice(const char * payload, size_t length);
void listenConnect(const char * payload, size_t length);

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  relayValue = digitalRead(RELAY_PIN);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  Serial.println("Start ....");
  InitSetting();
}

void loop()
{
  webSocket.loop();
  resetButton();
  delay(1000);
}

void InitSetting()
{
  ChipId += String(ESP.getChipId());
  SSIDName += String(ESP.getChipId());

  WiFi.hostname(SSIDName);

  wifiManager.addParameter(&socket_server);
  wifiManager.addParameter(&socket_server_port);
  wifiManager.addParameter(&socket_hostname);
  wifiManager.addParameter(&socket_channel);
  wifiManager.addParameter(&socket_token);

  wifiManager.setTimeout(120);

  if (wifiManager.autoConnect(SSIDName.c_str()))
  {
    const char *token = socket_token.getValue();
    const char *serverName = socket_server.getValue();
    const int port = atol(socket_server_port.getValue());
    const char *room = socket_hostname.getValue();
    const char *channelName = socket_channel.getValue();
    socketUrl += token;

    webSocket.on("connect", listenConnect);
    webSocket.on(channelName, listenDevice);
    webSocket.begin(serverName, port, socketUrl.c_str());

    Serial.println("connected...");
  }
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

void listenConnect(const char * payload, size_t length) {
  Serial.println("connected Socket...");
  Serial.println(payload);
  webSocket.emit("status", sendStatus());
}

void listenDevice(const char * payload, size_t length) {
  const size_t capacity = 76 + length;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  const char *token = socket_token.getValue();
  const char *tokenData = doc["token"]; // "5678"
  const char *data_type = doc["data"]["type"]; // "Number"
  int data_value = doc["data"]["value"]; // 0
  const char *event = doc["event"]; // "device"
  const char *emitter = doc["emitter"]; // "io"

  Serial.println(token);
  Serial.println(tokenData);
  Serial.println(payload);
  if (token == tokenData) {
    uint8_t value = data_value ? LOW : HIGH;
    if (relayValue != value) {
      relayValue = value;
      digitalWrite(RELAY_PIN, relayValue);
    }
  }
}

const char* sendStatus() {
  StaticJsonDocument<256> doc;
  doc["token"] = TokenKey;
  JsonObject data = doc.createNestedObject("data");
  data["type"] = "Number";
  data["value"] = relayValue;
  char json_string[256];
  serializeJson(doc, json_string);
  Serial.println(json_string);
  return json_string;
}

void reconnect()
{
  int mytimeout = millis() / 1000;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if ((millis() / 1000) > mytimeout + 3)
    { // try for less than 4 seconds to connect to WiFi router
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("\nIP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\nCheck Router ");
  }
}

void resetButton()
{
  int debounce = 500;
  if (digitalRead(TRIGGER_PIN) == HIGH)
  {
    delay(debounce);
    Serial.println("Pressed Reset Button");
    if (digitalRead(TRIGGER_PIN) == HIGH)
    {
      WiFiManager wifiManager;
      wifiManager.resetSettings();
      delay(1000);
      Serial.println("ESP Reset Config");
      ESP.restart();
    }
  }
}
