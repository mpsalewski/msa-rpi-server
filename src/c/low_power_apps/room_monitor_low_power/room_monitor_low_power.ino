/******************************************************************************
 *
 * File:           room_monitor_low_power.ino
 * Author(s):      Mika Paul Salewski
 * Created:        2025-12-09
 * Last Updated:   2025-12-09
 * Version:        2025.12.09
 *
 * Title:
 *     ESP32 Room Monitor — Low-Power Temperature & Humidity Sender
 *
 * Description:
 *     This firmware runs on an ESP32 using low-power techniques.
 *     The microcontroller periodically wakes up from deep sleep,
 *     powers the DHT11 sensor via a dedicated power GPIO,
 *     reads temperature/humidity values and transmits them to
 *     the Raspberry Pi backend using HTTP POST requests.
 *
 *     After transmission, the ESP32 switches WiFi off and re-enters
 *     deep sleep until the next measurement cycle.
 *
 * Copyright:
 *     (c) 2025, Mika Paul Salewski
 *
 * License:
 *    CC BY-NC-SA 4.0
 *
 * Notes:
 *     - WiFi credentials and API keys stored in secrets.h
 *     - Designed for battery powered environments
 *     - Low power techniques:
 *         + Deep sleep between measurements
 *         + Controlled sensor power switching
 *         + Minimal WiFi active time
 *         + Reduced CPU frequency
 *     - Avoid using GPIO 0/2 on ESP32 as they affect boot mode
 ******************************************************************************/



/************************ Includes / Libraries *******************************/
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include "secrets.h"    // WiFi SSID/password + SERVER_URL + API_KEY




/*************************** Local Defines ***********************************/
/* Sensor Configuration */
#define DHTPIN 4        // GPIO Pin für DHT Sensor
#define DHTTYPE DHT11   // DHT Sensor Typ
#define SENSOR_POWER_PIN    15          // external sensor power switch (through transistor)

/* Measurement schedule */
#define SLEEP_MINUTES 5                  // sleep duration
#define WIFI_CONNECT_TIMEOUT_MS 15000    // WiFi connection timeout




/************************** Local Structure **********************************/
/* (Reserved for future struct definitions if needed) */   




/************************* Local Variables ***********************************/
/* DHT unified driver instance */ 
DHT_Unified dht(DHTPIN, DHTTYPE);



/************************** Function Declaration *****************************/
/* Sends single sensor measurement to backend server */    
void sendSensorData(const char* sensorType, float value);



/**************************** Setup ******************************************/
void setup() {


    /* CPU frequency scaling for additional power saving */
    setCpuFrequencyMhz(80);     // reduce CPU frequency (default 240 MHz)

#ifdef DEBUG
    /* Initialize serial interface */                                          
    Serial.begin(115200);
#endif

    /* Power-switch pin configuration */
    pinMode(SENSOR_POWER_PIN, OUTPUT);
    digitalWrite(SENSOR_POWER_PIN, LOW);    // ensure sensor is initially off

    /* Activate DHT power */
    digitalWrite(SENSOR_POWER_PIN, HIGH);
    delay(2000);    // DHT11 warm-up time


    /* Re-enable DHT data line pull-up */
    pinMode(DHTPIN, INPUT_PULLUP);

    /* Initialize DHT sensor */                                                
    dht.begin();


    /* WiFi activation */
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(100);
    }


    /* Retrieve sensor metadata */                                             
    sensor_t sensor;

    dht.temperature().getSensor(&sensor);

    /* Read temperature */
    sensors_event_t event;
    float temperature = NAN;
    float humidity    = NAN;

    dht.temperature().getEvent(&event);
    if (!isnan(event.temperature)) {
        temperature = event.temperature;
    }

    /* Read humidity */
    dht.humidity().getEvent(&event);
    if (!isnan(event.relative_humidity)) {
        humidity = event.relative_humidity;
        
    }



    /* Transmit measurements only if WiFi was established */
    if (WiFi.status() == WL_CONNECTED) {

        if (!isnan(temperature)) {
            sendSensorData("temperature_msa_room", temperature);
        }

        if (!isnan(humidity)) {
            sendSensorData("humidity_msa_room", humidity);
        }

        /* Disconnect immediately to save power */
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }

    /* Prepare sensor shutdown */
    pinMode(DHTPIN, INPUT);      // disable pull-up and avoid backfeeding
    digitalWrite(DHTPIN, LOW);   // ensure no internal pull applied

    /* Power down sensor */
    digitalWrite(SENSOR_POWER_PIN, LOW);


    /* Configure deep sleep timer */
    uint64_t sleepDurationUS = (uint64_t)SLEEP_MINUTES * 60ULL * 1000000ULL;
    esp_sleep_enable_timer_wakeup(sleepDurationUS);

    /* Enter deep sleep */
    esp_deep_sleep_start();


}
/*****************************************************************************/




/******************************** main loop **********************************/
void loop() {
  
    /* Intentionally unused */
    /* Device sleeps in setup() */

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

#ifdef DEBUG
    if (httpResponseCode > 0) {
        Serial.print("Server response: ");
        Serial.println(http.getString());
    } else {
        Serial.print("Transmission error: ");
        Serial.println(httpResponseCode);
    }
#endif 

    /* Cleanup */
    http.end();
}


