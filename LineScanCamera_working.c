/* Line Scan Camera Test Program
 * 10/16/2016 : David Slaughter (first code)
 * 02/01/2017 : Thuy Nguyen (refactor)
 * 03/03/2017 : TN (displayed camera images to LCD with sampling, commented codes for auto-exposure for fast speed)
 * 
 * Note:
 *   - The TSL1401-DB contains a TAOS TSL1401R 128-pixel grayscale line scan camera 
 *     and a 7.9mm lens that provides a field of view equal to subject distance.
 *   - The circuit board of the TAOS TSL1401R line scan camera is translucent (backlight 
 *     will pass through the board) thus, you need to enclose the back of the camera to 
 *     prevent backlight from affecting the image.
 */

#include "simpletools.h"                      // Include simple tools
#include "adcDCpropab.h"                      // Include analog to digital header
#include "serial.h"

#define YES               1
#define NO                0

// for lights
#define PIN_LIGHT_1       10
#define PIN_LIGHT_2       11

// for LCD
#define PIN_LCD           5
#define LCD_LINE_LENGTH   21
serial *lcd;

// for line-scan camera
#define PIN_AO_A          0   // AO (analog pixel input from the sensor: 0 – Vdd, or tri-stated if beyond pixel 128)  // This is ANALOG pin 
#define PIN_SI_B          0   // SI (digital output to the sensor: begins a scan/exposure)
#define PIN_CLK_C         1   // CLK (digital output to the sensor: latches SI & clocks the pixels out)   // A, B, and C are DB-Expander pins

#define LINE_SCAN_LENGTH        128
#define CLOCKS_UNTIL_EXPOSURE   18
#define MAX_INTENSITY_ANALOG    4094

#define N                 34
#define M                 3  

#define INTENSITY_THR     2500

//unsigned int image[LINE_SCAN_LENGTH];
unsigned int image[LCD_LINE_LENGTH];


// --------------------------------------------------------------------

void lcdClear() {
  writeChar(lcd, 0x7C);
  writeChar(lcd, 0x00);
}  
  
void printImageToTerminal() {
  //for(int j = 0; j < LINE_SCAN_LENGTH; j++) 
  for(int j = 0; j < LCD_LINE_LENGTH; j++) 
    printf("%d ", image[j]);
  printf("\n");
} 

void printImageToLCD() {
  lcdClear();
  //for(int j = 0; j < LINE_SCAN_LENGTH; j++) 
  for(int j = 0; j < LCD_LINE_LENGTH; j++) {
    if (image[j] > INTENSITY_THR)
      writeStr(lcd, "-");
    else
      writeStr(lcd, "_");     
  } 
}  
   
void setup() {
  // initialize serial connections with peripheral devices (LCD)
  lcd = serial_open(PIN_LCD, PIN_LCD, 0, 115200);
      
  // initialize the Analog-Digital Convert (ADC)
  adc_init(21, 20, 19, 18); // uses the adcDCpropab.h header
  
  // turn on 2 lights
  //low(PIN_LIGHT_1);   high(PIN_LIGHT_1);
  //low(PIN_LIGHT_2);   high(PIN_LIGHT_2);
}  
  
// --------------------------------------------------------------------

int main() {
  setup();  
  
  unsigned int isInCaptureMode = NO;
  unsigned int expTime = 300 * 1E3; // camera exposure time in microsec // 80 is good when the lights are on, no auto-exposure needed
  unsigned int expTimeCnt = expTime * st_usTicks; // exposure time in system clock ticks  
  
  // Note for this loop:
  //   1. At first, isInCaptureMode = NO, PIN_SI_B and PIN_CLK_C are high to turn on the clock and start an integrate scan.
  //   2. PIN_SI_B is set to low, to start to snap a picture; however, at this time isInCaptureMode = NO, so just wait for exposure time to elapse
  //      Note that CLOCKS_UNTIL_EXPOSURE is 18, so the exposure time needs to compensate for this elapse time
  //   3. After waiting for an exposure time, isInCaptureMode is set to YES, then 128 pixels in a line scan are captured.
  // This code generally means 1 cycle to wait for exposure time, 1 cycle to capture picture, then loop back.
  while(1) {
    unsigned int lastTimeCnt = 0; // to record the start of the new image exposure period
    unsigned int maxIntensity = 0;
  
    high(PIN_SI_B); // clock in Scan N
        
    int c = 0;
    for(int j = 0; j < LINE_SCAN_LENGTH; j++) { // integrate Scan N
      high(PIN_CLK_C);
      
      if(j == 0) low(PIN_SI_B); // start to snap a picture
      if(isInCaptureMode == NO && j == CLOCKS_UNTIL_EXPOSURE) lastTimeCnt = CNT; // record the start of the new image exposure period (CNT: current system clock)
      
      if(isInCaptureMode == YES) { 
        if((N <= j && j <= LINE_SCAN_LENGTH - N) && ((j-N)%M == 0))
          image[c++] = adc_in(PIN_AO_A);           
        
        //image[j] = adc_in(PIN_AO_A); // takes ~300 ms for 128 pixels
        //if(image[j] > maxIntensity) maxIntensity = image[j]; // find max image intensity
      }      
      low(PIN_CLK_C);
    }
        
    if(isInCaptureMode == NO) {
      //printImageToTerminal();
      printImageToLCD();
             
      //while(CNT - lastTimeCnt < expTimeCnt) { } // wait for exposure time to elapse (CNT is current system lock)
      while(CNT - lastTimeCnt < expTime * st_usTicks) { } 
      
      isInCaptureMode = YES; 
      
    } else {
      //if (maxIntensity > MAX_INTENSITY_ANALOG)  expTime = expTime / 2;                    // auto exposure routine
      //else                                      expTime = expTime * 3300 / maxIntensity ; // Line Scan Camera Exposure time microseconds      
      //if (expTime > 150000) expTime = 150000;      
      //expTimeCnt = expTime * st_usTicks; // exposure time in system clock Ticks    
      
      isInCaptureMode = NO;
    }
  }    
}