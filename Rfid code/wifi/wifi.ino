#include <Wire.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>


/*
  How to connect the NodeMCU to the Nano
  Connect RX of NodeMCU => TX of Nano
*/

/*
  A LED is Attached by connecting the shorter pin to GND and longer pin to a GPIO
  NOTE: Only the ONBOARD LED i.e. D4 lights up on digital LOW; other pins light up when digital HIGH!
  If adding or subtracting working pins, make sure to change the maxPins at #REF1
*/

String separate(String strIn, int pos, char delimiter = ';') {
  int delemCount = 0;
  int len = strIn.length();
  String buf = "";
  
  for (int i = 0; i < len;i++) {  
    if (strIn.charAt(i) == delimiter) {
      delemCount++;
    } else {
      buf += strIn.charAt(i);
    }
    
    if (i == len - 1 || delemCount == pos + 1) {
      //Serial.println("buffer: " + String(buf));
      return buf;
    }
    if (strIn.charAt(i) == delimiter) {
      buf = "";
    }
  }
  return "NaN";
}

/*EEPROM wrapper START*/
void eFormat() {
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("CLEARED");
}
void eWrite(String strIn) {
  int addr = 0;
  char data[strIn.length() + 2];
  byte datasize;

  strIn.toCharArray(data, strIn.length() + 1);
  Serial.println(data);

  datasize = strIn.length() + 1;
   
  for (int i = 0; i < 512; i++) {
     EEPROM.write(i, 0);
  }
  EEPROM.write(addr++, datasize);
  for (int i=0; i<datasize; i++) {
     EEPROM.write(addr++, (uint8_t)data[i]);
  }
  EEPROM.commit();
}
String eRead(int sizeOfData) {
  String ret;
  for(int i=1; i<=sizeOfData; i++)
  {
    ret.concat( (char)EEPROM.read(i) );
  }
  return ret;
}
boolean eIsEmpty () {
  Serial.println(EEPROM.read(0));
  if (EEPROM.read(0) == 0) {
    return true;
  } else {
    return false;
  }
}
/*EEPROM wrapper END*/

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer apServer(80);


const char* ssid = "Y5M8E_B";//Change this to match your SSID. This is the 'name' of the WiFi router //4G4ALL
const char* password = "increase123";//Password of the router //decrease12
const char* server = "likemart.asia"; //Change this to the IP address of the server

String modId = "20";
const char* section = "";

String strResponse = "NaN";
String strState = "";

int temp = 25; //temperature of the board

String strSIn = "";

int statusLedA = D4;
int powerLed = D1; //shows if power has come to the module

int resetSwitch = D3;
int resetSwitchState = 1;

int nativeRst = 12;

void sendBlinkCode (int count = 1, int interval = 1000) {
  for (int i = 0;i < count;i++) {
    digitalWrite(statusLedA, LOW);
    delay(interval);
    digitalWrite(statusLedA, HIGH);
    delay(interval);
  }
}

String setAttendance(const char* payload) {
  if((WiFiMulti.run() == WL_CONNECTED)) {
    HTTPClient http;
    String strCmd = String("/eduplus/SERVER/machine.php?command=attendanceInsert&identity=OTgwNTA1&rfId=") + 
                    String(payload);

    Serial.println("requesting...");

    http.begin(server, 80, strCmd); //HTTP

    int httpCode = http.GET();
    if (httpCode == 200) {
      String response = http.getString();
      Serial.println("~" + response + "~");
      if (response == "attendanceExists") {
        sendBlinkCode(1, 500);
      } else {
        sendBlinkCode(2, 100);
      }
      return response;
    } else {
      Serial.println("~conErr~");
      sendBlinkCode(3, 1000);
      /*digitalWrite(statusLedA, LOW);
      delay(500);
      digitalWrite(statusLedA, LOW);
      delay(500);
      digitalWrite(statusLedA, LOW);
      delay(500);*/
      return "err";
    }
  }
  return "NaN";
}

