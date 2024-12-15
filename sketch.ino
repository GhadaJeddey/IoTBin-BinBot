#define BLYNK_TEMPLATE_ID "TMPL2l_tOebCw"
#define BLYNK_TEMPLATE_NAME "Bin Storage State"
#define BLYNK_AUTH_TOKEN "yI3AOkEwAIz6AfTXGyIY9aSgAd6xJEUN"
#define BLYNK_PRINT Serial

#include "ESP32Servo.h"
#include "BlynkSimpleEsp32.h"
#include <WiFi.h>
#include "PubSubClient.h"

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883 ;
const char* lid_topic = "smartbin/lid";
const char* status_topic = "smartbin/status";

WiFiClient espClient;
PubSubClient client(espClient);


Servo myservo;  
const int insideTrigPin = 25;
const int insideEchoPin = 26;
const int outsideTrigPin = 22;
const int outsideEchoPin = 23;
const int servoPin = 18;
const int LED = 19; 

// Variables for distance measurement
float insideDistance; // distance between outside sensor and hand 
float outsideDistance; //distance between inside sensor and litter
float duration;
int number_times_opened = 0;
bool isServoMoving = false;
unsigned long servoStartTime = 0;
unsigned long measurementPauseTime = 0;
bool isMeasurementPaused = false;
int pos =0;


// Variables for pin status
bool outsideSensorWorking = false;
bool insideSensorWorking = false;
bool servoWorking = false;

int binFillPercentage = 0;
bool binStatus = true;
int binHeight = 30;

const char* ssid = "Wokwi-GUEST";
const char* pwd = "" ;
const char* auth = BLYNK_AUTH_TOKEN ;

BlynkTimer timer;

void setup() 
{
  Serial.begin(9600);
  delay(1000);




  pinMode(insideTrigPin, OUTPUT);
  pinMode(insideEchoPin, INPUT);
  pinMode(outsideTrigPin, OUTPUT);
  pinMode(outsideEchoPin, INPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(insideTrigPin, LOW);
  digitalWrite(outsideTrigPin, LOW);
  digitalWrite(LED, LOW);

  // Verify connections for ultrasonic sensors and servo
  outsideSensorWorking = verifyUltrasonicSensor(insideTrigPin, insideEchoPin);
  insideSensorWorking = verifyUltrasonicSensor(outsideTrigPin, outsideEchoPin);

  //attach the myservo Object to servoPin pin
  myservo.attach(servoPin);
  servoWorking = verifyServo();

  // Initialize servo position
  myservo.write(0);

  Blynk.begin(auth, ssid, pwd);
  Blynk.virtualWrite(V1,0);  
  Blynk.virtualWrite(V0,0);

  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(mqttCallback);

  connectToMQTT();


}


void loop() 
{
  Blynk.run();
  timer.run();
  //mechanism used to stop turn off the bin's operations (for maintainance par example)
  if (!binStatus) {
    delay(10000);  // Check status every 10 seconds 
    return;
  }

  if (!checkComponentsFunctionality()) return;

  //ensures theres a 8sec pause between measurements after the lid operations . 
  if (isMeasurementPaused) {
    if (millis() - measurementPauseTime >= 8000) {
      isMeasurementPaused = false;
    }
    return;
  }
  openLid();
  BLYNK_WRITE(V2);

  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  delay(1000);

}


//---------------------------------------FUNCTIONS--------------------------------------


//send pulse and mesure distance between the pulse issuer and the obstacle ( hand or trash )
float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  return (duration == 0) ? -1 : (duration * 0.034) / 2;
}


