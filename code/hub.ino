#include <ArduinoJson.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <printf.h>
RF24 radio(7,8); // ce,cs
#define ssid "Duc Duy Wifi"
#define password "1tu2duy3"
#define server "192.168.1.10"
#define port 80
#define apiKey "BqkbCjdsOIicxLwQo5CT7e9gBKxbcHM9hHJs4JH19UlnsntCmx"
const uint64_t addr = 0x0a0c0a0c0aLL;
String uri,packet;
char down[13],key[11],tus,hard;
unsigned long time1;
void start()
{
  Serial.println("STARTING...");
  
  Serial1.setTimeout(10000);

  while (!Serial1.availableForWrite()) {}
  Serial1.println("AT");
  if (Serial1.find("OK")) {
    Serial.println("AT OK");
  } else {
    Serial.println("AT TIMED OUT");
  }

  while (!Serial1.availableForWrite()) {}
  Serial1.println("AT+RST");
//  if (Serial1.find("ready")) {
  if (Serial1.find("invalid")) {
    Serial.println("RST OK");
  } else {
    Serial.println("RST TIMED OUT");
  }

  boolean wifiCheck = false;
  if (Serial1.find("WIFI GOT IP")) {
    Serial.println("WIFI CONNECTED AND GOT IP");
    wifiCheck = true;
  }

  while (!Serial1.availableForWrite()) {}
  Serial1.println("AT+CWMODE?");
  if (Serial1.find("+CWMODE:1")) {
    Serial.println("AT CWMODE 1[1] OK");
  } else {
      while (!Serial1.availableForWrite()) {}
      Serial1.println("AT+CWMODE=1");
    if (Serial1.find("OK")) {
      Serial.println("AT CWMODE 1[2] OK");
    } else {
      Serial.println("AT CWMODE 1 TIMED OUT");
    }
  }
  if (!wifiCheck) {
    while (!Serial1.availableForWrite()) {}
    Serial1.println("AT+CWJAP?");
    if (Serial1.find(ssid)) {
      Serial.println("CWJAP[1] OK");
    } else {
      while (!Serial1.availableForWrite()) {}
      Serial1.println("AT+CWJAP=\"" + String(ssid) + "\",\"" + String(password) + "\"");
      if (Serial1.find("OK")) {
        Serial.println("CWJAP[2] OK");
      } else {
        Serial.println("CWJAP TIMED OUT");
      }
    }
  } else {
    Serial.println("NO NEED TO CWJAP");
  }

  while (!Serial1.availableForWrite()) {}
  Serial1.println("AT+CIPMUX?");
  if (Serial1.find("+CIPMUX:0")) {
    Serial.println("CIPMUX 0[1] OK");
  } else {
    while (!Serial1.availableForWrite()) {}
    Serial1.println("AT+CIPMUX=0");
    if (Serial1.find("OK")) {
      Serial.println("CIPMUX 0[2] OK");
    } else {
      Serial.println("CIPMUX TIMED OUT");
    }
  }

  while (!Serial1.availableForWrite()) {}
  Serial1.println("AT+CIPSTART=\"TCP\",\"" + String(server) + "\"," + String(port));
  if (Serial1.find("OK")) {
    Serial.println("CIPSTART OK");
  } else {
    Serial.println("CIPSTART TIMED OUT");
  }

  Serial1.setTimeout(1000);
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  radio.begin();
//   printf_begin();
//   radio.printDetails();
  radio.openWritingPipe(addr);
  radio.openReadingPipe(1,addr);
  radio.startListening();
  while (!Serial1) {}
  start();
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
    uri     = "/smartbots/api/" + String(apiKey) + "/up/" + String(key) + "/" + String(tus) + "/" + String(hard);
    packet  = "GET " + uri + " HTTP/1.1\r\nHost: " + server + "\r\n\r\n";
    while (!Serial1.availableForWrite()) {}
    Serial1.print("AT+CIPSEND=");
    Serial1.println(packet.length());
    
    if (Serial1.find(">")) {
      Serial.println("CIPSEND OK");
      while (!Serial1.availableForWrite()) {}
      Serial1.print(packet);
      if (Serial1.find("SEND OK\r\n")) {
        Serial.println("CIPSEND DONE");
      } else {
        Serial.println("CIPSEND[2] TIMED OUT");
        start();
      }
    } else {
      Serial.println("CIPSEND[1] TIMED OUT");
      start();
    }
    
  } else {

    if ((unsigned long)(millis()-time1) > 500)
    {
      uri     = "/smartbots/api/" + String(apiKey) + "/down";
      packet  = "GET " + uri + " HTTP/1.1\r\nHost: " + server + "\r\n\r\n";
      
      while (!Serial1.availableForWrite()) {}
      Serial1.print("AT+CIPSEND=");
      Serial1.println(packet.length());
      
      if (Serial1.find(">")) {
        Serial.println("CIPSEND OK");
        while (!Serial1.availableForWrite()) {}
        Serial1.print(packet);
        
        if (Serial1.find("SEND OK\r\n")) {
      
          Serial.println("CIPSEND DONE");
        
          if (Serial1.find("\r\n\r\n")) {
            unsigned int i = 0;
            unsigned int j = 0;
            char outputString[100] = {};
            while (i < 5000) { // Kiểm tra thời gian
              if (Serial1.available()) {
                outputString[j] = Serial1.read();
                i = 0; // Đặt lại bộ đếm thời gian
                j++;
              }
              i++;
            }
            StaticJsonBuffer<200> jsonBuffer;
            JsonObject& data = jsonBuffer.parseObject(outputString);
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
        } else {
          Serial.println("CIPSEND[2] TIMED OUT");
          start();
        }
      } else {
        Serial.println("CIPSEND[1] TIMED OUT");
        start();
      }
      time1 = millis();
    }
  }
}
