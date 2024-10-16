#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

const uint DISPLAY_SWITCH = 1000 * 5;
const char* SSID = "WeatherStation-Nic";
const char* PASSWORD = "admin123456789";

enum DisplayState 
{
  WIND, 
  TEMPERATURE, 
  HUMIDITY
};

WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);

ulong lastSendTime = NULL;
DisplayState currentDisplayState = DisplayState::WIND;
float currentTemperature = -1;
int currentHumidity = -1;
float currentWindSpeed = -1;

void setup() {
  Serial.begin(115200);
  //lcd.init();
  lcd.clear();
  lcd.begin(21, 22);  
  lcd.backlight();

  WiFi.mode(WIFI_AP_STA);
  Serial.println("Setting AP (Access Point)…");
  WiFi.softAP(SSID, PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.on("/data", HTTP_POST, handle_OnReceiveData);
  server.onNotFound(handle_NotFound);
  server.begin();
}

void loop() {
  server.handleClient();
  ulong currentTime = millis();
  if(lastSendTime == NULL || currentTime > (lastSendTime + DISPLAY_SWITCH)){
    displayData();
    lastSendTime = currentTime;
  }
}

void handle_OnReceiveData() {
  if (!server.arg("plain")){
    server.send(400, "text/html", "Invalid content type");
    return;
  }
  String requestBody = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, requestBody);
  if (error) {
    Serial.print("Fehler beim Parsen des JSON: ");
    Serial.println(error.c_str());
    server.send(400, "application/json", "Invalid JSON");
    return;
  }
  float temperature = doc["temperature"];
  int humidity = doc["humidity"];
  float windSpeed = doc["windSpeed"];
  Serial.println("New Data:");
  Serial.printf("Temperature: %f\n", temperature);
  Serial.printf("Humidity: %i\n", humidity);
  Serial.printf("WindSpeed: %f\n", windSpeed);
  onReceiveData(temperature, humidity, windSpeed);
  server.send(200, "text/html", "OK"); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void onReceiveData(float temp, int hum, float wind){
  currentTemperature = temp;
  currentHumidity = hum;
  currentWindSpeed = wind;
}

void displayData(){
  String name = "";
  String value = "";
  switch(currentDisplayState){
    case DisplayState::WIND:
      name = "Wind";
      value = String(currentWindSpeed, 2) + " km/h";
      break;
    case DisplayState::HUMIDITY:
      name = "Luftfeuchtigkeit";
      value = String(currentHumidity) + " %";
      break;
    case DisplayState::TEMPERATURE:
      name = "Temperatur";
      value = String(currentTemperature, 2) + " Grad";
      break;
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(name);
  lcd.setCursor(0, 1);
  lcd.print(value);
  currentDisplayState = getNextState();
}

DisplayState getNextState(){
  switch(currentDisplayState){
    case DisplayState::WIND:
      return DisplayState::HUMIDITY;
    case DisplayState::HUMIDITY:
      return DisplayState::TEMPERATURE;
    case DisplayState::TEMPERATURE:
      return DisplayState::WIND;
  }
  return DisplayState::WIND;
}