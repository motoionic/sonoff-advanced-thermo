#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define BUTTON 0
#define LED 13
#define RELAY 12
#define THERMO 14
#define ssid ""
#define password ""
#define mqtt_server "192.168.1.1"
#define mqtt_user ""
#define mqtt_pass ""
#define mqtt_id ""
#define mqtt_topic "lights/mbedroom/right"

bool sendStatus = false;
bool requestRestart = false;
unsigned long count = 0;
unsigned long nowtime = millis();

OneWire ds(THERMO);
DallasTemperature sensors(&ds);

DeviceAddress bedroomThermometer;

void callback(char* topic, byte* payload, unsigned int length) {

  String payload_string = "";
  for (int i = 0; i < length; i++) {
    payload_string = payload_string + (char)payload[i];
  }
  
  if (payload_string == "on") {
    digitalWrite(LED, LOW);
    digitalWrite(RELAY, HIGH);
    Serial.println("Relay ON via MQTT");
  } else if (payload_string == "off") {
    digitalWrite(LED, HIGH);
    digitalWrite(RELAY, LOW);
    Serial.println("Relay OFF via MQTT");
  } else if (payload_string == "reset") {
    requestRestart = true;
  }

  sendStatus = true;
}

WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, callback, espClient);
Ticker btn_timer;

void setup() 
{
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  pinMode(BUTTON, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);

  digitalWrite(RELAY, LOW);
  digitalWrite(LED, HIGH);

  btn_timer.attach(0.05, button);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());

  sensors.begin();

  sensors.getAddress(bedroomThermometer, 0);
  sensors.setResolution(bedroomThermometer, 9);

  Serial.print("Thermometer Address: ");

  for (uint8_t i = 0; i < 8; i++)
  {
    if (bedroomThermometer[i] < 16) Serial.print("0");
    Serial.print(bedroomThermometer[i], HEX);
  }

  Serial.println();

  
}

void button() {
  if (!digitalRead(BUTTON)) {
    count++;
  } 
  else {
    if (count > 1 && count <= 40) {   
      digitalWrite(LED, !digitalRead(LED));
      digitalWrite(RELAY, !digitalRead(RELAY));
      Serial.println("Relay status toggled via button");
      sendStatus = true;
    } 
    
    count=0;
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(mqtt_id, mqtt_user, mqtt_pass)) {
      client.subscribe (mqtt_topic);
    } else {
      delay (5000);
    }
  }
}

void checkStatus() {
  if (sendStatus) {
    if(digitalRead(LED) == LOW) {
      client.publish((char*) mqtt_topic"/status", (char*) "on");
      Serial.println((char*) mqtt_topic"/status");
      Serial.println("Relay status is on");
    } else {
      client.publish((char*) mqtt_topic"/status", (char*) "off");
      Serial.println((char*) mqtt_topic"/status");
      Serial.println("Relay status is off");
    }
    sendStatus = false;
  }
}

void loop() 
{
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  checkStatus();

  if (millis() - nowtime > 60000) {
    sensors.requestTemperatures();
    String inC = String(sensors.getTempC(bedroomThermometer), 1);
    Serial.print("Bedroom Thermometer: ");
    Serial.println(inC);

    client.publish((char*) "thermometer/mbedroom", (char*) inC.c_str() );

    nowtime = millis();
  }
}
