/*
  LiquidCrystal Library - Hello World

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "Hello World!" to the LCD
 and shows the time.

  The circuit:
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

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 modified 7 Nov 2016
 by Arturo Guadalupi

 This example code is in the public domain.

 https://docs.arduino.cc/learn/electronics/lcd-displays

*/

// include the library code:
#include <LiquidCrystal.h>
#include "msa_lcd.h"



// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

unsigned long time_diff=0;


void setup() {


    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    lcd.clear();

    // Print a message to the LCD.
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


void loop() {

   

    time_diff=millis();
    for(int i = 0;i <26; i++){
        
        

         // set the cursor to column 0, line 1
        // (note: line 1 is the second row, since counting begins with 0):
        lcd.setCursor(0, 1);

        lcd.print("Shiting ");

        // print the number of seconds since reset:
        lcd.print((millis()-time_diff) / 1000);
        lcd.print("s");

        // show big icon at right edge (columns 14+15)
        
        lcd.setCursor(14, 0);
        lcd.write(byte(0)); // top-left
        lcd.write(byte(1)); // top-right
        lcd.setCursor(14, 1);
        lcd.write(byte(2)); // bottom-left
        lcd.write(byte(3)); // bottom-right
         delay(250);

    }
    for(int i = 0;i <26; i++){

        
        lcd.setCursor(0, 1);
        //if()

        lcd.print("Feel Free    ");

        // Non-Shitting Icon
        lcd.setCursor(14, 0);
        lcd.write(byte(4));
        lcd.write(byte(5));

        lcd.setCursor(14, 1);
        lcd.write(byte(6));
        lcd.write(byte(7));
        
        delay(250);
    }

    


}
