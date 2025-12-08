/******************************************************************************
 * 
 * File:           apartment_traffic_detection.ino
 * Author(s):      Mika Paul Salewski
 * Created:        2025-12-08
 * Last Updated:   2025-12-08
 * Version:        2025.12.08
 *
 * Title:
 *     ESP32 Apartment Traffic Detection — Motion & Door based Entry/Exit
 *
 * Description:
 *     This firmware runs on an ESP32 and detects apartment traffic activity
 *     using an HC-SR501 PIR motion detector and a hall-effect door sensor.
 *     By analyzing motion before/after door events, the system identifies
 *     entry and exit directions. The detected events are transmitted to a
 *     Raspberry Pi REST backend via HTTP POST requests.
 * 
 * Copyright:
 *     (c) 2025, Mika Paul Salewski
 *
 * License:
 *    CC BY-NC-SA 4.0
 * 
 * Notes:
 *     - WiFi credentials and API keys are stored inside secrets.h
 *     - Runs fully event-driven using interrupts
 *     - Avoid using GPIO 0/2 on ESP32 as they affect boot mode
 *     - HC-SR501 can show false triggers during hardware warm-up
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
#define HALL_SENS_A_IN 34   // Hall-Sensor Analog Input [V] = f([T])
#define HALL_SENS_D_IN 16   // Hall-Sensor Digital Input 

/* Apartment Traffic States */
#define APARTMENT_TRAFFIC 1
#define APARTMENT_NO_TRAFFIC 0
#define APARTMENT_FINISHED_TRAFFIC 2

#define HC_SR501_MOTION 1
#define HC_SR501_NO_MOTION 0
#define HALL_DOOR_OPEN 1 

#define HALL_DOOR_CLOSED 0 
#define APARTMENT_ENTRY 0
#define APARTMENT_EXIT 1


/************************** Local Structure **********************************/
/* (Reserved for future struct definitions if needed) */   
struct apartment_traffic_s{
    uint8_t status = 0;
    uint8_t direction = 0;
}apartment_traffic;



/************************* Local Variables ***********************************/
/* HC-SR501 Status */ 
uint8_t hc_sr501_status = 0;    // 0 := no motion; 1 := motion

/* Hall Sensor Status */
uint8_t hall_status = 0;        // 0 := door closed; 1 := door open

/* Apartment Traffic Alert */
uint8_t apartment_traffic_status = 0;   // 0 := no traffic; 1 := traffic 

/* Measurement delay in milliseconds */  
uint32_t delayMS = 2000; 



/************************** Function Declaration *****************************/
/* Sends single sensor measurement to backend server */    
void sendSensorData(const char* sensorType, float value);

/* HC-SR501 Motion Detection ISR */
void hc_sr501_ISR(void);

/* Hall-Sensor Magnet Detection ISR */
void hall_sens_ISR(void);

/* WiFi lost connection handling */
void wifi_lost_con_handling(void);

/**************************** Setup ******************************************/
void setup() {


    /* Initialize serial interface */                                          
    Serial.begin(115200);

    /*** 
     * Initialize HC-SR501 sensor setup delay 
     * Datasheet: The device requires nearly a minute to initialize. During this 
     * period, it can and often will output false detection signals. 
     * µC needs to take this initialization period into consideration. 
    ***/         
    //pinMode(HC_SR501_PIN, INPUT_PULLUP);                                       
    pinMode(HC_SR501_PIN, INPUT);                                       
    delay(10000);

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
    /* HC-SR501-Sensor Motion Detection ISR */
    attachInterrupt(digitalPinToInterrupt(HC_SR501_PIN), hc_sr501_ISR, CHANGE);

    Serial.println("setup done ...");


}
/*****************************************************************************/




