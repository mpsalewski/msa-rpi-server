/******************************************************************************
 * 
 * File:           room_monitor.ino
 * Author(s):      Mika Paul Salewski
 * Created:        2025-11-20
 * Last Updated:   2025-11-27
 * Version:        2025.11.20
 *
 * Title:
 *     ESP32 Room Monitor — Temperature & Humidity Sender
 *
 * Description:
 *     This firmware runs on an ESP32 and periodically reads
 *     temperature and humidity values from a DHT11 sensor.
 *     The measured values are transmitted to a Raspberry Pi
 *     REST API backend using HTTP POST requests.
 *
 * Copyright:
 *     (c) 2025, Mika Paul Salewski
 *
 * License:
 *    CC BY-NC-SA 4.0
 * 
 * Notes:
 *     - WiFi credentials and server API keys are stored in
 *       the separate secrets.h file (excluded from version control).
 *     - This module is intentionally simple and robust for
 *       embedded environments.
 *  
******************************************************************************/



/************************ Includes / Libraries *******************************/
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include "secrets.h"    // WiFi SSID/password + SERVER_URL + API_KEY




/*************************** Local Defines ***********************************/
/* Sensor Configuration */
#define DHTPIN 2        // GPIO Pin für DHT Sensor
#define DHTTYPE DHT11   // DHT Sensor Typ




/************************** Local Structure **********************************/
/* (Reserved for future struct definitions if needed) */   




/************************* Local Variables ***********************************/
/* DHT unified driver instance */ 
DHT_Unified dht(DHTPIN, DHTTYPE);

/* Sensor measurement buffers */                                             
float temperature = 0.0;
float humidity = 0.0;

/* Measurement delay in milliseconds */  
uint32_t delayMS = 2000; 




/************************** Function Declaration *****************************/
/* Sends single sensor measurement to backend server */    
void sendSensorData(const char* sensorType, float value);



/**************************** Setup ******************************************/
void setup() {


    /* Initialize serial interface */                                          
    Serial.begin(115200);

    /* Initialize DHT sensor */                                                
    dht.begin();
    Serial.println(F("DHTxx Unified Sensor Initialization"));

    /* Retrieve sensor metadata */                                             
    sensor_t sensor;


    /* ---------------- Temperature sensor info ---------------- */
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
    /* ---------------- Humidity sensor info ------------------- */
    dht.humidity().getSensor(&sensor);
    Serial.println(F("Humidity Sensor"));
    Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
    Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
    Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
    Serial.println(F("------------------------------------"));
    

    /* Auto-adjust measurement interval */                                     
    delayMS = (sensor.min_delay / 1000) * 60 * 5;   // sensor.min_delay = 1e+6µs --> send val every 5 min

    /* Connect to WiFi network */     
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: "); Serial.println(WiFi.localIP());

}
/*****************************************************************************/




/******************************** main loop **********************************/
void loop() {
  
    /* Check WiFi connection status; attempt reconnect if disconnected */
    if (WiFi.status() != WL_CONNECTED) {
        
        Serial.println(F("WiFi lost — attempting reconnect..."));

        /* Try reconnect */
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        uint8_t retries = 0;
        /* Try six mintues to reconnect */
        while (WiFi.status() != WL_CONNECTED && retries < 600) {
            delay(1000);
            Serial.print(F("."));
            retries++;
        }

        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(F("WiFi reconnected successfully!"));
            Serial.print(F("IP address: ")); Serial.println(WiFi.localIP());
        } 
        else {
            Serial.println(F("WiFi reconnect failed"));

            Serial.println(F("restarting..."));
            delay(10000);  
            ESP.restart();
        }
    }



    /* Read temperature event */                                               
    sensors_event_t event;
    dht.temperature().getEvent(&event);

    if (isnan(event.temperature)) {
        Serial.println(F("Error reading temperature!"));
    } 
    else {
        temperature = event.temperature;
        Serial.print(F("Temperature: "));
        Serial.print(temperature);
        Serial.println(F(" °C"));
    }

    /* Read humidity event */                                                  
    dht.humidity().getEvent(&event);

    if (isnan(event.relative_humidity)) {
        Serial.println(F("Error reading humidity!"));
    } 
    else {
        humidity = event.relative_humidity;
        Serial.print(F("Humidity: "));
        Serial.print(humidity);
        Serial.println(F(" %"));
    }

    /* Transmit data to server */                                              
    if (WiFi.status() == WL_CONNECTED) {
        sendSensorData("temperature_msa_room", temperature);
        sendSensorData("humidity_msa_room", humidity);
    } else {
        Serial.println("WiFi not connected — data not sent");
    }


    /* Delay until next sensor cycle */                                        
    delay(delayMS);     

}



/************************** Function Definitions *****************************/
/***
 * void sendSensorData(const char* sensorType, float value) 
 * 
 * Sends a single measurement (temperature or humidity)
 * to the REST API backend via HTTP POST.
 * 
 * @param sensorType  String identifying the sensor type
 * @param value       Numeric measurement value
 * 
 * @return None
 * 
 * @note Adds API key in headers for authentication.
***/
void sendSensorData(const char* sensorType, float value) {

    /* Prepare HTTP client */                                                  
    HTTPClient http;
    http.begin(SERVER_URL);

    /* Configure headers */                                                   
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("X-API-Key", API_KEY);

    /* Assemble POST data */  
    String postData = "sensor_type=" + String(sensorType) + "&value=" + String(value, 2);

    /* Execute POST request */
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
        Serial.print("Server response: ");
        Serial.println(http.getString());
    } else {
        Serial.print("Transmission error: ");
        Serial.println(httpResponseCode);
    }

    /* Cleanup */
    http.end();
}


