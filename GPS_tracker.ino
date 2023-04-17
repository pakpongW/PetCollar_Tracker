#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <TridentTD_LineNotify.h>
#include <BlynkSimpleEsp32.h>
#include "Secret.h"

#define BLYNK_PRINT Serial
#define RX_PIN 16
#define TX_PIN 17

#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

BlynkTimer timer;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SoftwareSerial gpsSerial(RX_PIN, TX_PIN);
TinyGPSPlus gps ;
TinyGPSCustom zdop(gps, "GPVTG", 7);
String googleMapsURL ;
String areaState ;

int buzzerPin = 25;
int noteDuration = 100;
int pauseDuration = 50;
bool isBuzzerOn = false;
bool isInside = false;

// Define global variables for the area square with a side length of 0.2 km
float lat1, lon1, lat2, lon2;

BLYNK_WRITE(V4){
  if(param.asInt()){
    Blynk.setProperty(V3, "label", "googleMapsURL");
    // Serial.print(String(googleMapsURL));
    LINE.notify("this is your pet location: " + String(googleMapsURL));
    Serial.println("pressed!!");
  }
}

BLYNK_WRITE(V8){
  if(param.asInt()){
    calculateArea();
    Blynk.virtualWrite(V6, (String(lat1, 6) + ", " + String(lat2, 6)));
    Blynk.virtualWrite(V7, (String(lon1, 6) + ", " + String(lon2, 6)));
    LINE.notify("Your location was set on: x1:" + String(lat1, 6) + ", x2: " + String(lat2, 6) + ", y1: " + String(lon1, 6) + ", y2: " + String(lon2, 6));
    areaState = "set";
    isInside = false;
    Serial.print("ser pressed!!");
  }

}

BLYNK_WRITE(V5) { 
  if(param.asInt() == 1){ 
    isBuzzerOn = true;
  }else{
    isBuzzerOn = false;
  }
}


void setup() {
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  Serial.begin(115200);
  gpsSerial.begin(9600);
  
  //set display OLED display 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  //set LINE token  
  LINE.setToken(SECRET_LINETOKEN);
  WiFi.begin(SECRET_WIFINAME, SECRET_PASSWORD);

  //check Wifi connection
  checkWifi();

  //Start to connect to Blynk
  Blynk.begin(SECRET_BLYNKTOKEN, SECRET_WIFINAME, SECRET_PASSWORD);

  //check blynk connection
  checkBlynk();

  //check GPS Fixtion
  checkGPS();

  areaState = "notset";
}

void checkGPS(){
  while(gpsSerial.available() == false){
    display.clearDisplay();
    display.setCursor(0, 30);
    display.print("Fixing GPS.");
    display.display();
    delay(1000);
    display.print(".");
    display.display();
    delay(1000);
    display.display();
    display.print(".");
    delay(1000);
    display.display();
    display.print(".");
    delay(1000);
    display.display();
    display.clearDisplay();
  }
  display.clearDisplay();
  display.setCursor(0, 30);
  display.print("GPS is ready!!!");
  display.display();
  delay(2000);
}

void checkWifi(){
  while (WiFi.status() != WL_CONNECTED)
  {
    display.clearDisplay();
    display.setCursor(0, 30);
    display.print("Connecting WiFi.");
    display.display();
    delay(1000);
    display.print(".");
    display.display();
    delay(1000);
    display.display();
    display.print(".");
    delay(1000);
    display.display();
    display.print(".");
    delay(1000);
    display.display();
  }
  display.clearDisplay();
  display.setCursor(0, 30);
  display.print("Connected WiFi!!");
  display.display();
  delay(2000);
}

void checkBlynk(){
  while (Blynk.connect() == false){
    display.clearDisplay();
    display.setCursor(0, 30);
    display.print("Connecting to Blynk.");
    display.display();
    delay(1000);
    display.print(".");
    display.display();
    delay(1000);
    display.display();
    display.print(".");
    delay(1000);
    display.display();
    display.print(".");
    delay(1000);
    display.display();
  }
  display.clearDisplay();
  display.setCursor(0, 30);
  display.print("Connected Blynk!!");
  display.display();
  delay(2000);
}

void loop() {
  // byte gpsData = gpsSerial.read();
  // Serial.write(gpsData);

  while(gpsSerial.available() > 0){
  // get the byte data from the GPS
    gps.encode(gpsSerial.read());
    if (gps.location.isUpdated()){
      // Serial.print("Latitude= "); 
      // Serial.print(gps.location.lat(), 6);
      // Serial.print(" Longitude= "); 
      // Serial.println(gps.location.lng(), 6);

      checkArea();
      blynkDisplay();
      display.clearDisplay();
      showBattery();
      displayLocation();
      displayDateTime();
      display.display();

      if(isBuzzerOn == 1){
        playTune();    
      }

      Blynk.run(); 
    }
  }

}