/******************************** main loop **********************************/
void loop() {
  

    /* Check WiFi connection status; attempt reconnect if disconnected */
    if (WiFi.status() != WL_CONNECTED) 
        wifi_lost_con_handling();


    /* Detect Apartment Traffic and analyze ENTRY or EXIT */
    if(apartment_traffic.status == APARTMENT_FINISHED_TRAFFIC){
        
        /* clear finished traffic IR */
        apartment_traffic.status = APARTMENT_NO_TRAFFIC;

        Serial.println("Apartment Traffic Finished");
        Serial.print("Apartment: ");
        if(apartment_traffic.direction == APARTMENT_ENTRY){
            Serial.println("ENTRY");
        }
        else{
            Serial.println("EXIT");
        }
         /* Transmit data to server */                                              
        if (WiFi.status() == WL_CONNECTED) {
            sendSensorData("apartment_traffic", (float)(apartment_traffic.direction));
        } else {
            Serial.println("WiFi not connected — data not sent");
        }
        
    }
   
#if 0
    /* Read HC_SR501_PIN */
    if(hc_sr501_status){
        Serial.println(F("Motion Detected"));
    }
    else{
        Serial.println(F("NO Motion"));
    }

    /* check if door is open */
    if(hall_status){
        Serial.println("DOOR OPEN");
    }
    else{
        Serial.println("DOOR CLOSED");        
    }
#endif
    

   


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
 * @brief Attempts to restore WiFi connection if disconnected.
 *
 * When WiFi is lost, the ESP32 tries reconnecting for up to 600 seconds.
 * If reconnection fails, the module performs a controlled system restart
 * to guarantee autonomous recovery and data availability.
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
 * @brief Interrupt Service Routine for hall-effect door sensor events.
 *
 * Triggered on door open/close. When a door event occurs while motion
 * has already been detected, this ISR interprets the event as EXIT.
 * The ISR only performs essential logic to keep interrupt time minimal.
 * 
 * @param void
 * 
 * @return None
 * 
 * @note Executed in interrupt context (keep code short).
***/
void hall_sens_ISR(void){


    
    /* analyze state */
    if (digitalRead(HALL_SENS_D_IN) == LOW) {
        
        hall_status = HALL_DOOR_OPEN;  
        /* movement before door is open --> indicates Exit */
        if(hc_sr501_status == HC_SR501_MOTION){
            apartment_traffic.status = APARTMENT_FINISHED_TRAFFIC;
            apartment_traffic.direction = APARTMENT_EXIT;
            /* clear IR */
            hall_status = HALL_DOOR_CLOSED;  
            hc_sr501_status = HC_SR501_NO_MOTION;
        }
        else{
            //apartment_traffic.status = APARTMENT_TRAFFIC;
        }

    } 
    else {
        hall_status = HALL_DOOR_CLOSED; // Door Closed
    }
 

}



/***
 * void hc_sr501_ISR(void)
 * 
 * @brief Interrupt Service Routine for HC-SR501 motion detection.
 *
 * Triggered on CHANGE when motion state switches HIGH or LOW.
 * When motion occurs after the hall sensor detected an open door,
 * the event is interpreted as apartment ENTRY.
 * 
 * @param void
 * 
 * @return None
 * 
 * @note HC-SR501 keeps signal HIGH for up to ~20s after detection.
 *       Afterwards LOW + ~3s lockout (hardware behavior).
***/
void hc_sr501_ISR(void){


    /* analyze state */
    if (digitalRead(HC_SR501_PIN) == HIGH) {
        hc_sr501_status = HC_SR501_MOTION;
        /* Door already open --> indicates Entrance */
        if(hall_status == HALL_DOOR_OPEN){
            apartment_traffic.status = APARTMENT_FINISHED_TRAFFIC;
            apartment_traffic.direction = APARTMENT_ENTRY;
            /* clear IR */
            hall_status = HALL_DOOR_CLOSED;  
            hc_sr501_status = HC_SR501_NO_MOTION;
        }
        else{
            //apartment_traffic.status = APARTMENT_TRAFFIC;
        }
        
    } 
    else {
        hc_sr501_status = HC_SR501_NO_MOTION;
    }
    

}
