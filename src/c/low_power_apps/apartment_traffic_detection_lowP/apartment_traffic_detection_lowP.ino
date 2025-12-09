/******************************************************************************
 *
 * File:           apartment_traffic_detection_lowP.ino
 * Author(s):      Mika Paul Salewski
 * Created:        2025-12-08
 * Last Updated:   2025-12-09
 * Version:        2025.12.09
 *
 * Title:
 *     ESP32 Apartment Traffic — Low-Power Entry/Exit Detection
 *
 * Description:
 *     Ultra low-power firmware for detecting apartment entry and exit using
 *     a PIR motion sensor and a hall-effect door sensor. Direction is derived
 *     by evaluating which event occurred first (motion→door = EXIT,
 *     door→motion = ENTRY). Results are sent to a REST backend via HTTP POST.
 *
 *     The ESP32 stays in deep sleep and only wakes on sensor events or when
 *     the sequence timeout expires. RTC memory is used to persist minimal
 *     state across deep sleep cycles.
 *
 * Copyright:
 *     (c) 2025, Mika Paul Salewski
 *
 * License:
 *     CC BY-NC-SA 4.0
 *
 * Notes:
 *     - WiFi credentials and API keys in secrets.h
 *     - Low-power:
 *         + Deep sleep between events
 *         + WiFi active only during transmission
 *         + RTC flags for event sequencing
 *     - HC-SR501 may keep HIGH for several seconds
 *     - Use RTC-capable GPIOs for wake sources
 *     - Avoid GPIO 0/2 as they affect boot mode
 *
 ******************************************************************************/




/************************ Includes / Libraries *******************************/
#include <WiFi.h>
#include <HTTPClient.h>

#include "secrets.h"    // WiFi SSID/password + SERVER_URL + API_KEY




/*************************** Local Defines ***********************************/
/* HC-SR501 Sensor Configuration */
#define HC_SR501_PIN 4      

/* Hall Sensor Configuration */
//#define HALL_SENS_A_IN 34   // Hall-Sensor Analog Input [V] = f([T])
#define HALL_SENS_D_IN 14   // RTC GPIO input for door detection


/* Apartment Traffic States */
#define APARTMENT_NO_TRAFFIC 0
#define APARTMENT_FINISHED_TRAFFIC 1

#define HC_SR501_MOTION 1
#define HC_SR501_NO_MOTION 0

#define HALL_DOOR_OPEN 1 
#define HALL_DOOR_CLOSED 0 

#define APARTMENT_ENTRY 0
#define APARTMENT_EXIT 1

#define MAX_EVENT_INTERVAL_MS 15000 // Maximum allowed interval between door and motion events (15s)
#define WIFI_CONNECT_TIMEOUT_MS 15000  // WiFi connection timeout in ms

#define DEBUG 1

/************************** Local Structure **********************************/
/* (Reserved for future struct definitions if needed) */   


/************************* Local Variables ***********************************/
/* Persist across deep sleep so sequences are tracked */
RTC_DATA_ATTR uint8_t rtc_motion_flag = 0;    // set when motion was seen and waiting for door
RTC_DATA_ATTR uint8_t rtc_door_flag = 0;      // set when door opened and waiting for motion



/************************** Function Declaration *****************************/
/* Sends single sensor measurement to backend server */    
void sendSensorData(const char* sensorType, float value);

/* Connect to WiFi and call sendSensorData */
void wifi_send_data(uint8_t status);


/* Helpers */
static inline void clear_sequence_flags() {
    rtc_motion_flag = HALL_DOOR_CLOSED;
    rtc_door_flag = HC_SR501_NO_MOTION;
}


/**************************** Setup ******************************************/
void setup() {

#ifdef DEBUG
    Serial.begin(115200);
#endif

    /* Initialize sensors */
    pinMode(HC_SR501_PIN, INPUT);                                       
    pinMode(HALL_SENS_D_IN, INPUT);

 
    /* Determine cause of wake */
    esp_sleep_wakeup_cause_t wake_cause = esp_sleep_get_wakeup_cause();

    /* small debounce: allow sensor pin to stabilise after wake */
    delay(20); 

    /* Process the event that caused wake (if any) */
    bool event_sent = false;

    /* ext0 wake: hall pin reached configured level -> treat as door event */    
    if(wake_cause == ESP_SLEEP_WAKEUP_EXT0) {

        int hall_level = digitalRead(HALL_SENS_D_IN);

        /* Door opened event */
        if(hall_level == LOW) {

            /* If motion was already detected earlier and within window -> EXIT */
            if(rtc_motion_flag) {        
                    wifi_send_data(APARTMENT_EXIT);
                    clear_sequence_flags();
                    event_sent = true;
                    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
            } 
            else {
                /* No prior motion -> set door flag (waiting for motion after door) */
                rtc_door_flag = HALL_DOOR_OPEN;
                esp_sleep_enable_timer_wakeup((uint64_t)MAX_EVENT_INTERVAL_MS * 1000ULL);
            }
        } 
        else {
            /* Door closed again without prior motion -> cancel any pending sequence flags */
            clear_sequence_flags();
        }
    }
    /* ext1 wake: PIR HIGH detected -> motion event */
    else if(wake_cause == ESP_SLEEP_WAKEUP_EXT1) {
        
        int pir_level = digitalRead(HC_SR501_PIN);

        /* Motion detected */
        if(pir_level == HIGH) {
             
            /* If door flag was previously set and within interval -> ENTRY */
            if(rtc_door_flag) {
                wifi_send_data(APARTMENT_ENTRY);
                clear_sequence_flags();
                event_sent = true;
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
            } 
            else {
                /* No prior door -> set motion flag (waiting for door) */
                rtc_motion_flag = HC_SR501_MOTION;
                esp_sleep_enable_timer_wakeup((uint64_t)MAX_EVENT_INTERVAL_MS * 1000ULL);
            }
            #ifdef DEBUG
                Serial.print("RTC Motion Flag ");
                Serial.println(rtc_motion_flag);
            #endif
        } 
        else {
            /* No Motion without prior open door -> cancel any pending sequence flags */
            clear_sequence_flags();
        }
    }
    /* Timer expired -> sequence window ended */
    else if (wake_cause == ESP_SLEEP_WAKEUP_TIMER) {
        /* clear flags */
        clear_sequence_flags();
    }
    else {
        /* Cold be boot or other wake reason — clear flags to avoid false positives */
        clear_sequence_flags();
    }

    /* get curent states to set wakeup calls */
    int cur_hall = digitalRead(HALL_SENS_D_IN); // LOW == open in your wiring
    int cur_pir  = digitalRead(HC_SR501_PIN);  // HIGH == motion


    /* wake up logic to detect both edges */
    int ext0_wake_level = (cur_hall == LOW) ? 1 : 0; 
    esp_sleep_enable_ext0_wakeup((gpio_num_t)HALL_SENS_D_IN, ext0_wake_level);

    /* 
     * always detect rising edge; PIR is igh for ~20s 
     * cleared by wakeup timer if ENTRY / EXIT 
    */
    if (cur_pir == LOW) {
        esp_sleep_enable_ext1_wakeup(1ULL << HC_SR501_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);
    } 
    else {
        // do not enable ext1 now; wait for PIR to go LOW (or timer) before enabling
        // bc of timer no need to look for falling edge
    }

    /* Go to deep sleep */
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
        sendSensorData("apartment_traffic", (float)status);

        /* Disconnect immediately to save power */
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    } else {
        Serial.println("WiFi not connected — data not sent");
    }

}


