#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

const int maxRetries = 20;
char* host = "192.168.1.13";
int port = 80;
const char* ssid = "DemoHub";
const char* password = "demopass";
const int pin = 5;

LiquidCrystal_I2C lcd(0x27, 16, 2);

ESP8266WebServer server(80);

String hubToken;

RF24 radio(4,15); // ce,cs

const uint64_t addr = 0x0a0c0a0c0aLL;

String content;
char down[13],key[11],tus,hard;
boolean goAP = false;

unsigned long time1;

void reset() {
  clearEeprom();
  lcd.clear();
  lcd.backlight();
  lcd.print("Resetting....");
  delay(2000);
  ESP.restart();
}

void setup() {
  
  Serial.begin(115200);

  pinMode(pin, INPUT_PULLUP);
  attachInterrupt(pin, reset, CHANGE);

  Wire.begin(0, 2);
  lcd.begin();
  lcd.backlight();
  lcd.print("Startup...");

  EEPROM.begin(512);
  Serial.println("Startup...");

  delay(2000);

  String staSsid = "";
  for (int i = 0; i < 32; ++i) {
    int next = EEPROM.read(i);
    if (next != 0) 
      staSsid += char(next);
  }

  String staPassword = "";
  for (int i = 32; i < 96; ++i){
    int next = EEPROM.read(i);
    if (next != 0) 
      staPassword += char(next);
  }

  String token = "";
  for (int i = 96; i < 146; ++i){
    int next = EEPROM.read(i);
    if (next != 0) 
      token += char(next);
  }

  Serial.print("STORED: ");
  Serial.print(staSsid);
  Serial.print("|");
  Serial.print(staPassword);
  Serial.print("|");
  Serial.println(token);

  hubToken = token;
  if (staSsid.length() > 1 ) {
    
    if (testSta(staSsid, staPassword) == true) {
      
      WiFi.softAPdisconnect(true);

      lcd.clear();
      lcd.backlight();
      lcd.print("Connected!");

      radio.begin();
      radio.openWritingPipe(addr);
      radio.openReadingPipe(1,addr);
      radio.startListening();

      delay(2000);
      lcd.clear();
      lcd.noBacklight();

    } else {
      lcd.clear();
      lcd.backlight();
      lcd.print("Connect failed!");
      goAP = true;
    }
  } else {
    lcd.clear();
    lcd.backlight();
    lcd.print("Go setup the hub");
    delay(2000);
    lcd.clear();
    lcd.print(ssid);
    lcd.print("|");
    lcd.print(password);
    lcd.setCursor(0,1);
    lcd.print(WiFi.softAPIP());
    goAP = true;
  }

  if (goAP) {
    startAP();
  }
}

void clearEeprom() {
  for (int i = 0; i < 146; ++i) { EEPROM.write(i, 0); }
  EEPROM.commit();
  Serial.println("Cleared Eeprom");
}

bool testSta(String staSsid, String staPassword) {
  WiFi.mode(WIFI_AP);
  // WiFi.disconnect();

  int retries = 0;

  Serial.print("Connecting to [");
  Serial.print(staSsid);
  Serial.println("]");  

  lcd.clear();
  lcd.backlight();
  lcd.print("Connecting...");

  WiFi.begin(staSsid.c_str(), staPassword.c_str());

  while (retries < maxRetries) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Successfully");
      return true;
    } 
    delay(500);
    Serial.println(".");    
    retries++;
  }

  WiFi.disconnect();

  Serial.println("Failed");
  return false;
}

void startAP(void) {
  Serial.println("Start SoftAP");

  WiFi.mode(WIFI_STA);
  WiFi.softAP(ssid, password);

  server.on("/", []() {
      content = "<!DOCTYPE html><meta charset=UTF-8><meta content='ie=edge' http-equiv='x-ua-compatible'><meta content='width=device-width, initial-scale=1.0' name='viewport'><title>Setting up your hub</title><style>*{-moz-box-sizing:border-box;box-sizing:border-box}body{font:16px Helvetica;background:#36404a}section{width:275px;background:#ecf0f1;padding:0 30px 30px 30px;margin:60px auto;text-align:center;border-radius:5px}span{display:block;position:relative;margin:0 auto;top:-40px;height:80px;width:80px;font-size:70px;border-radius:50%;background-color:#5fbeaa;color:#fff;box-shadow:1px 1px 2px rgba(0,0,0,.3)}h1{font-size:24px;font-weight:100;margin-bottom:30px;margin-top:-10px}input{width:100%;background:#bdc3c7;border:none;height:30px;margin-bottom:15px;border-radius:5px;text-align:center;font-size:14px;color:#7f8c8d}input:focus{outline:0}button{width:100%;height:30px;border:none;background:#5fbeaa;color:#ecf0f1;font-weight:100;margin-bottom:15px;border-radius:5px;transition:all ease-in-out .2s;border-bottom:3px solid #56a695}button:focus{outline:0}button:hover{background:#56a695}</style><section><span>+</span><h1>Hub Setup</h1><form action=setup><input name=ssid placeholder=SSID><input name=password placeholder=Password type=password><input name=token placeholder=Token><button type=submit>Setup</button></form></section>";
      server.send(200, "text/html", content);  
  });
    
  server.on("/setup", []() {

      String qsid = server.arg("ssid");
      String qpass = server.arg("password");
      String token = server.arg("token");

      if (qsid.length() > 0 && qpass.length() > 0 && token.length() == 50) {

        clearEeprom();
  
        Serial.println(qsid);
        Serial.println(qpass);
        Serial.println(token);

        for (int i = 0; i < qsid.length(); ++i) {
          EEPROM.write(i, qsid[i]);
        }

        for (int i = 0; i < qpass.length(); ++i) {
          EEPROM.write(32+i, qpass[i]);
        }

        for (int i = 0; i < token.length(); ++i) {
          EEPROM.write(96+i, token[i]);
        }

        EEPROM.commit();
        server.send(200, "application/json", "{\"Success\":\"true\"}");
        ESP.restart();

      } else {
        server.send(200, "application/json", "{\"Success\":\"false\"}");
      }
      
  });

  server.begin();
}

void loop() {
  if (goAP) {
    server.handleClient();
  } else {
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

        lcd.clear();
        lcd.backlight();
        lcd.print("Request failed!");

        return;
      } else {
        Serial.print("Connected to ");
        Serial.println(host);

        lcd.clear();
        lcd.print("Working...");
        lcd.noBacklight();
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
          Serial.println("Request failed!");
          client.stop();

          lcd.clear();
          lcd.backlight();
          lcd.print("Request failed!");

          return;
        }
      }

      lcd.clear();
      lcd.print("Working...");
      lcd.noBacklight();
    }

    if ((unsigned long)(millis()-time1) > 500) {
     
      WiFiClient client;
      client.stop();
    
      if (!client.connect(host, port)) {
        Serial.print("Connect to ");
        Serial.print(host);
        Serial.println(" failed");

        lcd.clear();
        lcd.backlight();
        lcd.print("Request failed!");

        return;
      } else {
        Serial.print("Connected to ");
        Serial.println(host);

        lcd.clear();
        lcd.print("Working...");
        lcd.noBacklight();
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

          lcd.clear();
          lcd.backlight();
          lcd.print("Request failed!");

          return;
        }
      }

      lcd.clear();
      lcd.print("Working...");
      lcd.noBacklight();

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
      // Serial.println(content);
    
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

    time1 = millis();
    }
  }
}