// Update bin fill percentage
void updateStorageLevel() 
{
  if (insideSensorWorking && !isServoMoving) {
    insideDistance = measureDistance(insideTrigPin, insideEchoPin);
    //Serial.print("inside distance: ");
    //Serial.println(insideDistance); 
    if (insideDistance >= 0) {
      int newFillPercentage = calculateFillPercentage(insideDistance);
      newFillPercentage = 97;
      //String statusMessage = String(binFillPercentage) + "% full";
      //client.publish(status_topic, statusMessage.c_str());
      if (newFillPercentage != binFillPercentage) {
        binFillPercentage = newFillPercentage;
        Serial.print("Bin Fill: ");
        Serial.print(binFillPercentage);
        Serial.println("%");
        Blynk.virtualWrite(V0, binFillPercentage);
        String statusMessage = String(binFillPercentage) + "% full";
        client.publish(status_topic, statusMessage.c_str());

        if (binFillPercentage >= 95) {
          Serial.println(" *_ BIN FULL - ALERT! _*");
          digitalWrite(LED,HIGH);
          Blynk.logEvent("bin_full__");
        }
      }
    } else {
      Serial.println("Bin Inside Sensor: Reading Error");
    }
  }
}
//open lid with button or mqtt msg
void openLidForced() {
      isServoMoving = true;
      isMeasurementPaused = true;
      measurementPauseTime = millis();//Stores the current time in milliseconds. 
                                      //This is used to track the pause duration . 
      number_times_opened++ ;
      Serial.println(number_times_opened);
      Blynk.virtualWrite(V1,number_times_opened);

      for (pos = 0; pos <= 90; pos += 1) {
        myservo.write(pos);
        delay(15);
      }
      Serial.println("Lid is Opened");
      delay(4000);        // Wait for garbage to be thrown in


      for (pos = 90; pos >= 0; pos -= 1) {
        myservo.write(pos);
        delay(15);
      }
      delay(1000);      // Short delay to ensure lid closes

      Serial.println("Lid is Closed");

      isServoMoving = false;
      updateStorageLevel(); // Check bin store level
}

// Function to perform lid opening operation 
void openLid() {

  if (outsideSensorWorking && servoWorking) {
    outsideDistance = measureDistance(outsideTrigPin, outsideEchoPin);
    //Serial.print("outside distance: ");
    //Serial.println(outsideDistance);
    //outsideDistance=4;
    if (outsideDistance <= 5) {      //openLid
      openLidForced();
    }
  }
}



BLYNK_WRITE(V2) {  
  int buttonState = param.asInt();
  if (buttonState == 1) {  // Button pressed
      openLidForced();
  }
}

//calculate fill percentage
int calculateFillPercentage(float distance) {
  if (distance >= binHeight)
    return 0;

  if (distance <= 5)  // if theres 5cm or less left in the bin , it is considered full .
    return 100;

  return (int)(100 - ((distance - 5) * 100) / (binHeight - 5));
}


// verify if servo and ultrasonic are operational or not  
bool verifyUltrasonicSensor(int trigPin, int echoPin) {
  for (int i = 0; i < 3; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2); 
    digitalWrite(trigPin, HIGH); //send pulse 
    delayMicroseconds(10); // generate a 10 microseconds pulse (required by most ultrasonic sensors)
    digitalWrite(trigPin, LOW); // stop pulse 

    long duration = pulseIn(echoPin, HIGH, 35000); // when the pulse returns , it sets the echo pin to high
    if (duration > 0) {
      Serial.println("UltraSonic Sensor Working!");
      return true;

    }
    delay(50);
  }
  return false;
}

bool verifyServo() {
  if(myservo.attached())
  {
    Serial.println("servo Sensor Working!");
    return true ;
  }
  return false ; 
}

// Function to check components functionality 
bool checkComponentsFunctionality () {
  if (!outsideSensorWorking && !insideSensorWorking && !servoWorking) {
    Serial.println("Critical failure: No components working.");
    delay(5000);
    return false;
  }
  return true;

}

//-----------------------------------------------------MQTT----------------------------------------------

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(message);

  if (String(topic) == lid_topic) {
    if (message == "open") {
      isServoMoving = true;
      isMeasurementPaused = true;
      measurementPauseTime = millis();//Stores the current time in milliseconds. 
                                      //This is used to track the pause duration . 
      number_times_opened++ ;
      Serial.println(number_times_opened);
      Blynk.virtualWrite(V1,number_times_opened);

      for (pos = 0; pos <= 90; pos += 1) {
        myservo.write(pos);
        delay(15);
      }
      Serial.println("Lid is Opened");
      delay(4000);        // Wait for garbage to be thrown in


      for (pos = 90; pos >= 0; pos -= 1) {
        myservo.write(pos);
        delay(15);
      }
      delay(1000);      // Short delay to ensure lid closes

      Serial.println("Lid is Closed");

      isServoMoving = false;
      updateStorageLevel(); // Check bin store level
    }
  }
}


void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) { // Replace with unique client ID if needed
      Serial.println("connected");
      client.subscribe(lid_topic); // Subscribe to the command topic
      Serial.println("Subscribed to topic: smartbin/lid");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void connectToWIFI()
{
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pwd ,6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}