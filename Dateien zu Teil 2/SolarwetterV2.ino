#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <ArduinoOTA.h>
#include <time.h>

const char *SSID = "Ihr WLAN";
const char *PWD = "WLAN-Passwort";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
const int RecordTime = 3;
const int SensorPin = 5;
float temperatur = 0;
float feuchtigkeit = 0;
float druck = 0;
float geschwindigkeit = 0;
float WindSpeed;
int dirPIN = 34;
int csPIN = 16;
int richtung = 0;
int InterruptCounter;
int dir;
String buffer1;
String hour;
String minutes;
String seconds;
String year;
String month;
String day;
String page; 
String datestring;
String timestring;
char* directions[]={ "Nord", "Nord-Ost", "Ost", "Sued-Ost", "Sued","Sued-West", "West", "Nord-West"};

Adafruit_BME280 bme;
WebServer server(80);

void setup() {
  Serial.begin(9600);
  Wire.begin(32, 33);
  
  if (!SD.begin(csPIN)) {
    Serial.println("Error, SD Initialization Failed");
    return;
  }
  
   // BME280 starten
  if (!bme.begin(0x76)) {
    Serial.println("Problem mit dem BME280");
  }
  
  connectToWiFi();
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  
  // Internet-Zeit holen
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //Server-Reaktionen definieren
  server.on("/init", getWebpage);
  server.on("/weather", getWeather);
  server.begin();

}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
    
  temperatur = bme.readTemperature();  
  feuchtigkeit = bme.readHumidity();
  druck = bme.readPressure() / 100;
  richtung = winddirect();
  wind();

  getTimevars();
  if (seconds == "00" || seconds == "01" || seconds == "02") {
    if (!sdwrite()) {
      Serial.println("Daten nicht in Datei geschrieben!");
    }
    else {
      Serial.println("Daten in Datei geschrieben!");
    }
    delay(3000);
  }
}

//Funktionen definieren

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PWD);  
  while (WiFi.status() != WL_CONNECTED) {
    //Serial.println(".");
    delay(500);
  } 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}
 
void getWebpage() {
  Serial.println("Get Webpage");
  page +="<head><title>Make-Solarwetter</title></head>";
  page +="<img src='http://www.heise.de/make/icons/make_logo.png' alt='Make:' height=10%><h1>Solar-Wetter </h1><br>";
  page +="<table>";
  page +="<tr><td><b>Temperatur:</b> </td><td>"; 
  page +=String(temperatur); 
  page +=" Grad Celsius<br></td></tr>";
  page +="<tr><td><b>Luftfeuchtigkeit:</b> </td><td>"; 
  page +=String(feuchtigkeit); 
  page +=" %<br></td></tr>";
  page +="<tr><td><b>Luftdruck:</b> </td><td>"; 
  page +=String(druck); 
  page +=" hPa<br></td></tr>";
  page +="<tr><td><b>Windrichtung:</b> </td><td>"; 
  page +=String(directions[richtung]); 
  page +="  <br></td></tr>";
  page +="<tr><td><b>Windgeschwindigkeit:</b> </td><td>"; 
  page +=String(geschwindigkeit); 
  page +=" km/h<br></td></tr>";
  page +="</table>";
  server.send(200, "text/html", page);
}
 
void getWeather() {
  Serial.println("Get weather");
  buffer1 ="{\"main\":{\"temp\":";
  buffer1 +=String(temperatur);
  buffer1 +=",\"pressure\":";
  buffer1 +=String(druck);
  buffer1 +=",\"humidity\":";
  buffer1 +=String(feuchtigkeit);
  buffer1 +="},\"wind\":{\"speed\":";
  buffer1 +=String(geschwindigkeit);
  buffer1 +=",\"deg\":";
  buffer1 +=String(45*richtung);
  buffer1 +="}}";
  server.send(200, "application/json", buffer1);
  Serial.println("Gesendetes JSON-Objekt: ");
  Serial.println(buffer1);
}

void wind() {
  InterruptCounter = 0;
  attachInterrupt(digitalPinToInterrupt(SensorPin), countup, RISING);
  delay(1000 * RecordTime);
  detachInterrupt(digitalPinToInterrupt(SensorPin));
  geschwindigkeit = (float)InterruptCounter / (float)RecordTime * 2.4;
}

void countup() {
  InterruptCounter++;
}

int winddirect(){
 int dirValue = 0;
  for (int i=0; i<100; i++) {
      dirValue = dirValue + analogRead(dirPIN);
  }
  dirValue = dirValue/100;
  if (dirValue < 230) {
     dir = 0;
  }
  if (dirValue >500 && dirValue < 580) {
     dir = 1;
  }
   if (dirValue >950 && dirValue < 990) {
     dir = 2;
  }
   if (dirValue >2300 && dirValue < 2330) {
     dir = 3;
  }
   if (dirValue >3860 && dirValue < 3890) {
     dir = 4;
  }
   if (dirValue >3460 && dirValue < 3490) {
     dir = 5;
  }
   if (dirValue >2930 && dirValue < 2950) {
     dir = 6;
  }
   if (dirValue >1640 && dirValue < 1670) {
     dir = 7;
  }
  return dir;
}   

void getTimevars(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  char timevar[5];
  strftime(timevar,3, "%H", &timeinfo);
  hour = timevar;
  strftime(timevar,3, "%M", &timeinfo);
  minutes = timevar;
  strftime(timevar,3, "%S", &timeinfo);
  seconds = timevar;
  strftime(timevar,5, "%Y", &timeinfo);
  year = timevar;
  strftime(timevar,5, "%b", &timeinfo);
  month = timevar;
  strftime(timevar,3, "%d", &timeinfo);
  day = timevar;
  datestring = day + "." + month + "." + year;
  timestring = hour + ":" + minutes + ":" + seconds;
}
boolean sdwrite() {
   String Datafile = "/" + year + "/Wetterdaten.csv";
   String Dirname = "/" + year;
   String Datastring = datestring + "," + timestring + "," + temperatur + "," + feuchtigkeit + "," + druck + "," + geschwindigkeit + "," + 45*richtung;
   
   Serial.println("Data-Zeile");
   Serial.println(Datastring);
   if (!SD.exists(Dirname)) {
    SD.mkdir(Dirname);
   }
   if (!SD.exists(Datafile)) {
    File Datas = SD.open(Datafile, FILE_WRITE);
    Datas.println("Datum,Zeit,Temperatur,Luftfeuchtigkeit,Luftdruck,Windgeschwindigkeit,Windrichtung");
    Serial.print("Kopfzeile geschrieben");
    Serial.println("Datum,Zeit,Temperatur,Luftfeuchtigkeit,Luftdruck,Windgeschwindigkeit,Windrichtung");
    Datas.close();
   }
   File Datas = SD.open(Datafile, FILE_APPEND);
   if (Datas) {
    Datas.println(Datastring);
    Datas.close();
    return true;
    Serial.println("Daten in Wetterdaten.csv geschrieben");
  } 
  else {
    Serial.println("Error, konnte Daten nicht in Wetterdaten.csv schreiben");
    return false;
  }
}