/*
<!DOCTYPE html>
<html>
<head>
  <meta charset='utf-8'>
  <meta name='viewport' content='width=device-width,initial-scale=1.0,user-scalable=no'>
  <meta name='mobile-web-app-capable' content='yes'>
  <meta name='theme-color' content='#455A64'><!--MD PRIMARY 700-->

  <meta name='uid' content='eHbetaZGVtbzAwMQ=='>

  <title>EDGE Plug</title>
</head>
<body>
  <h1>Connect to router</h1>
  <p>Enter your WiFi router SSID (name) and password to continue</p>
  <form>
  <p>
    <input type='text' id='txtSSID' name='ssid' placeholder='SSID'>
  </p>
  <p>
    <input type='password' id='txtPassword' name='password' placeholder='Password'>
  </p>
  <p>
    <input type='submit' value='Connect'>
  </p>
  </form>
</body>
</html>
*/
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1.0,user-scalable=no'><meta name='mobile-web-app-capable' content='yes'><meta name='theme-color' content='#455A64'><!--MD PRIMARY 700--><meta name='uid' content='eHbetaZGVtbzAwMQ=='><title>EDGE Plug</title></head><body><h1>Connect to router</h1><p>Enter your WiFi router SSID (name) and password to continue</p><form><p><input type='text' id='txtSSID' name='ssid' placeholder='SSID'></p><p><input type='password' id='txtPassword' name='password' placeholder='Password'></p><p><input type='submit' value='Connect'></p></form></body></html>";
  String newSSID = "";
  String newPassword = "";
  String newModuleId = "20";
  apServer.send(200, "text/html", html);
  
  for (uint8_t i = 0; i<apServer.args(); i++){
    Serial.println(" " + apServer.argName(i) + ": " + apServer.arg(i) + "\n");

    if (apServer.argName(i) == "ssid") {
      newSSID = apServer.arg(i);
    }

    if (apServer.argName(i) == "password") {
      newPassword = apServer.arg(i);
    }

    newModuleId = 9991; //hardcoded
  }

  if (newSSID != "") {
    //SSID entered. Assume password also entered
    eWrite(newSSID + ";" + newPassword + ";" + newModuleId + ";rand");

    Serial.println("Found new SSID. Will reset in 1 second1");
    apServer.close();
    apServer.stop();
    delay(1000);
    setup();
  }
}
void setup() {
  String credentials = "";
  String thisPass = "";
  String thisSSID = "";
  String thisModuleId = "";
  
  EEPROM.begin(512);

  pinMode(nativeRst, OUTPUT);
  pinMode(resetSwitch, INPUT);
  pinMode(statusLedA, OUTPUT);
  pinMode(powerLed, OUTPUT);
  
  digitalWrite(statusLedA, HIGH);
  
  Serial.begin (9600);

  if (eIsEmpty()) {
    Serial.println("SSID EMPTY");
    digitalWrite(statusLedA, LOW);

    Serial.print("Configuring access point...");
    /* You can remove the password parameter if you want the AP to be open. */
    
    WiFi.softAP("EduPlus 9991");
  
  
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    apServer.on("/", handleRoot);
    apServer.begin();
    Serial.println("HTTP server started");
    
  } else {
    credentials = "Nokia;K1234567;23"; //PASSWORD CHANGE ONLY HERE

    Serial.println("Credentials found: ");
    Serial.println(credentials);
    
    thisSSID = separate(credentials, 0);
    thisPass = separate(credentials, 1);
    thisModuleId = separate(credentials, 2);

    Serial.println(thisSSID);
    Serial.println(thisPass);
    Serial.println(thisModuleId);

    ssid = thisSSID.c_str();
    password = thisPass.c_str();
    modId = thisModuleId;

    Serial.println(ssid);
    Serial.println(password);
    Serial.println(modId);

    delay(2000);
    
    WiFiMulti.addAP(ssid, password);
  }
}
void loop() {
  digitalWrite(powerLed, HIGH);
  
  strState = String(temp); //temperature is always the first var in this csv
 
  if (eIsEmpty()) {
    //act as an AP to get router credentials
    apServer.handleClient();

    if (millis() % 2 == 0) {
      digitalWrite(statusLedA, LOW);
    } else {
      digitalWrite(statusLedA, HIGH);
    }
    
  } else {
    //act as a client
    
  }

  if (Serial.available() > 0) {
    strSIn = Serial.readStringUntil('\n');
    Serial.println(strSIn);
    if (strSIn == "read") {
        Serial.println(eRead(512));
        exit;
    }
    if (strSIn == "send") {
        //Serial.println(eRead(512));
        //Serial.println();
        strState = "59C48459";
        setAttendance(strState.c_str());
        delay(1000);
        exit;
    }
    if (strSIn == "format") {
        eFormat();
        eFormat();
        delay(1000);
        exit;
    }
    //if none of these special commands
    if (strSIn.indexOf(';') == 0) {
      //Serial.println(separate(strSIn, 1));
      setAttendance(separate(strSIn, 1).c_str());
      delay(1000);
    }

  }
  
  if (resetSwitchState == 0) {
    digitalWrite(statusLedA, HIGH);
    Serial.println(millis());
    if (String(millis()).endsWith("5") || String(millis()).endsWith("4") || String(millis()).endsWith("3")) {
      eFormat();
      setup();
    }
  } else {
    //digitalWrite(statusLedA, LOW);
  }
  delay(100);
}


