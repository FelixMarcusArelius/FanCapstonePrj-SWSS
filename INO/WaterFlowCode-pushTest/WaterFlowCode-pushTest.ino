
// version 1.0
// April-11-2023

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <FirebaseESP8266.h>
#include <String.h>
#include <time.h>
// #include <WiFiUdp.h>
// #include <NTPClient.h>
#include <iostream>

const char* ssid = "";
const char* password = "";
// const char* ntpServer = "pool.ntp.org";

#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""

// variables for connection & timestamp.
volatile uint32_t wifiLocalIP = WiFi.localIP();
volatile wl_status_t wifiStatus = WiFi.status();
String macAddress;

time_t now;
struct tm *timeinfo;

time_t init_pt;

// variables for calculation.
#define FLOWPIN D4

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 7.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned long flowMilliLitres;
unsigned int totalMilliLitres;
float flowLitres;
float totalLitres;


FirebaseData firebaseData;
FirebaseJson json;

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}


void setup() {
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  macAddress = WiFi.macAddress();

  Serial.begin(115200);

  pinMode(FLOWPIN, INPUT_PULLUP);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(FLOWPIN), pulseCounter, FALLING);

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  configTime(-14400, 0, "pool.ntp.org");

  now = time(0);

  String target_path_total = String("/sensors/total-volume/") + macAddress; 

  if (Firebase.ready() && Firebase.getFloat(firebaseData, target_path_total)) {
    totalLitres = firebaseData.to<float>();
  } else {
    totalLitres = 0.00;
  }
}

void loop ()
{
  now = time(0);
  timeinfo = localtime(&now);

  currentMillis = millis();

  if (currentMillis - previousMillis > interval) {

    pulse1Sec = pulseCount;
    pulseCount = 0;

    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    flowMilliLitres = (flowRate / 60) * 1000;
    flowLitres = (flowRate / 60);

    totalMilliLitres += flowMilliLitres;
    totalLitres += flowLitres;

    char buffer[80];

    strftime(buffer, 80, "%Y:%m:%d:%H:%M:%S", timeinfo);

    String target_path_flowrate = String("/sensors/flowrate/") + macAddress + String("/") + buffer;
    String target_path_total = String("/sensors/total-volume/") + macAddress; 
    String target_path_tempFlowRate = String("/sensors/tempFlowRate/") + macAddress;
    
      if (Firebase.ready() && flowLitres > 0) {

        Serial.print("flow rate: ");
        Serial.print(flowLitres);
        Serial.print("total volume: ");
        Serial.print(totalLitres);
        Serial.print("Time Stamp: ");
        Serial.print(buffer);
        Serial.print("\n");

        Firebase.setFloat(firebaseData, target_path_flowrate, flowLitres);
        Firebase.setFloat(firebaseData, target_path_total, totalLitres);
        Firebase.setFloat(firebaseData, target_path_tempFlowRate, flowLitres);
      } else {
        Serial.print("pending...");
        Serial.print("\n");
      }
  }
}

