#define BLYNK_TEMPLATE_ID "TMPL2l_tOebCw"
#define BLYNK_TEMPLATE_NAME "Bin Storage State"
#define BLYNK_AUTH_TOKEN "yI3AOkEwAIz6AfTXGyIY9aSgAd6xJEUN"

#define BLYNK_PRINT Serial
#include <ESP32Servo.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "PubSubClient.h"

// Pins
#define TRIG_PIN 18
#define ECHO_PIN 19
#define TRIG_PIN_INSIDE 12
#define ECHO_PIN_INSIDE 14
#define LED 21

// MQTT Configuration
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* lid_topic = "smartbin/lid";
const char* status_topic = "smartbin/status";

// Wi-Fi credentials
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

WiFiClient espClient;
PubSubClient client(espClient);

// Servo and state variables
Servo myServo;
int pos = 0, speed = 15;
bool lidOpen = false;

// Function declarations
void connectToWiFi();
void connectToMQTT();
void connectToBlynk();
void mqttCallback(char* topic, byte* payload, unsigned int length);
long getUltrasonicDistance(int trig, int echo);
void openLid();
void closeLid();
boolean isBinFull();
void updateBinStatus();

// Blynk Timer
BlynkTimer timer;

void setup() {
  Serial.begin(115200);
  myServo.attach(5);

  // Initialize connections
  connectToWiFi();
  connectToBlynk();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  // Subscribe to MQTT topic
  client.subscribe(lid_topic);
  Serial.println("Subscribed to topic: smartbin/lid");

  // Setup pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN_INSIDE, OUTPUT);
  pinMode(ECHO_PIN_INSIDE, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // Timer for periodic updates
  timer.setInterval(5000L, updateBinStatus);

  Serial.println("Setup complete!");
}

void loop() {
  // Run necessary services
  Blynk.run();
  timer.run();

  // Reconnect Wi-Fi, Blynk, and MQTT if disconnected
  if (WiFi.status() != WL_CONNECTED) connectToWiFi();
  if (!Blynk.connected()) connectToBlynk();
  if (!client.connected()) connectToMQTT();

  client.loop();

  // Monitor bin state
  float distance = getUltrasonicDistance(TRIG_PIN, ECHO_PIN);
  if (distance < 16 && !isBinFull()) {
    openLid();
  } else if (isBinFull()) {
    Serial.println("BIN IS FULL");
    digitalWrite(LED, HIGH);
    Blynk.virtualWrite(V1, "Full");
    Blynk.logEvent("bin_full__");
  } else {
    digitalWrite(LED, LOW);
    Blynk.virtualWrite(V1, "Not Full");
  }

  delay(1000);  // Avoid spamming the loop
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void connectToBlynk() {
  while (!Blynk.connected()) {
    Serial.println("Connecting to Blynk...");
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    delay(5000);
  }
  Serial.println("Connected to Blynk!");
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    String clientId = "ESP32Client-" + String(random(0, 10000));
    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT Connected!");
      client.subscribe(lid_topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

long getUltrasonicDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  return pulseIn(echo, HIGH) * 0.0343 / 2; 
}

void openLid() {
  if (!lidOpen) {
    while (pos <= 89) {
      pos += 1;
      myServo.write(pos);
      delay(speed);
    }
    lidOpen = true;
    delay(5000);
    closeLid();
  }
}

void closeLid() {
  if (lidOpen) {
    while (pos >= 1) {
      pos -= 1;
      myServo.write(pos);
      delay(speed);
    }
    lidOpen = false;
  }
}

boolean isBinFull() {
  float distance = getUltrasonicDistance(TRIG_PIN_INSIDE, ECHO_PIN_INSIDE);
  Serial.print("Distance inside bin: ");
  Serial.println(distance);
  return (distance < 8);
}

void updateBinStatus() {
  float distance = getUltrasonicDistance(TRIG_PIN_INSIDE, ECHO_PIN_INSIDE);
  int height = 40;  // Bin height in cm
  float percentage = ((height - distance) / height) * 100;
  percentage = max(0.0f, percentage);

  if (percentage >= 90) {
    digitalWrite(LED, HIGH);
    Blynk.logEvent("bin_full__");
  }

  // Send status to Blynk and MQTT
  String statusMessage = String(percentage) + "% full";
  Serial.print("Status Message: ");
  Serial.println(statusMessage);

  client.publish(status_topic, statusMessage.c_str());
  Blynk.virtualWrite(V0, percentage);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  
  String message;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == lid_topic && message == "open") {
    openLid();
  }
}
