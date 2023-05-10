// simple project to display rotating 3D cube on both the 128x64 SSD1306 OLED Display and the 16x2 LCD character display
// created by upir, 2023
// youtube channel: https://www.youtube.com/upir_upir

// full tutoral is here: https://youtu.be/IvMauAxWPkQ
// source files on GitHub: https://github.com/upiir/3d_on_character_display

// Links from the video:
// 3D OLED Cube tutorial: https://youtu.be/kBAcaA7NAlA
// 16x2 VFD display: https://s.click.aliexpress.com/e/_DmSNri1
// 16x2 LCD display: https://s.click.aliexpress.com/e/_Dlhenjx
// 16x2 OLED display: https://s.click.aliexpress.com/e/_Dd1pebJ
// Arduino UNO - https://s.click.aliexpress.com/e/_AXDw1h
// Arduino breadboard prototyping shield - https://s.click.aliexpress.com/e/_ApbCwx
// I2C Scanner sketch - https://playground.arduino.cc/Main/I2cScanner/
// Jumper wires: https://s.click.aliexpress.com/e/_DCbVWUt

// Other 16x2 character display tutorials:
// Arduino Gauge in 11 Minutes - https://youtu.be/upE17NHrdPc
// Smooth Arduino 16x2 Gauge - https://youtu.be/cx9CoGqpsfg
// DIY Battery Indicator - https://youtu.be/Mq0WBPKGRew
// 1 DISPLAY 3 SENSORS - https://youtu.be/lj_7UmM0EPY
// small display - BIG DIGITS - https://youtu.be/SXSujfeN_QI
// Smooth Gauge - No Custom Characters - https://youtu.be/MEhJtpkjwnc


#include "U8g2lib.h" // we are using u8g2 library for drawing on the OLED display
#include <LiquidCrystal_I2C.h> // if you don´t have I2C version of the display, use LiquidCrystal.h library instead

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
// note - if you don´t see anything on your character display, make sure to check the I2C address using the I2C scanner sketch

uint8_t upir_logo[] = {   // simple way how to define bitmap pictures for u8g2 library
B00010101, B11010111,     //  ░░░█░█░███░█░███
B00010101, B01000101,     //  ░░░█░█░█░█░░░█░█
B00010101, B10010110,     //  ░░░█░█░██░░█░██░
B00011001, B00010101      //  ░░░██░░█░░░█░█░█
};

// initialize the 128x64 SSD1306 OLED display
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0); // [page buffer, buffer size = 128 bytes (128*8/8)]

byte points[8][2]; // eight 2D points for the cube, values will be calculated in the code

int orig_points [8][3] = {  // eight 3D points - set values for 3D cube
{-1,-1, 1},
{1,-1,1},
{1,1,1},
{-1,1,1},
{-1,-1,-1},
{1,-1,-1},
{1,1,-1},
{-1,1,-1}
};

float rotated_3d_points [8][3];   // eight 3D points - rotated around Y axis
float angle_deg = 60.0;           // rotation around the Y axis
float z_offset = -4.0;            // offset of the cube on Z axis
float cube_size = 70.0;           // cube size (multiplier)
float time_frame;                 // ever increasing time value

byte copied_framebuffer[40]; // copied content of the u8g2 framebuffer, we need 20*16 pixels = 40 bytes
byte u8g2_current_page = 0; // current page for u8g2 page drawing loop, this display is redrawn using 8 pages sized 128x8px
unsigned long buffer_lines[16]; // variables holding the content of all 16 lines (required for custom characters)
byte lcd_custom_character[8]; // helper array for defining custom characters for 16x2 character display

void setup() {
  u8g2.begin(); // begin the u8g2 
  u8g2.setColorIndex(1); // set color to white

  Serial.begin(9600); // start serial communication for printing values from Arduino, only for debugging

  lcd.init();          // initialize the 16x2 lcd module
  lcd.backlight();     // enable backlight for the LCD module  

  // print custom characters on the 16x2 character display
  // this is only needed once, because after that, we are only changing the pixel content of those characters
  // since the display resolution is 16x2 and the cube takes 4x2 characters
  // we can either show 3 cubes with spacing or 4 cubes without spacing
  for (byte lcd_counter = 0; lcd_counter < 3; lcd_counter++) {

    lcd.setCursor(1 + lcd_counter * 5,0);              
    lcd.write(byte(0));  
    lcd.setCursor(2 + lcd_counter * 5,0);              
    lcd.write(byte(1));    
    lcd.setCursor(3 + lcd_counter * 5,0);              
    lcd.write(byte(2));  
    lcd.setCursor(4 + lcd_counter * 5,0);              
    lcd.write(byte(3));   

    lcd.setCursor(1 + lcd_counter * 5,1);              
    lcd.write(byte(4));  
    lcd.setCursor(2 + lcd_counter * 5,1);              
    lcd.write(byte(5));    
    lcd.setCursor(3 + lcd_counter * 5,1);              
    lcd.write(byte(6));  
    lcd.setCursor(4 + lcd_counter * 5,1);              
    lcd.write(byte(7)); 

  }

}