void displayLocation() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.print("location: ");

  if(gps.location.lat() == 0.0000 && gps.location.lng() == 0.0000){
    display.setCursor(0, 20);
    display.print(" not found +_+' ");
  }
  else{
    display.setCursor(0, 20);
    display.print("x:");
    display.print(gps.location.lng(), 4);
    display.setCursor(0, 30);
    display.print("y:");
    display.print(gps.location.lat(), 4);
  }
  
}

void blynkDisplay(){
  if (gps.location.isValid()){
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();
    Blynk.virtualWrite(V1, String(latitude, 6));
    Blynk.virtualWrite(V2, String(longitude, 6));

    googleMapsURL = "https://www.google.com/maps/search/?api=1&query=";
    googleMapsURL += String(latitude, 6);
    googleMapsURL += ",";
    googleMapsURL += String(longitude, 6);
    // Serial.println(String(googleMapsURL));
    Blynk.virtualWrite(V3, String(googleMapsURL));

  }else{
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();
    Blynk.virtualWrite(V1, String(latitude, 6));
    Blynk.virtualWrite(V2, String(longitude, 6));
    Blynk.virtualWrite(V3, " Not found +_+' ");
    googleMapsURL = "Not found +_+";
    // Serial.println(String(googleMapsURL));
  }
}

void calculateArea() {
  // Calculate the area of the square with a side length of 0.2 km around the current location
  float sideLength = 0.2;
  float lat = gps.location.lat();
  float lon = gps.location.lng();

  Serial.println("In calculateArea");
  
  // Calculate the distance in degrees for the given side length
  float distance = sideLength / 111.32; // 1 degree of latitude is approximately 111.32 km
  
  // Calculate the coordinates of the corners of the square
  lat1 = lat - distance;
  lon1 = lon - distance;
  lat2 = lat + distance;
  lon2 = lon + distance;
}

void checkArea() {
  // Get the latitude and longitude of the current location
  float lat = gps.location.lat();
  float lon = gps.location.lng();

  if(areaState == "set"){
    // Check if the current location is inside or outside the square area
    if (lat >= lat1 && lat <= lat2 && lon >= lon1 && lon <= lon2) {
      if(isInside == false){
        LINE.notify("Your pet is in area: " + String(googleMapsURL));
        isInside = true;
      }
    } 
    else {
      if(isInside == true){
        LINE.notify("Your pet is out of your area" + String(googleMapsURL));
        isInside = false;
      }
      
    }
  }
}

void displayDateTime() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // //for date
  display.setCursor(0, 40);
  display.print("Date: ");
  display.print(gps.date.month());
  display.print('/');
  display.print(gps.date.day());
  display.print('/');
  display.print(gps.date.year());

  //for time
  int hour ;
  if(gps.time.hour() < 17){
    int hour = gps.time.hour() + 7;
  }else{
    int hour = gps.time.hour();
  }
  int minute = gps.time.minute();
  int second = gps.time.second();
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hour, minute, second);
  display.setCursor(0, 50);
  display.print("Time: ");
  display.print(timeStr);
  display.display();
}

void showBattery(){
  float battery_voltage = analogRead(A0) * (5.0 / 1023.0);
  int battery_percent = map(battery_voltage, 3.0, 4.2, 0, 100);

  int usb_voltage = analogRead(34) * (5.0 / 1023.0) * 2.0;
  bool is_charging = usb_voltage > 4.9;

  // Draw battery bar
  display.drawRect(100, 0, 24, 12, WHITE);
  display.fillRect(127, 3, 4, 6, WHITE);
   if (usb_voltage > 4.9) {
    for (int i = 0; i < 10; i++) {
      display.fillRect(103, 3, map(i, 0, 10, 0, 18), 6, WHITE);
      displayLocation();
      displayDateTime();
      display.display();
      delay(100);
    }
  } else {
    display.fillRect(103, 3, map(battery_percent, 0, 100, 0, 18), 6, WHITE);
  }
}

void playTune() {
  int melody[] = {
    262, 294, 330, 349, 392, 440, 494, 523
  };
  
  for (int i = 0; i < 8; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(noteDuration);
    digitalWrite(buzzerPin, LOW);
    delay(pauseDuration);
    tone(buzzerPin, melody[i], noteDuration);
  }
  noTone(buzzerPin);
}