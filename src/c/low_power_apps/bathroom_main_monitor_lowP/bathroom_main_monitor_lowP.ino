/******************************************************************************
 * 
 * File:           bathroom_monitor_lowP.ino
 * Author(s):      Mika Paul Salewski
 * Created:        2025-11-20
 * Last Updated:   2025-12-09
 * Version:        2025.12.09
 *
 * Title:
 *     ESP32 Bathroom Monitor — Low-Power Hall-Sensor Occupancy Detection
 *
 * Description:
 *     This firmware runs on an ESP32 in ultra-low-power mode.
 *     The microcontroller wakes up from deep sleep when the bathroom
 *     door changes state (hall sensor). It connects to WiFi briefly,
 *     transmits occupancy information to the REST API backend on a
 *     Raspberry Pi, and then re-enters deep sleep.
 *
 *     Low-power techniques:
 *         + Deep sleep between door events
 *         + WiFi active only during transmission
 *         + Reduced CPU frequency
 *         + Door state stored in RTC memory
 *
 * Copyright:
 *     (c) 2025, Mika Paul Salewski
 *
 * License:
 *    CC BY-NC-SA 4.0
 * 
 * Notes:
 *     - WiFi credentials and server API keys are stored inside secrets.h
 *       and are intentionally excluded from version control.
 *     - Only minimal processing is executed inside the ISR.
 *     - GPIO 0 / GPIO 2 should generally not be used on ESP32 hardware
 *       due to possible boot-mode interference.
 *     - Debug prints enabled with #define DEBUG
 *     - Wakeup:
 *         + Uses ext0 wakeup on GPIO14 (RTC-capable) to detect door state changes
 *         + Only one flanke per sleep is monitored
 *  
******************************************************************************/



/************************ Includes / Libraries *******************************/
#include <WiFi.h>
#include <HTTPClient.h>

#include "secrets.h"    // WiFi SSID/password + SERVER_URL + API_KEY




/*************************** Local Defines ***********************************/

/* Hall Sensor Configuration */
#define HALL_SENS_A_IN 34   // Hall-Sensor Analog Input [V] = f([T])
#define HALL_SENS_D_IN 14   // RTC GPIO input for door detection

#define HALL_DOOR_LOCKED 0 
#define HALL_DOOR_UNLOCKED 1 

/* WiFi & Sleep Configuration */
#define WIFI_CONNECT_TIMEOUT_MS 15000   // WiFi connection timeout in ms


/************************** Local Structure **********************************/
/* (Reserved for future struct definitions if needed) */   

/************************* Local Variables ***********************************/
/* RTC memory for door status */
RTC_DATA_ATTR uint8_t hall_status = HALL_DOOR_LOCKED;   // persists across deep sleep





/************************** Function Declaration *****************************/
/* Sends single sensor measurement to backend server */    
void sendSensorData(const char* sensorType, float value);

void wifi_send_data(uint8_t status);

/**************************** Setup ******************************************/
void setup() {

#ifdef DEBUG
    /* Initialize serial interface */                                          
    Serial.begin(115200);
#endif 

    /* Initialize Hall Sensor */
    pinMode(HALL_SENS_D_IN, INPUT);

    /* Read current hall state immediately */
    if(digitalRead(HALL_SENS_D_IN) == LOW) {
        hall_status = HALL_DOOR_UNLOCKED;
    }
    else{ 
        hall_status = HALL_DOOR_LOCKED;
    }


    /* WiFi only active to send event */
    wifi_send_data(hall_status);


    /* Configure GPIO as wakeup source from deep sleep */
    /* if hall_status = DOOR_UNLOCKED wait for 1 (HIGH) and viceversa */
    esp_sleep_enable_ext0_wakeup((gpio_num_t)HALL_SENS_D_IN, hall_status == HALL_DOOR_UNLOCKED ? 1 : 0);


    /* Enter deep sleep immediately after sending */
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
 * Sends a single measurement to the REST API backend via HTTP POST.
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
    } 
    else {
        Serial.print("Transmission error: ");
        Serial.println(httpResponseCode);
    }

    /* Cleanup */
    http.end();
}


/***
 * void wifi_send_data(uint8_t status)
 * 
 * Connects to WiFi, sends occupancy data, and disconnects to save power.
 * 
 * @param status  Current door status (locked/unlocked)
 * 
 * @return None
***/
void wifi_send_data(uint8_t status){

    /* Connect WiFi */
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(100);
    }

    if(WiFi.status() == WL_CONNECTED){
        sendSensorData("bathroom_main", (float)status);

        /* Disconnect immediately to save power */
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    } else {
        Serial.println("WiFi not connected — data not sent");
    }

}






