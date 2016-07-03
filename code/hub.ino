#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>

RF24 radio(4,15); // ce,cs

const uint64_t addr = 0x0a0c0a0c0aLL;

char* ssid = "Duc Duy Wifi";
char* password = "1tu2duy3";
char* host = "192.168.1.14"; 
int port = 80;
char* hubToken = "Vznx2HNra15IBNEGys8I1xsjm6FTq80v9ZJe8sufj5CVsjOgqK";

String content;
char down[13],key[11],tus,hard;

void setup() {
  Serial.begin(115200);

  radio.begin();
  radio.openWritingPipe(addr);
  radio.openReadingPipe(1,addr);
  radio.startListening();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (radio.available()) {
    memset(down,' ',sizeof(down));
    radio.read(down,sizeof(down));
    Serial.print("DATA FROM NRF: ");
    Serial.println(down);
    strncpy(key,down,10);
    tus = down[10];
    hard = down[11];

    WiFiClient client;
    client.stop();

    if (!client.connect(host, port)) {
      Serial.print("Connect to ");
      Serial.print(host);
      Serial.println(" failed");
      return;
    } else {
      Serial.print("Connected to ");
      Serial.println(host);
    }

    String url = "/smartbots/api/";
    url += hubToken;
    url += "/up/";
    url += key;
    url += "/";
    url += tus;
    url += "/";
    url += hard;
  
    Serial.print("Requesting URL: ");
    Serial.println(url);
  
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
    
    unsigned long timeout = millis();
    
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout <<<");
        client.stop();
        return;
      }
    }
  }

  WiFiClient client;
  client.stop();

  if (!client.connect(host, port)) {
    Serial.print("Connect to ");
    Serial.print(host);
    Serial.println(" failed");
    return;
  } else {
    Serial.print("Connected to ");
    Serial.println(host);
  }

  String url = "/smartbots/api/";
  url += hubToken;
  url += "/down";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  
  unsigned long timeout = millis();
  
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout <<<");
      client.stop();
      return;
    }
  }

  if (client.find("\r\n\r\n")) {
    while (client.available()){
      content = client.readStringUntil('\n');
    }
  }

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject(content);
  Serial.print("DATA FROM ESP: ");
  data.printTo(Serial);
  Serial.println();

  for (int i = 0; i < data["c"]; i++) {
    char y[] = {};
    String str;
    str=String(i);
    str.toCharArray(y,10);
    const char* up = data[y];
    radio.stopListening();
    radio.write(up,11);
    radio.startListening();
  }

}

