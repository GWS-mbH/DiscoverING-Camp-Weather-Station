#include <WiFi.h>
#include <Bonezegei_DHT11.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const uint DHT_PIN = 4;
const uint DELAY = 1000 * 4;
//TODO: Setup WIFI
const char* SSID = "";
const char* PASSWORD = "";

const int SENSOR_PIN = 35;       //Pin muss interruptfähig sein und darf kein AD2 Pin sein, weil diese durch das WLAN genutzt werden
const long MESS_INTERVAL = 1000 * 2; //Mess-Intervall für Windmessung in Millisekunden
const int WIND_FACTOR = 240; //Faktor mit Anemometer-Messung bestimmen

TaskHandle_t taskWind;
SemaphoreHandle_t i2cSemaphore;
volatile float windSpeed;
int interruptCounter = 0;

Bonezegei_DHT11 dht(DHT_PIN);
ulong lastSendTime = NULL;

void setup() {
  Serial.begin(115200);

  createSemaphore();

  dht.begin();
  setupWlan();

  xTaskCreatePinnedToCore(
    windTask,
    "WindTask",
    10000,
    NULL,
    1,
    &taskWind, 
    0);
}

void createSemaphore(){
  i2cSemaphore = xSemaphoreCreateMutex();
  xSemaphoreGive( ( i2cSemaphore) );
}

void lockVariable(){
  xSemaphoreTake(i2cSemaphore, portMAX_DELAY);
}

void unlockVariable(){
  xSemaphoreGive(i2cSemaphore);
}

void loop() {
  ulong currentTime = millis();
  if(lastSendTime == NULL || currentTime > (lastSendTime + DELAY)){
    sendData();
    lastSendTime = currentTime;
  }
}

void windTask(void* pvParameters){
  for(;;){
    measureWindSpeed();
  }
}

void measureWindSpeed(){
  interruptCounter = 0;
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), countUp, RISING);
  delay(MESS_INTERVAL);
  detachInterrupt(digitalPinToInterrupt(SENSOR_PIN));
  lockVariable();
  windSpeed = (float)interruptCounter / (float)MESS_INTERVAL * WIND_FACTOR;
  unlockVariable();
}

void countUp() {
    interruptCounter++;
}

void sendData(){
  if(!dht.getData())
    return;
  lockVariable();
  //TODO: Get Temperature & Humidity from Sensor
  //Documentation: https://github.com/GWS-mbH/DiscoverING-Camp-Weather-Station/blob/main/libraries/Bonezegei_DHT11/src/Bonezegei_DHT11.h
  float temp = -1;
  int hum = -1;
  float windSpeedCopy = windSpeed;
  unlockVariable();
  String json;
  serializeJson(buildJSON(temp, hum, windSpeedCopy), json);
  
  sendDataToHub(json);
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print("---");
  Serial.print("Hum: ");
  Serial.print(hum);
  Serial.print("---");
  Serial.print("Wind: ");
  Serial.print(windSpeedCopy);
  Serial.println("---");
}

JsonDocument buildJSON(float temp, int hum, float windSpeed){
  JsonDocument doc;
  //TODO: Build JsonDocument
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