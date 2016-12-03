#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define nrfIrqPin 9
#define btnPin 5

const int maxRetries = 20;
const char* ssid = "DemoHub";
const char* password = "demopass";

String staPassword = "";
String staSsid = "";
String token = "";
String host = "";
String port = "";

LiquidCrystal_I2C lcd(0x27, 16, 2);

ESP8266WebServer server(80);

RF24 radio(4,16); // ce,cs

const uint64_t pipes[2] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
// pipe.1: write | pipe.2: read -> oposite with hub

struct recei {
  byte type;
  char token[11];
  short state;
  float data;
} recei;

struct trans {
  byte type;
  char token[11];
  short state;
} trans;

String content;
char down[13],key[11],tus,hard;
boolean goAP = false;

String data1;

unsigned long time1;

//----------------------------------------------------------------------------------------

void reset() {
  clearEeprom();
  lcd.clear();
  lcd.backlight();
  lcd.print("Resetting....");
  delay(2000);
  ESP.restart();
}

//----------------------------------------------------------------------------------------

void setup() {
  
  Serial.begin(115200);

  pinMode(btnPin, INPUT_PULLUP);
  attachInterrupt(btnPin, reset, CHANGE);
//  attachInterrupt(btnPin, check_radio, LOW);

  Wire.begin(0, 2);
  lcd.begin();
  lcd.backlight();
  lcd.print("Startup...");

  EEPROM.begin(512);
  Serial.println("Startup...");

  delay(2000);

  for (int i = 0; i < 32; ++i) {
    int next = EEPROM.read(i);
    if (next != 0) 
      staSsid += char(next);
  }

  for (int i = 32; i < 96; ++i){
    int next = EEPROM.read(i);
    if (next != 0) 
      staPassword += char(next);
  }

  for (int i = 96; i < 146; ++i){
    int next = EEPROM.read(i);
    if (next != 0) 
      token += char(next);
  }

  for (int i = 146; i < 161; ++i){
    int next = EEPROM.read(i);
    if (next != 0) 
      host += char(next);
  }

  for (int i = 161; i < 166; ++i){
    int next = EEPROM.read(i);
    if (next != 0) 
      port += char(next);
  }

  Serial.print("STORED: ");
  Serial.print(staSsid);
  Serial.print("|");
  Serial.print(staPassword);
  Serial.print("|");
  Serial.print(token);
  Serial.print("|");
  Serial.print(host);
  Serial.print("|");
  Serial.println(port);

  if (staSsid.length() > 1 ) {
    
    if (testSta(staSsid, staPassword) == true) {
      
      WiFi.softAPdisconnect(true);

      lcd.clear();
      lcd.backlight();
      lcd.print("Connected!");

      radio.begin();
      radio.setAutoAck(true);
      radio.enableAckPayload();
      radio.enableDynamicPayloads();
      radio.setRetries(15,15);
      radio.openReadingPipe(1,pipes[1]);
      radio.openWritingPipe(pipes[0]);
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
//    lcd.print("192.168.4.1");
    goAP = true;
  }

  if (goAP) {
    startAP();
  }
}

//----------------------------------------------------------------------------------------

void clearEeprom() {
  for (int i = 0; i < 166; ++i) { EEPROM.write(i, 0); }
  EEPROM.commit();
  Serial.println("Cleared Eeprom");
}

//----------------------------------------------------------------------------------------

bool testSta(String staSsid, String staPassword) {
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();

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

//----------------------------------------------------------------------------------------

void startAP(void) {
  Serial.println("Start SoftAP");

  WiFi.mode(WIFI_STA);
  WiFi.softAP(ssid, password);

  server.on("/", []() {
      content = "<!DOCTYPE html><meta charset='utf-8'><meta content='ie=edge' http-equiv='x-ua-compatible'><meta content='width=device-width, initial-scale=1.0' name='viewport'><title>Setting up your hub</title><style>*{-moz-box-sizing:border-box;box-sizing:border-box}body{font:16px Helvetica;background:#36404a}section{width:275px;background:#ecf0f1;padding:0 30px 30px 30px;margin:60px auto;text-align:center;border-radius:5px}span{display:block;position:relative;margin:0 auto;top:-40px;height:80px;width:80px;font-size:70px;border-radius:50%;background-color:#5fbeaa;color:#fff;box-shadow:1px 1px 2px rgba(0,0,0,.3)}h1{font-size:24px;font-weight:100;margin-bottom:30px;margin-top:-10px}input{width:100%;background:#bdc3c7;border:none;height:30px;margin-bottom:15px;border-radius:5px;text-align:center;font-size:14px;color:#7f8c8d}input:focus{outline:0}button{width:100%;height:30px;border:none;background:#5fbeaa;color:#ecf0f1;font-weight:100;margin-bottom:15px;border-radius:5px;transition:all ease-in-out .2s;border-bottom:3px solid #56a695}button:focus{outline:0}button:hover{background:#56a695}</style><body><script>function setup(){var xhttp=new XMLHttpRequest(); xhttp.onreadystatechange=function(){if(xhttp.readyState==4&&xhttp.status==200){alert(xhttp.responseText);}}; xhttp.open('GET','setup?ssid='+document.getElementsByName('ssid')[0].value+'&password='+document.getElementsByName('password')[0].value+'&token='+document.getElementsByName('token')[0].value+'&host='+document.getElementsByName('host')[0].value+'&port='+document.getElementsByName('port')[0].value,true); xhttp.send(); return false;}</script><section> <span>+</span> <h1>Hub Setup</h1> <form id='hubSetup'> <input name='ssid' placeholder='SSID'> <input name='password' placeholder='Password' type='password'> <input name='token' placeholder='Token'><input name='host' placeholder='Host'><input name='port' placeholder='Port'> <button type='submit' onclick='return setup();'>Setup</button> </form></section></body></html>";
      server.send(200, "text/html", content);  
  });
    
  server.on("/setup", []() {

      String ussid = server.arg("ssid");
      String upass = server.arg("password");
      String utoken = server.arg("token");
      String uhost = server.arg("host");
      String uport = server.arg("port");

      Serial.println(ussid);
      Serial.println(upass);
      Serial.println(utoken);
      Serial.println(uhost);
      Serial.println(uport);

      if (ussid.length() > 0 && upass.length() > 0 && utoken.length() == 50 && uhost.length() > 0 && uport.length() > 0) {

        clearEeprom();

        for (int i = 0; i < ussid.length(); ++i) {
          EEPROM.write(i, ussid[i]);
        }

        for (int i = 0; i < upass.length(); ++i) {
          EEPROM.write(32+i, upass[i]);
        }

        for (int i = 0; i < utoken.length(); ++i) {
          EEPROM.write(96+i, utoken[i]);
        }

        for (int i = 0; i < uhost.length(); ++i) {
          EEPROM.write(146+i, uhost[i]);
        }

        for (int i = 0; i < uport.length(); ++i) {
          EEPROM.write(161+i, uport[i]);
        }

        EEPROM.commit();
        server.send(200, "text/html", "Hub setup successfully!");
        ESP.restart();

      } else {
        server.send(200, "text/html", "Please re-check infomation!");
      }
      
  });

  server.begin();
}

//----------------------------------------------------------------------------------------

void check_radio(void) {

  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);
  
  if (rx || radio.available()) {
    //
  }
}
//----------------------------------------------------------------------------------------

void loop() {
  if (goAP) {
    server.handleClient();
  } else {
    if (radio.available()) {

      radio.read(&recei,sizeof(recei));
      Serial.println("Got from Nrf: ");
      Serial.println(recei.type);
      Serial.println(recei.token);
      Serial.println(recei.state);
      Serial.println(recei.data);
      Serial.println("END");
    
      WiFiClient client;
      client.stop();
    
      if (!client.connect(host.c_str(), port.toInt())) {
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
    
      String url = "/smartbots/api/up";
    
      data1 = "hubToken=" + token + "&botToken=" + String(recei.token) + "&status=" + String(recei.state);
      
      if (recei.type == 3) {
        data1 = data1 + "&hard=1";
      }
    
      Serial.print("Requesting URL: ");
      Serial.println(url);
    
      client.print(String("POST ") + url + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" +
             "Accept: *" + "/" + "*\r\n" +
             "Content-Length: " + data1.length() + "\r\n" +
             "Content-Type: application/x-www-form-urlencoded\r\n" +
             "Connection: close\r\n" + 
             "\r\n" + data1);
    
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
    
      if (!client.connect(host.c_str(), port.toInt())) {
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
    
      String url = "/smartbots/api/down";
    
      Serial.print("Requesting URL: ");
      Serial.println(url);
      
      data1 = "hubToken="+ token;
      
      client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Accept: *" + "/" + "*\r\n" +
                   "Content-Length: " + data1.length() + "\r\n" +
                   "Content-Type: application/x-www-form-urlencoded\r\n" +
                   "Connection: close\r\n" + 
                   "\r\n" + data1);
    
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

//      StaticJsonBuffer<200> jsonBuffer;
      DynamicJsonBuffer jsonBuffer;
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
        trans.type = 1;
        strncpy(trans.token,data[y]["token"],10);
        trans.state = data[y]["state"];
        radio.stopListening();

        if (radio.write(&trans,sizeof(trans))) {
          Serial.println("Tx successfully!");
        } else {
          Serial.println("Failed tx...");
        }
        radio.startListening();
      }

    time1 = millis();
    }
  }
}
