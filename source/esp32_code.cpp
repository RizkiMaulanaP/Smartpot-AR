#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif
#include <Adafruit_Sensor.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <DHT_U.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// WIFI SSID & PASSWORD
#define WIFI_SSID "RizkiMP7"
#define WIFI_PASSWORD "12345678"

// RTDB stuff
#define API_KEY "YOUR_OWN_KEY"
#define DATABASE_URL "YOUR_OWN_URL"
#define USER_EMAIL "YOUR_OWN_EMAIL"
#define USER_PASSWORD "YOUR_OWN_PASSWORD"

#define DHTPIN 32
#define DHTTYPE DHT11

#define SOILPIN 33
#define IN1 35
#define IN2 34


DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

float temperature;
float humidity;
int16_t water_time;
int16_t water_time_cmp;
int16_t soil_humidity;

void setup()
{

  Serial.begin(115200);

  pinMode(SOILPIN, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  multi.addAP(WIFI_SSID, WIFI_PASSWORD);
  multi.run();
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    if (millis() - ms > 10000)
      break;
#endif
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectNetwork(true);

  fbdo.setBSSLBufferSize(4096, 1024);


  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;


  // Temperature
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Humidity
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));

  // Delay penyiraman air
  water_time = -999;
  water_time_cmp = -999;
}

void loop()
{

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
      sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    }
    else {
    temperature = event.temperature;
    }

    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    }
    else {
    humidity = event.relative_humidity;
    }

    sendDataPrevMillis = millis();
    Firebase.RTDB.setFloat(&fbdo, F("/plant1/temperature"), temperature) ? "ok" : fbdo.errorReason().c_str();
    Firebase.RTDB.setFloat(&fbdo, F("/plant1/humidity"), humidity) ? "ok" : fbdo.errorReason().c_str();
    // Dummy Value
    soil_humidity = analogRead(SOILPIN); 
    Serial.println(soil_humidity);
    soil_humidity = map(soil_humidity, 0, 4096, 100, 0);

    Firebase.RTDB.setInt(&fbdo, F("/plant1/soil_humidity"), soil_humidity) ? "ok" : fbdo.errorReason().c_str();

    if(water_time == -999 && water_time_cmp == -999)
    {
      Firebase.RTDB.getInt(&fbdo, F("/plant1/water_time_cmp")) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str();
      water_time_cmp = 0;
      Firebase.RTDB.getInt(&fbdo, F("/plant1/water_time_cmp"), &water_time_cmp) ? String(water_time_cmp).c_str() : fbdo.errorReason().c_str();

      Firebase.RTDB.getInt(&fbdo, F("/plant1/water_time")) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str();
      water_time = 0;
      Firebase.RTDB.getInt(&fbdo, F("/plant1/water_time"), &water_time) ? String(water_time).c_str() : fbdo.errorReason().c_str();
    }

    else if(water_time == 0) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      delay(5000);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      Firebase.RTDB.getInt(&fbdo, F("/plant1/water_time_cmp")) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str();
      water_time_cmp = 0;
      Firebase.RTDB.getInt(&fbdo, F("/plant1/water_time_cmp"), &water_time_cmp) ? String(water_time_cmp).c_str() : fbdo.errorReason().c_str();
      water_time = water_time_cmp;
    }

    else
    {
      water_time -= 1;
      Firebase.RTDB.setInt(&fbdo, F("/plant1/water_time"), water_time) ? "ok" : fbdo.errorReason().c_str();
    }

    Serial.println();

    Serial.print("Temperature: ");
    Serial.println(temperature);
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Soil Humidity: ");
    Serial.println(soil_humidity);
    Serial.print("Water Time Compare: ");
    Serial.println(water_time_cmp);
    Serial.print("Water Time Current: ");
    Serial.println(water_time);
  }
}