void loop() {

  time_frame++;                               // increase the time frame value by 1
  cube_size = 20 + sin(time_frame * 0.2)*6;   // oscilate cube size between values 30 - 70

  //cube_size = 56;
  cube_size = 17;

  // increase the angle by 5° increments
  if (angle_deg < 90-5) {
    angle_deg = angle_deg + 5;
  } else {
    angle_deg = 0;
  }

  // calculate the points
  for (int i=0; i<8; i++) {
    // rotate 3d points around the Y axis (rotating X nad Z positions)
    rotated_3d_points [i][0] = orig_points [i][0] * cos(radians(angle_deg)) - orig_points [i][2] * sin(radians(angle_deg));
    rotated_3d_points [i][1] = orig_points [i][1];
    rotated_3d_points [i][2] = orig_points [i][0] * sin(radians(angle_deg)) + orig_points [i][2] * cos(radians(angle_deg)) + z_offset;  

    // project 3d points into 2d space with perspective divide -- 2D x = x/z,   2D y = y/z
    points[i][0] = round(64 + rotated_3d_points [i][0] / rotated_3d_points [i][2] * cube_size);
    points[i][1] = round(32 + rotated_3d_points [i][1] / rotated_3d_points [i][2] * cube_size); 

  }


  u8g2.firstPage(); // start the u8g2 drawing loop
  u8g2_current_page = 0; // reset the current page counter
  do {

    // connect the lines between the individual points
    u8g2.drawLine(points[ 0 ][ 0 ], points[ 0 ][ 1 ] , points[ 1 ][ 0 ] , points[ 1 ][ 1 ] );  // connect points 0-1
    u8g2.drawLine(points[ 1 ][ 0 ], points[ 1 ][ 1 ] , points[ 2 ][ 0 ] , points[ 2 ][ 1 ] );  // connect points 1-2  
    u8g2.drawLine(points[ 2 ][ 0 ], points[ 2 ][ 1 ] , points[ 3 ][ 0 ] , points[ 3 ][ 1 ] );  // connect points 2-3      
    u8g2.drawLine(points[ 3 ][ 0 ], points[ 3 ][ 1 ] , points[ 0 ][ 0 ] , points[ 0 ][ 1 ] );  // connect points 3-0      

    u8g2.drawLine(points[ 4 ][ 0 ], points[ 4 ][ 1 ] , points[ 5 ][ 0 ] , points[ 5 ][ 1 ] );  // connect points 4-5
    u8g2.drawLine(points[ 5 ][ 0 ], points[ 5 ][ 1 ] , points[ 6 ][ 0 ] , points[ 6 ][ 1 ] );  // connect points 5-6  
    u8g2.drawLine(points[ 6 ][ 0 ], points[ 6 ][ 1 ] , points[ 7 ][ 0 ] , points[ 7 ][ 1 ] );  // connect points 6-7      
    u8g2.drawLine(points[ 7 ][ 0 ], points[ 7 ][ 1 ] , points[ 4 ][ 0 ] , points[ 4 ][ 1 ] );  // connect points 7-4  

    u8g2.drawLine(points[ 0 ][ 0 ], points[ 0 ][ 1 ] , points[ 4 ][ 0 ] , points[ 4 ][ 1 ] );  // connect points 0-4
    u8g2.drawLine(points[ 1 ][ 0 ], points[ 1 ][ 1 ] , points[ 5 ][ 0 ] , points[ 5 ][ 1 ] );  // connect points 1-5  
    u8g2.drawLine(points[ 2 ][ 0 ], points[ 2 ][ 1 ] , points[ 6 ][ 0 ] , points[ 6 ][ 1 ] );  // connect points 2-6      
    u8g2.drawLine(points[ 3 ][ 0 ], points[ 3 ][ 1 ] , points[ 7 ][ 0 ] , points[ 7 ][ 1 ] );  // connect points 3-7                 

    // draw upir logo 
    u8g2.drawBitmap(112, 0, 2, 4, upir_logo); 

    // printing some text was only for debugging purposes.. 
    /*u8g2.setFont(u8g2_font_5x7_tr); // set small font
    u8g2.drawStr(0,7,"Helo"); // draw helper text on the screen
    u8g2.drawStr(0,15,"Wrd!"); // draw helper text on the screen*/
    
    if (u8g2_current_page == 3 || u8g2_current_page == 4) { // when the cube is centered, it is on pages 3 and 4
      
      memcpy(&copied_framebuffer[(u8g2_current_page-3)*20], u8g2.getBufferPtr()+54, 20); // copy 20 bytes (20*8 pixels) from the framebuffer
 
      for (byte y = 0; y < 8; y++) { // for every bit in the byte
        for (byte x = 0; x < 20; x++) { // for every byte in the array
          if (bitRead(copied_framebuffer[x + (u8g2_current_page-3) * 20], y) == 1) { // if the bit is 1
            bitSet(buffer_lines[y + (u8g2_current_page-3)*8], 31-x); // set the bit to 1
            //Serial.print("X"); // print X if the bit is 1
          }
          else {
            bitClear(buffer_lines[y + (u8g2_current_page-3)*8], 31-x); // set the bit to 0
            //Serial.print("."); // print dot if the bit is 0
          }
        }
        //Serial.println(""); // jump to next line after every byte
      }
    }

    u8g2_current_page++; // increase the current page counter

    } while ( u8g2.nextPage() ); // u8g2 page drawing loop



/*
    // print the content of the variable buffer_lines
    // this is for debugging only

    for (byte y = 0; y < 16; y++) { // for every bit in the byte
      for (byte x = 0; x < 32; x++) { // for every byte in the array
        if (bitRead(buffer_lines[y], x) == 1) { // if the bit is 1
          Serial.print("X"); // print X if the bit is 1
        }
        else {
          Serial.print("."); // print dot if the bit is 0
        }
      }
      Serial.println(""); // jump to next line after every byte
    }
*/


  for (byte char_id = 0; char_id < 8; char_id++) {  // create 8 custom characters
    for (byte char_line = 0; char_line < 8; char_line++) { // set 8 lines for every custom character
      lcd_custom_character[char_line] = buffer_lines[char_line + ((char_id/4)*8)] >> 27 - ((char_id % 4) * 5);
    }
    // create a special character for the character display on position char_id
    lcd.createChar(char_id, lcd_custom_character);  
  } 

  // and that´s it :)
}
