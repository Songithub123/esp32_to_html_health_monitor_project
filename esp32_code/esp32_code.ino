#include <WiFi.h>
#include <WebSocketsServer.h>

const char *ssid = "";
const char *pass = "";

WebSocketsServer webSocket = WebSocketsServer(1337);

int heartBeat = 80;
int oxygenLevel = 56;
int bodyTemperature = 31;

int check_wifi_interval=0;
int sensor_interval=0;

float latitude=0;
float longitude=0;
float e;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    Serial.println("Esp32  is connecting....");  
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());           // Send the IP address of the ESP32 to the computer
  }
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}


void loop() {
  if (millis() - check_wifi_interval > 500){
    while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    Serial.println("Esp32  is connecting....");  
    check_wifi_interval = millis();
  }
  }
  if (millis() - sensor_interval > 50){
    e = (float)random(-1, 1);
    latitude = (float)random(-20, 20);
    longitude = (float)random(-20, 20);
    heartBeat += e;
    oxygenLevel += e;
    bodyTemperature+= e;
    webSocket.loop();
    // Assuming you have variables heartBeat, oxygenLevel, and bodyTemperature
    String data = "{ \"heartBeat\": " + String(heartBeat) + ", 
                    \"oxygenLevel\": " + String(oxygenLevel) + ", 
                    \"bodyTemperature\": " + String(bodyTemperature) + ",
                    \"Latitude\":" + String(latitude) + ",
                    \"Longitude\":" + String(longitude) + ", }";
    webSocket.broadcastTXT(data);
    sensor_interval = millis();
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
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
