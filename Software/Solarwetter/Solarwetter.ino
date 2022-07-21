#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>

const int dirPIN = 34;
const int RecordTime = 3;
const int SensorPin = 5;
const char* ssid = "WLAN-SSID";
const char* password = "WLAN-Passwort";
int dirValue = 0;
int dir;
int InterruptCounter;
float WindSpeed;
char* directions[]={ "Nord", "Nord-Ost", "Ost", "Sued-Ost", "Sued","Sued-West", "West", "Nord-West"};

Adafruit_BME280 bme;
WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  server.begin();
  Serial.println(F("Server started"));
  Serial.println(WiFi.localIP());
  Serial.println(F("BME280 test"));
  Wire.begin(32, 33);
  bool status;
  status = bme.begin(0x76);  
  if (!status) {
    Serial.println("Kann keinen BME280 finden, Verkabelung prüfen!");
    while (1);
  }
  Serial.println("BME280 ok!");
  Serial.println();
}
void loop() { 
  WiFiClient client = server.available();
  float temperatur = bme.readTemperature();
  float feuchtigkeit = bme.readHumidity();
  float druck = bme.readPressure() / 100.0F;
  float geschwindigkeit = WindSpeed;

  client.print("<head><title>Make-Solarwetter</title><meta http-equiv='refresh' content='5' /></head>");
  client.print("<img src='http://www.heise.de/make/icons/make_logo.png' alt='Make:' height=10%><h1>Solar-Wetter </h1><br>");
  client.print("<table>");
  client.print("<tr><td><b>Temperatur:</b> </td><td>"); client.print(temperatur); client.print(" Grad Celsius<br></td></tr>");
  client.print("<tr><td><b>Luftfeuchtigkeit:</b> </td><td>"); client.print(feuchtigkeit); client.print(" %<br></td></tr>");
  client.print("<tr><td><b>Luftdruck:</b> </td><td>"); client.print(druck); client.print(" hPa<br></td></tr>");
  client.print("<tr><td><b>Windrichtung:</b> </td><td>"); client.print(directions[winddirect()]); client.print("  <br></td></tr>");
  client.print("<tr><td><b>Windgeschwindigkeit:</b> </td><td>"); client.print(geschwindigkeit); client.print(" km/h<br></td></tr>");
  client.print("</table>");

  wind();
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

void wind() {
  InterruptCounter = 0;
  attachInterrupt(digitalPinToInterrupt(SensorPin), countup, RISING);
  delay(1000 * RecordTime);
  detachInterrupt(digitalPinToInterrupt(SensorPin));
  WindSpeed = (float)InterruptCounter / (float)RecordTime * 2.4;
}

void countup() {
  InterruptCounter++;
}
