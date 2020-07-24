#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include "wifi_credentials.h"

#define LED_PIN D8
#define MAX_BRIGHTNESS 1023

#define DEFAULT_DURATION 1800000
#define FADE_DURATION 300000

const char* MQTT_IP = "192.168.1.2";

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");
  for (int i = 0; i < length; i++) {
    Serial.print(" 0x");
    Serial.print(String(payload[i], HEX));
  }
  Serial.println();

  int val;
  if ((payload[0] == 0xFF) && (payload[1] == 0xFF) && (payload[2] == 0xFF) && (payload[3] == 0xFF)) {
    //ESP.deepSleep(0);
    val = 0;
  }
  else {
    val = (payload[2] * 256) + payload[3];
    if (val > MAX_BRIGHTNESS)
      val = MAX_BRIGHTNESS;
    if (val < 0)
      val = 0;
  }
  analogWrite(LED_PIN, val);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Nightlight-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("nightlight", "start");
      // ... and resubscribe
      client.subscribe("nightlight");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);

  Serial.begin(115200);
  Serial.println();

  WiFi.begin(WIFI_SSID, WIFI_PSK);

  client.setServer(MQTT_IP, 1883);
  client.setCallback(callback);

  Serial.print("Connecting");
  unsigned long start = millis();
  while ((WiFi.status() != WL_CONNECTED) && (millis() - start < 3000))
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long val;
    if (millis() < (DEFAULT_DURATION - FADE_DURATION)) {
      val = MAX_BRIGHTNESS;
    }
    else if (millis() > (DEFAULT_DURATION - FADE_DURATION) && (millis() < DEFAULT_DURATION)) {
      val = MAX_BRIGHTNESS * (millis() - (DEFAULT_DURATION - FADE_DURATION)) / DEFAULT_DURATION;
    }
    else {
      val = 0;
    }
    analogWrite(LED_PIN, val);
  }
  else {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }
}
