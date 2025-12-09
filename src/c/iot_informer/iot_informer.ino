/******************************************************************************
 *
 * File:           iot_informer.ino
 * Author(s):      Mika Paul Salewski
 * Created:        2025-12-08
 * Last Updated:   2025-12-09
 * Version:        2025.12.09
 *
 * Title:
 *     Arduino IoT Informer — Bathroom Occupancy Display & I2C Informer
 *
 * Description:
 *     This firmware runs on an ESP32 and acts as an I2C slave to a master
 *     controller (e.g., Raspberry Pi). It receives occupancy updates for the
 *     main bathroom and displays the status on a 16x2 LCD with custom icons.
 *     Free/Occupied status and elapsed occupancy time are shown.
 *
 *     Key features:
 *         + I2C slave communication for sensor updates
 *         + 16x2 LCD display with custom emoji characters
 *         + Tracks first occupancy timing
 *         + Simple, robust design for embedded environments
 *
 * Copyright:
 *     (c) 2025, Mika Paul Salewski
 *
 * License:
 *     CC BY-NC-SA 4.0
 *
 * Notes:
 *     - I2C address: 0x08
 *     - WiFi not used in this module
 *     - Bathroom status codes:
 *         0 = OCCUPIED
 *         1 = FREE
 *     - Custom emoji characters stored in msa_lcd.h
 *
 *
 * The circuit:
 *  LCD RS pin to digital pin 12
 *  LCD Enable pin to digital pin 11
 *  LCD D4 pin to digital pin 5
 *  LCD D5 pin to digital pin 4
 *  LCD D6 pin to digital pin 3
 *  LCD D7 pin to digital pin 2
 *  LCD R/W pin to ground
 *  LCD VSS pin to ground
 *  LCD VCC pin to 5V
 *  10K resistor:
 *  ends to +5V and ground
 *  wiper to LCD VO pin (pin 3)
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
/* LCD pin mapping annd create instance */
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

/* Timer for occupancy duration */
unsigned long time_diff=0;

/* Bathroom status flags */
uint8_t bathroom_status = 0;
uint8_t bathroom_status_change = 0;

/* I2C input buffer */
String inputString = "";
bool stringComplete = false;


/************************** Function Declaration *****************************/
void requestEvent(void);
void receiveEvent(int howMany);
void processData(String data);


/**************************** Setup ******************************************/
void setup() {

    /* Initialize serial interface */         
    Serial.begin(9600);


    /* Initialize I2C interface */         
    Wire.begin(I2C_ADDRESS);        // Initialize Arduino as I2C slave
    Wire.onRequest(requestEvent);   // Register callback when master requests data
    Wire.onReceive(receiveEvent);   // Register callback when data is received by master            


    /* Initialize 16x2 LCD */
    lcd.begin(16, 2);
    lcd.clear();

    /* Print a message to the LCD. */
    lcd.print("Hey, Fuckers!");

    /* Create custom characters for emoji display */
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

    /* Process I2C input when a complete string has been received */
    if (stringComplete) {
        Serial.println("Received: " + inputString); 
        processData(inputString);
        inputString = "";
        stringComplete = false;
    }


    /* Update LCD display based on bathroom status */
    if(bathroom_status == BATHROOM_MAIN_FREE){
        
        /* Mark status change for timing */
        bathroom_status_change = 1;

        /* Display FREE status text */
        lcd.setCursor(0, 1);
        lcd.print("Feel Free    ");

        /* Display free toilet icon */
        lcd.setCursor(14, 0);
        lcd.write(byte(4));
        lcd.write(byte(5));
        lcd.setCursor(14, 1);
        lcd.write(byte(6));
        lcd.write(byte(7));

    }
    else{

        /* Track first entry timestamp after free */
        if(bathroom_status_change){
            bathroom_status_change = 0;
            time_diff=millis();
        }

        /* Display OCCUPIED status text with elapsed seconds */
        lcd.setCursor(0, 1);
        lcd.print("Shiting ");


        /* print the number of seconds since reset */
        lcd.print((millis()-time_diff) / 1000);
        lcd.print("s");

        /* Display occupied toilet icon */
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
 *     Callback executed when I2C master requests data.
 *     Currently placeholder; can send temperature/humidity data in future.
***/
void requestEvent(void) {
    // Placeholder: No data sent currently
}




/***
 * void receiveEvent(int howMany)
 * 
 * @brief
 *     Callback executed when data is received from I2C master.
 *     Buffers input until '#' termination character.
 *
 * @param howMany  Number of bytes received from master
 * 
 * @return None
 * 
 * @note 
 * 
***/
void receiveEvent(int howMany){
    
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
 * void processData(String data)
 * 
 * @brief
 *     Parses incoming sensor data and updates bathroom status.
 *     Handles 'apartment_traffic' and 'bathroom_main' sensor types.
 *
 * @param data  CSV string formatted as "sensor_type,value"
 * 
 * @return None
 * 
 * @note 
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


