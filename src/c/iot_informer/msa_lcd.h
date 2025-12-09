
#ifndef MSA_LCD_H
#define MSA_LCD_H


// Top-left (8x8, nur 5 rechte Bits werden genutzt)
byte unhappyTopLeft[8] = {
    B00100, 
    B00010, 
    B00000, 
    B00110, 
    B00110,
    B00000,
    B00000,
    B00000 
};

// Top-right (8x8)
    byte unhappyTopRight[8] = {
    B00100, 
    B01000,
    B00000, 
    B01100,
    B01100, 
    B00000, 
    B00000, 
    B00000  
};

// Bottom-left (8x8)
byte unhappyBottomLeft[8] = {
    B00000,
    B00011, 
    B00100, 
    B00100, 
    B00000, 
    B00000, 
    B00000, 
    B00000  
};

// Bottom-right (8x8)
byte unhappyBottomRight[8] = {

    B00000, 
    B11000, 
    B00100,
    B00100, 
    B00000, 
    B00000, 
    B00000, 
    B00000  
};




// Top-left (8x8, nur 5 rechte Bits werden genutzt)
byte happyTopLeft[8] = {
    B00000, 
    B00110, 
    B01000, 
    B00110, 
    B00110,
    B00000,
    B00000,
    B00000 
};

// Top-right (8x8)
byte happyTopRight[8] = {
    B00000, 
    B01100,
    B00010, 
    B01100,
    B01100, 
    B00000, 
    B00000, 
    B00000    
};

// Bottom-left (8x8)
byte happyBottomLeft[8] = {
    B00000, 
    B01111, 
    B01000, 
    B00100,
    B00011, 
    B00000, 
    B00000, 
    B00000  
};

// Bottom-right (8x8)
byte happyBottomRight[8] = {
    B00000, 
    B11110, 
    B00010, 
    B00100,
    B11000, 
    B00000, 
    B00000, 
    B00000  
};





#endif