#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

MAX30105 particleSensor;

const byte RATE_SIZE = 4;  //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];     //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;  //Time at which the last beat occurred

float beatsPerMinute = 0;
int beatAvg = 0;
long irValue = 0;
bool irBool = false;
float oxygenLevel = 0;


//TEMP
#define ONE_WIRE_BUS 5  // Change this to the GPIO pin you connected the DS18B20 data pin to
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float previousTemp;
float tempC;




const char *ssid = "";
const char *pass = "";
WebSocketsServer webSocket = WebSocketsServer(1337);
unsigned long check_wifi_interval = 0;
unsigned long sensor_interval = 0;



float latitude = 0; 
float longitude = 0;
TinyGPSPlus gps;
HardwareSerial SerialGPS(1);
float e = 0;



void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");
  SerialGPS.begin(115200, SERIAL_8N1, 16, 17);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    Serial.println("Esp32  is connecting....");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connection established!");
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());  // Send the IP address of the ESP32 to the computer
  }
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))  //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1)
      ;
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();                     //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A);  //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0);   //Turn off Green LED


  sensors.begin();
}

void read_max30102() {
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true) {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;  //Store this reading in the array
      rateSpot %= RATE_SIZE;                     //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000) {
    irBool = false;
    Serial.print(" No finger?, ");
  } else {
    irBool = true;
  }
  Serial.println();
}



void read_ds18b20() {
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);
  if (tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temp: ");
    Serial.println(tempC);
  } else {
    Serial.println("Error: Unable to read temperature.");
  }
}


void read_neo_6m(){
  if (gps.encode(SerialGPS.read())) {
    if (gps.location.isValid()){
      latitude = gps.location.lat();
      longitude = gps.location.lng();
      Serial.print("Latitude = ");
      Serial.println(latitude);
      Serial.print("Longitude = ");
      Serial.println(longitude);
      delay(50);
    }
  } else {
    Serial.println("NO GPS");
  }
}

void loop() {
  read_max30102();
  delay(1);
  read_ds18b20();
  delay(1);
  read_neo_6m();
  if (millis() - check_wifi_interval > 500) {
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {  // 5 seconds timeout
      WiFi.begin(ssid, pass);
      Serial.println("Esp32  is connecting....");
    }
    check_wifi_interval = millis();
  }
  if (millis() - sensor_interval > 10) {
    /*e = (float)random(-1, 1);
    latitude = (float)random(-20, 20);
    longitude = (float)random(-20, 20);
    beatsPerMinute += e;
    oxygenLevel += e;
    tempC+= e;*/

    webSocket.loop();
    String data = "{ \"heartBeat\": " + String(beatsPerMinute) + ","
                  //                          \"oxygenLevel\": "
                  //+ String(oxygenLevel) + ","
                                          " \"bodyTemperature\": "
                  + String(tempC) + ","
                                    " \"Latitude\":"
                  + String(latitude) + ","
                                       " \"Longitude\":"
                  + String(longitude) + ", "
                                        " \"IR\":"
                  + String(irBool) + " }";

    webSocket.broadcastTXT(data);
    sensor_interval = millis();
  }
}




void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_TEXT:
      // Handle text messages from the client
      if (payload[0] == 'p' && payload[1] == 'i' && payload[2] == 'n' && payload[3] == 'g') {
        webSocket.sendTXT(num, "pong");
      }
      break;
    case WStype_DISCONNECTED:
      // Handle client disconnection
      break;
    case WStype_CONNECTED:
      // Handle a new client connection
      break;
  }
}
