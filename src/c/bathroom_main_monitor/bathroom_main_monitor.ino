/******************************************************************************
 * 
 * File:           bathroom_monitor.ino
 * Author(s):      Mika Paul Salewski
 * Created:        2025-11-20
 * Last Updated:   2025-11-27
 * Version:        2025.11.20
 *
 * Title:
 *     ESP32 Bathroom Monitor — Hall-Sensor Based Occupancy Detection
 *
 * Description:
 *     This firmware runs on an ESP32 and monitors a bathroom door using a
 *     magnetic hall-effect sensor. When door lock/unlock events occur,
 *     the system transmits occupancy information (FREE / OCCUPIED) to a
 *     REST backend API running on a Raspberry Pi.
 *
 *     The implementation uses interrupts to provide immediate response
 *     to hall-sensor state changes and avoids polling whenever possible.
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
 *  
******************************************************************************/



/************************ Includes / Libraries *******************************/
#include <WiFi.h>
#include <HTTPClient.h>

#include "secrets.h"    // WiFi SSID/password + SERVER_URL + API_KEY




/*************************** Local Defines ***********************************/

/* Hall Sensor Configuration */
#define HALL_SENS_A_IN 34   // Hall-Sensor Analog Input [V] = f([T])
#define HALL_SENS_D_IN 16   // Hall-Sensor Digital Input 

#define HALL_DOOR_LOCKED 0 
#define HALL_DOOR_UNLOCKED 1 



/************************** Local Structure **********************************/
/* (Reserved for future struct definitions if needed) */   
struct priv_s {
    
    /* Hall Sensor Status */
    uint8_t hall_status = 0;        // 0 := door closed; 1 := door open
    uint8_t hall_event = 0;

}priv;

/************************* Local Variables ***********************************/


/* Measurement delay in milliseconds */  
uint32_t delayMS = 2000; 



/************************** Function Declaration *****************************/
/* Sends single sensor measurement to backend server */    
void sendSensorData(const char* sensorType, float value);

/* Hall-Sensor Magnet Detection ISR */
void hall_sens_ISR(void);

/* WiFi lost connection handling */
void wifi_lost_con_handling(void);

/**************************** Setup ******************************************/
void setup() {


    /* Initialize serial interface */                                          
    Serial.begin(115200);


    /* Initialize Hall Sensor */
    //pinMode(HALL_SENS_D_IN, INPUT_PULLUP);
    pinMode(HALL_SENS_D_IN, INPUT);



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



    /***
    * Initialize Interrupts
    ***/
    /* Hall-Sensor Magnet Detection ISR */
    attachInterrupt(digitalPinToInterrupt(HALL_SENS_D_IN), hall_sens_ISR, CHANGE);


    Serial.println("setup done ...");


}
/*****************************************************************************/




/******************************** main loop **********************************/
void loop() {
  

    /* Check WiFi connection status; attempt reconnect if disconnected */
    if (WiFi.status() != WL_CONNECTED) 
        wifi_lost_con_handling();


    /* hall event handling */
    if(priv.hall_event){
        
        /* clear event */
        priv.hall_event = 0;

        Serial.print("Main Bathroom: ");
        if(priv.hall_status == HALL_DOOR_UNLOCKED){
            Serial.println("FREE");
        }
        else{
            Serial.println("OCCUPIED");
        }
        
        /* Transmit data to server */                                              
        if (WiFi.status() == WL_CONNECTED) {
            sendSensorData("bathroom_main", (float)(priv.hall_status));
        } else {
            Serial.println("WiFi not connected — data not sent");
        }
        
    }
   


    /* Delay until next sensor cycle */                                        
    delay(delayMS);     

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
 * void wifi_lost_con_handling(void)
 * 
 * @brief Attempts to restore WiFi connectivity when disconnected.
 *
 * The ESP32 tries reconnecting for up to 600 seconds. If unsuccessful,
 * the module performs a controlled reboot to maintain autonomous
 * recovery and guarantee continued status reporting.
 * 
 * @param void
 * 
 * @return None
 * 
 * @note 
***/
void wifi_lost_con_handling(void){

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
        Serial.print(F("IP address: ")); 
        Serial.println(WiFi.localIP());
    } 
    else {
        Serial.println(F("WiFi reconnect failed"));

        Serial.println(F("restarting..."));
        delay(10000);  

        /***
         * RESTART ESP32 
        ***/
        ESP.restart();
    }


}







/***
 * void hall_sens_ISR(void)
 * 
 * @brief Interrupt Service Routine for hall-effect magnet events
 *        indicating bathroom lock/unlock state changes.
 *
 * Triggered when digital hall sensor changes from HIGH to LOW or
 * vice versa. The ISR only stores the current door state and sets
 * an event flag processed by the main loop.
 * 
 * @param void
 * 
 * @return None
 * 
 * @note 
***/
void hall_sens_ISR(void){

    /* set event */
    priv.hall_event = 1;
    
    /* analyze state */
    if (digitalRead(HALL_SENS_D_IN) == LOW) {
        
        priv.hall_status = HALL_DOOR_UNLOCKED;  

    } 
    else {
        priv.hall_status = HALL_DOOR_LOCKED; 
    }
 

}

