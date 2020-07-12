#include <Adafruit_NeoPixel.h>
#include <BH1750FVI.h>
#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "secret.h"

#define PIN            D4
#define NUMPIXELS      7


// Create the Lightsensor instance
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);
// Create the Neopixel instance
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
// Create the sht30 instance
SHT3X sht30(0x45);
// Create the Wifi instance
const char* ssid = SSID ;
const char* password = PASSWORD;
const char* mqtt_server = MQTT_SERVER_IP;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

int temp = 0;
int hum = 0;
int light = 0;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  LightSensor.begin(); // This initializes the light sensor
  pixels.begin(); // This initializes the NeoPixel library.
  setAllPixels(0, 0, 0);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // put your main code here, to run repeatedly:

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 200000) {
    lastMsg = now;
    if(sht30.get()==0){
      temp = sht30.cTemp*100.0;
      hum = sht30.humidity*100.0;
      light = LightSensor.GetLightIntensity();
      Serial.print("Light: ");
      Serial.println(light);
      Serial.print("Temperature in Celsius : ");
      Serial.println(sht30.cTemp);
      Serial.print("Relative Humidity : ");
      Serial.println(sht30.humidity);
      Serial.println();
      Serial.println("Publishing data to sensor/x");
      snprintf (msg, 50, "%d", light);
      client.publish("sensor/light", msg);
      snprintf (msg, 50, "%d", (int)(temp)); // no %f in arduino
      client.publish("sensor/temp", msg);
      snprintf (msg, 50, "%d", (int)(hum));
      client.publish("sensor/hum", msg);
    }
    else{
      Serial.println("Error!");
    }
    
  }
  

}

void setAllPixels(int r, int g, int b) {
  for (int i = 0; i < NUMPIXELS; i++) {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(r, g, b)); // Moderately bright green color.
    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),"","","sensor/status", 0, true, "offline")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("sensor/status", "online");
      // ... and resubscribe
      client.subscribe("sensor/cmd");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  client.publish("sensor/status", "online");
}


