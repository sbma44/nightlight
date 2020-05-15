#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "wifi_credentials.h"

#define LED_PIN D6
#define MAX_BRIGHTNESS 1023

#define DEFAULT_DURATION 1800000
#define FADE_DURATION 300000

HTTPClient http;
const int capacity = JSON_OBJECT_SIZE(3) + 2*JSON_OBJECT_SIZE(1);
StaticJsonDocument<capacity> doc;

void setup()
{
  Serial.begin(115200);
  Serial.println();

  WiFi.begin(WIFI_SSID, WIFI_PSK);

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
    char url[50];
    sprintf(url, "http://%s:%s/", SERVER_IP, SERVER_PORT);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();  
      Serial.println(payload);
      
      DeserializationError err = deserializeJson(doc, payload);
      if (err) {
        Serial.println("deserialization failed");
      }

      unsigned long val = doc["val"].as<unsigned long>();
      val = _max(0, _min(MAX_BRIGHTNESS, val));
      analogWrite(LED_PIN, val);
    }
    else {
      Serial.print("HTTP error: ");
      Serial.println(httpCode);
    }
    http.end();
  }
}
