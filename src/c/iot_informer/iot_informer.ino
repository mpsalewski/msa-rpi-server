/******************************************************************************
 * 
 * File:           iot_informer.ino
 * Author(s):      Mika Paul Salewski
 * Created:        2025-12-08
 * Last Updated:   2025-12-09
 * Version:        2025.12.09
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
 *
 *
 *    The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 *
******************************************************************************/



/************************ Includes / Libraries *******************************/
#include <LiquidCrystal.h>
#include "msa_lcd.h"
#include <Wire.h>


/*************************** Local Defines ***********************************/
/* I2C Slave address */
#define I2C_ADDRESS 0x08    

#define BATHROOM_MAIN_OCCUPIED 0 
#define BATHROOM_MAIN_FREE 1 

/************************** Local Structure **********************************/
/* (Reserved for future struct definitions if needed) */   



/************************* Local Variables ***********************************/
/* 
 * initialize the library by associating any needed LCD interface pin 
 * with the arduino pin number it is connected to
*/
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

unsigned long time_diff=0;

uint8_t bathroom_status = 0;
uint8_t bathroom_status_change = 0;


String inputString = "";
bool stringComplete = false;


/************************** Function Declaration *****************************/
void requestEvent(void);
void receiveEvent(int howMany);



/**************************** Setup ******************************************/
void setup() {

    /* Initialize serial interface */         
    Serial.begin(9600);


    /* Initialize I2C interface */         
    Wire.begin(I2C_ADDRESS);        // Initialize Arduino as I2C slave
    Wire.onRequest(requestEvent);   // Register callback when master requests data
    Wire.onReceive(receiveEvent);   // Register callback when data is received by master            


    /* Initialize LCD */
    lcd.begin(16, 2);
    lcd.clear();

    /* Print a message to the LCD. */
    lcd.print("Hey, Fuckers!");

    // Create the 4 custom characters for the emoji
    lcd.createChar(0, unhappyTopLeft);
    lcd.createChar(1, unhappyTopRight);
    lcd.createChar(2, unhappyBottomLeft);
    lcd.createChar(3, unhappyBottomRight);
    
    lcd.createChar(4, happyTopLeft);
    lcd.createChar(5, happyTopRight);
    lcd.createChar(6, happyBottomLeft);
    lcd.createChar(7, happyBottomRight);


}
/*****************************************************************************/



/******************************** main loop **********************************/
void loop() {

    /* check if new data available*/
    if (stringComplete) {
        Serial.println("Received: " + inputString); 
        processData(inputString);
        inputString = "";
        stringComplete = false;
    }

    if(bathroom_status == BATHROOM_MAIN_FREE){
        
        /* indicate bathroom status change for counter in occupied */
        bathroom_status_change = 1;

        /* set free display version on bottom row on lcd */
        lcd.setCursor(0, 1);
        lcd.print("Feel Free    ");

        /* set free toilet icon */
        lcd.setCursor(14, 0);
        lcd.write(byte(4));
        lcd.write(byte(5));
        lcd.setCursor(14, 1);
        lcd.write(byte(6));
        lcd.write(byte(7));

    }
    else{

        /* check if this is the first entry after free */
        if(bathroom_status_change){
            bathroom_status_change = 0;
            time_diff=millis();
        }

        /* set occupied display version on bottom row on lcd */
        lcd.setCursor(0, 1);
        lcd.print("Shiting ");


        // print the number of seconds since reset:
        lcd.print((millis()-time_diff) / 1000);
        lcd.print("s");

        /* set occupied toilet icon */
        lcd.setCursor(14, 0);
        lcd.write(byte(0)); 
        lcd.write(byte(1)); 
        lcd.setCursor(14, 1);
        lcd.write(byte(2)); 
        lcd.write(byte(3)); 

    }

    delay(100);

}


/************************** Function Definitions *****************************/
/***
 * void requestEvent(void)
 * 
 * @brief 
 *   
 * Callback executed when master requests data 
 *
 * @param sensorType  String identifying the sensor type
 * @param value       Numeric measurement value
 * 
 * @return None
 * 
 * @note Adds API key in headers for authentication.
***/
void requestEvent(void) {
#if 0
    // Prepare data as CSV: "temperature,humidity"
    char buffer[40];
    char t[10], h[10];

    dtostrf(temperature, 0, 2, t);
    dtostrf(humidity, 0, 2, h);

    int len = sprintf(buffer, "%s,%s#", t, h);
    buffer[len] = '\0';

    Serial.println(buffer);
    Wire.write((uint8_t*)buffer, len);

#endif 
}




/***
 * void receiveEvent(void) 
 * 
 * @brief 
 *   
 * Callback executed when data is received from master 
 *
 * @param sensorType  String identifying the sensor type
 * @param value       Numeric measurement value
 * 
 * @return None
 * 
 * @note Adds API key in headers for authentication.
***/
void receiveEvent(int howMany){
    //inputString = "";   
    while (Wire.available()) {
    
        char c = Wire.read();
        
        if (c == '#') {
            stringComplete = true;
        } 
        else {
            inputString += c;
        }
    }
}



/***
 * void receiveEvent(void) 
 * 
 * @brief 
 *   
 * Callback executed when data is received from master 
 *
 * @param sensorType  String identifying the sensor type
 * @param value       Numeric measurement value
 * 
 * @return None
 * 
 * @note Adds API key in headers for authentication.
***/
void processData(String data) {

    /* check if the message contains a comma */
    int commaIndex = data.indexOf(',');
    if (commaIndex == -1) {
        Serial.println("invalid data: " + data);
        return;
    }

    /* extract numeric value from data */
    String sensorType = data.substring(0, commaIndex);

    /* extract value */ 
    float value = data.substring(commaIndex + 1).toFloat();

    /* handle different sensor types */
    /* apartment_traffic */
    if (sensorType == "apartment_traffic") {
        if (value == 1.0) {
            Serial.println("Apartment EXIT!");
        } 
        else if (value == 0.0) {
            Serial.println("Apartment ENTRY!");
        } 
        else {
            Serial.println("invalid value for apartment_traffic: " + String(value));
        }
    } 
    /* bathroom_main */
    else if (sensorType == "bathroom_main") {
        if (value == 1.0) {
            Serial.println("Main Bathroom is FREE!");
        } 
        else if (value == 0.0) {
            Serial.println("Main Bathroom is OCCUPIED!");
        } 
        else {
            Serial.println("invalid value for bathroom_main: " + String(value));
        }
        bathroom_status = (uint8_t)value;
    } 
    else {
        Serial.println("unregistered sensor_type: " + sensorType);
    }
}


