#include <WiFi.h>
#include <Bonezegei_DHT11.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const uint DHTPIN = 4;
const uint DELAY = 1000 * 60;
const char* SSID = "WeatherStation-Nic";
const char* PASSWORD = "admin123456789";


Bonezegei_DHT11 dht(DHTPIN);
ulong lastSendTime = NULL;

void setup() {
  Serial.begin(115200);
  dht.begin();
  setupWlan();
}

void loop() {
  ulong currentTime = millis();
  if(lastSendTime == NULL || currentTime > (lastSendTime + DELAY)){
    sendData();
    lastSendTime = currentTime;
  }
}

void sendData(){
  if(!dht.getData())
    return;
  float temp = dht.getTemperature();
  int hum = dht.getHumidity();
  float windSpeed = 0; //TODO: Add WindSpeed
  String json;
  serializeJson(buildJSON(temp, hum, windSpeed), json);
  sendDataToHub(json);
}

JsonDocument buildJSON(float temp, int hum, float windSpeed){
  JsonDocument doc;
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  doc["windSpeed"] = windSpeed;
  return doc;
}

void sendDataToHub(String json){
  HTTPClient http;
  http.begin("http://192.168.4.1/data");
  http.addHeader("Content-Type", "application/json");
  Serial.println(json);
  int httpResponseCode = http.POST(json);
  if (httpResponseCode == 200) {
    Serial.println("Datatransfer was successful");
  } else {
    Serial.print("ERROR: Datatransfer was not successful: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void setupWlan(){
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}