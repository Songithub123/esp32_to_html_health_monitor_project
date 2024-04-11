#include <WiFi.h>
#include <WebSocketsServer.h>

const char *ssid = "giangSon";
const char *pass = "124781998";

WebSocketsServer webSocket = WebSocketsServer(1337);
int heartBeat = 80;
int oxygenLevel = 56;
int bodyTemperature = 31;
float e;
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
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
  e = (float)random(-1, 1);
  heartBeat += e;
  oxygenLevel += e;
  bodyTemperature+= e;
  webSocket.loop();
  // Assuming you have variables heartBeat, oxygenLevel, and bodyTemperature
  String data = "{ \"heartBeat\": " + String(heartBeat) + ", \"oxygenLevel\": " + String(oxygenLevel) + ", \"bodyTemperature\": " + String(bodyTemperature) + " }";
  webSocket.broadcastTXT(data);
  delay(50);
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
