/* Screen and touch defs 
*/
#ifndef  MYSCREENDEFS_H
#define  MYSCREENDEFS_H

#include "SPI.h"
#include "Adafruit_GFX.h"

#include <XPT2046_Touchscreen.h>

#ifdef ILI9341 
#define sx(x)	((short)(x))	// no scaling
#define sy(x)	((short)(x))	// no scaling
#include "Adafruit_ILI9341.h"
Adafruit_ILI9341 tft = Adafruit_ILI9341(LTFT_CS, LTFT_DC);
//Adafruit_ILI9341 tft = Adafruit_ILI9341(LTFT_CS, LTFT_DC, LTFT_RST);
//Adafruit_ILI9341 tft = Adafruit_ILI9341(LTFT_CS, LTFT_DC, LTFT_MOSI, LTFT_CLK, LTFT_RST, LTFT_MISO);
#endif

#ifdef ILI9488
// see also  myInst.h
#define sx(x)	((short)(1.5 * (x)))	  // scale by 480/320 for bigger display
#define sy(x)	((short)(1.333 * (x))) // 320/240
Arduino_DataBus *bus = new Arduino_ESP32SPI(LTFT_DC, LTFT_CS, LTFT_CLK /* SCK */, LTFT_MOSI /* MOSI */, -1 /* MISO */, VSPI /* spi_num */);
Arduino_ILI9488_18bit tft =  Arduino_ILI9488_18bit(bus, LTFT_RST, 0, false /* IPS */);
#endif

#ifdef ILI9488
	//#include "myFonts/DejaVu_Sans_Mono_Bold_16.h"
	#include "myFonts/DejaVu_Sans_Mono_Bold_18.h"
	#include "myFonts/DejaVu_Sans_Mono_Bold_22.h"
	//#include "myFonts/DejaVu_Sans_Mono_Bold_24.h"
	//#include "myFonts/DejaVu_Sans_Mono_Bold_26.h"
	#include "myFonts/DejaVu_Sans_Mono_Bold_32.h"
	#include "myFonts/DejaVu_Sans_Mono_Bold_42.h"
	
	#define FONT0 DejaVu_Sans_Mono_Bold_18  // smallest
	#define FONT1 DejaVu_Sans_Mono_Bold_22  // small
	//#define FONT2 DejaVu_Sans_Mono_Bold_22  // larger
	//#define FONT2 DejaVu_Sans_Mono_Bold_24  // larger
	#define FONT2 DejaVu_Sans_Mono_Bold_32  // larger
	//#define FONT2 DejaVu_Sans_Mono_Bold_28  // larger
	#define FONT3 DejaVu_Sans_Mono_Bold_42        // biggest
#else // ILI9341
	#include "myFonts/DejaVu_Sans_Mono_Bold_16.h"
	#include "myFonts/DejaVu_Sans_Mono_Bold_18.h"
	//#include "myFonts/DejaVu_Sans_Mono_Bold_22.h"
	#include "myFonts/DejaVu_Sans_Mono_Bold_24.h"
	//#include "myFonts/DejaVu_Sans_Mono_Bold_26.h"
	#include "myFonts/DejaVu_Sans_Mono_Bold_28.h"
	//	#include "myFonts/DejaVu_Sans_Mono_Bold_32.h"
	//#include "myFonts/DejaVu_Sans_Mono_Bold_38.h"
	
	#define FONT0 DejaVu_Sans_Mono_Bold_16  // smallest
	#define FONT1 DejaVu_Sans_Mono_Bold_18  // small
	//#define FONT2 DejaVu_Sans_Mono_Bold_22  // larger
	#define FONT2 DejaVu_Sans_Mono_Bold_24  // larger
	//#define FONT2 DejaVu_Sans_Mono_Bold_26  // larger
	#define FONT3 DejaVu_Sans_Mono_Bold_28  // larger
	//#define FONT3 DejaVu_Sans_Mono_Bold_32  // biggest
	//#define FONT3 DejaVu_Sans_Mono_Bold_38        // biggest
#endif

#define NUMFONTS 4
const GFXfont *myFonts[NUMFONTS] = {&FONT0, &FONT1, &FONT2, &FONT3};

#define MY_DARKGREEN 0x034E0
#define MY_RED 0xF800
#define MY_WHITE 0xFFFF
#define MY_YELLOW 0xFFE0
#define MY_GREEN RGB565(0,36,0)
#define RGB565(R, G, B) (((R)<<11)|((G)<<5)|(B))  
#define LIGHTYELLOW RGB565(31,63,0)
#define YELLOW_VL RGB565(31,63,10)
#define CYAN_L RGB565(15,45,31)
#define CYAN_VL RGB565(21,63,31)
#define CYAN_M RGB565(8,32,24)
#define GREEN_L RGB565(5,63,5)
#define CYAN_M RGB565(8,32,24)
#define PINK_M RGB565(31,28,31)
#define MY_ORANGE RGB565(31,31,0)	// R + G/2
// special colours
#define VOLT_COL LIGHTYELLOW
#define AMP_COL GREEN_L
#define WATT_COL PINK_M
#define RES_COL CYAN_L
#define TEMP_COL 0x0F9CB
// screen background
#define BGCOL 0 
#define BGHIGH RGB565(5,11,4)  
#define MIDBG RGB565(5,11,4) 
// value text
#define TEXTCOL  MY_YELLOW
#define HIGHLIGHTCOL MY_WHITE
//menu button items
#define BTEXTCOL RGB565(20,36,20) //light grey
#define BHIGHCOL 0xffff  //LIGHTYELLOW // MY_YELLOW // ILI9341_BLACK 

// menu button backgrounds
#define BBORDHIGH BHIGHCOL
#define BBORD BGCOL
#define BBGHIGH RGB565(0,14,28) // MED blue
#define BBXHIGH RGB565(0,0,20)  // DK blue
#define BBSHIGH RGB565(20,10,0) // MED ORANGE
// mode buttons
#define BONCOL MY_WHITE
#define BBMHIGH RGB565(0,45,0) // bright green
#define BBMXHIGH RGB565(3,20,0) // med green
// cal buttons
#define BBRHIGH RGB565(31,0,0)
#define BBRXHIGH RGB565(15,0,0)

// error screen backgrounds
#define ERR_BG_A MY_DARKGREEN
#define ERR_BG_B MY_RED

// indicator BG colours
#define ITRIG MY_RED 			// Action triggered
#define IENABLE	MY_DARKGREEN	// Function enabled
#define IND_ONC MY_GREEN
#define IND_ONLC GREEN_L		// FAN medium
#define IND_OFFC MY_RED			// FAN High
#define IND_GO_COL RGB565(0,20,32)
// screen calibration 
struct screenCals {
  int16_t thmin; // tft.pixel (0,0)
  int16_t tvmin;
  int16_t thmax;// tft.pixel (HMAX,VMAX)
  int16_t tvmax;
  uint8_t tsRot;	// screen rotation - as some touch screens are rotated 180%
  uint8_t scrRot;  // screen rotation - as some touch screens are rotated 180%
} ;
screenCals sc = {400, 300, 3900, 3700, TOUCHROT, SCREENROT}; // useful starting values, overwritten by screen cal and EEPROM restore.

// touch units to screen pixel conversions
int16_t pxh (int16_t px){ 
  int x = ((px-sc.thmin)*HMAX)/(sc.thmax-sc.thmin);
  if (x < 0) return 0;
  if (x > HMAX) return HMAX;
  return x;
}
int16_t pxv (int16_t py){ 
    int y = ((py-sc.thmin)*VMAX)/(sc.tvmax-sc.tvmin);
  if (y < 0) return 0;
  if (y > VMAX) return VMAX;
  return y;  
}

XPT2046_Touchscreen ts(TFT_TOUCH_CS);
TS_Point p;
//XPT2046_Touchscreen ts(TFT_TOUCH_CS, TFT_TOUCH_IRQ);
//XPT2046_Touchscreen ts(CS_PIN);  // Param 2 - NULL - No interrupts
//XPT2046_Touchscreen ts(CS_PIN, 255);  // Param 2 - 255 - No interrupts
//XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);  // Param 2 - Touch IRQ Pin - interrupt enabled polling

//Adafruit_ILI9341 tft = Adafruit_ILI9341(LTFT_CS, LTFT_DC, LTFT_RST);
//Adafruit_ILI9341 tft = Adafruit_ILI9341(LTFT_CS, LTFT_DC, LTFT_MOSI, LTFT_CLK, LTFT_RST, LTFT_MISO);

#define BVSPACE 45
#define BVSPACE2 30

#define LEGX 22
#define LSETX 35
#define LSETX2 52
#define RSETX 160
#define LEG2X 5
#define DWIDTH2 12
#define CIRCX 10
#define VALX 100
#define VAL2X 60
#define CALX 80
#define CAL2X 160
#define CALREADX 240
#define READX 185
#define TOPLINE 50
#define TOPLEG 17 // big enough for a FONT2 character
#define TOPLEGS 15 // big enough for a FONT1 character

#define IPSPACE 55
#define IPDOT 40
#define ROUNDERR 0.01    // float rounding may stop hi/low set values being attained.
#define UNITLEN 6
// turn these into a struct, include format string?
struct  valFmt {
 volatile void * vp;    // pointer to value - type is determined by fmt, assume all readings will be volatile
 uint8_t font;  // font (??-1 for GLCD font (wider character set)
 uint8_t textSize;     // textSize
 float minVal, maxVal; // limits for settings; hi-low error points for readings (float is OK for most things)
 short xpos, ypos;     // screen location
 char  fmt;             // format and value type - see printfloatX() - 'T' gets special treatment when touched
 char  units[UNITLEN]; // units to print on screen
 uint16_t clr;        // colour of text
 uint8_t menu;         // which menu are these in?
 uint8_t pre, post;    // decimals before and after the point (post is ignored for fmt != 'F')
 bool *displayIf;		 // only display if TRUE
 };
 
// SETTINGS VARIABLES 
// current set/read vals. SET values will be overwritten on start-up from saved EEPROM values
// only float and int8_t allowed
#define vSetting 	pSet.voltage
#define iSetting 	pSet.current
#define rSetting 	pSet.resistance
#define pSetting 	pSet.power
#define ltSetting	pSet.logTime
/*
#define iSLimit 	pSetX.limitOn
#define iSTrack 	pSetX.trackOn
#define iRLimit 	pSetX.eTrack
#define gSvolts 	pSetX.trackSv
#define gScurr 		pSetX.trackSa
#define gTrack 		pSetX.trackGrp
#define vTrack 		pSetX.trackSv
#define cTrack 		pSetX.trackSa
*/
#define iHost 		myID.instName
#define tsRota		sc.tsRot
float calOffLo_T, calOffHi_T;
// READINGS VARIABLES 
#define vReading 	meas.ADSvolts
#define iReading 	meas.ADSamps
#define tReading 	meas.ESPtemp
float pReading = 0.0;
float rReading = 0.0;

#define wAutoConn 	myID.autoConnect
#define IPreading 	IPstring
#define HostName 	myHostName // FQDN (mDNS)
#define dummyRead	xDumVal
#define wReading pContA.watts
int dummySteps;
#define stepSMPS dummySteps
//float ADCvRead = 3.2, ADCvReadS = 3.5; //vSetting = 5.0, vReading = 15.8,  vReadingS = 23.4,
//float  ADCiRead = 2.1;// iSetting = 2.0,iReading = 2.4,
 // uint8_t iRLimit = 100;iSLimit = 1,
//uint8_t vSTrack = 12;//, gTrack = 2;
char dummyS[64] = "ABC";
// hi [real, read]; lo [] ...
/*
float dummyCal[12] = {24.1, 2.3, 1, .04, 
                      2.0, 1.4, 0.1, 0.05,
                      27.2, 2.35, 6, 0.8};
*/
#define NUMSETTINGS 25
#define LSET 2 // index for LIMIT
#define TSET 3 // TRACK

valFmt setVals[NUMSETTINGS] = {
#define SV_V 0
   {&vSetting,  2, 1, VDROPOUT,VMAXOP, sx(READX),sy(TOPLINE+0*BVSPACE), 'F', "",VOLT_COL, 0,3,2, &notSTmode},// volts set
#define SV_I 1
   {&iSetting,  2, 1, IDROPOUT,IMAXOP, sx(READX+12),sy(TOPLINE+1*BVSPACE), 'F', "", AMP_COL, 0,2,2, &notSTmode},     // amps limit set - changed control_setup() if daughter board present
#define SV_P 2
   {&pSetting,  2, 1, PDROPOUT,PMAXOP, sx(READX),sy(TOPLINE+2*BVSPACE), 'F', "", WATT_COL,0,3,2, &notSTmode},    // power
   {&rSetting,  2, 1, RDROPOUT,RMAXOP, sx(READX),sy(TOPLINE+3*BVSPACE), 'F', "", RES_COL, 0,3,2, &notSTmode},    // resistance
   {&ltSetting, 2, 1, 0,MAXLOGTIME, sx(READX),sy(TOPLINE+4*BVSPACE), 'L', "", RES_COL, 0,6,0, NULL},    // log time, always show
#define SET_COM 5
   //COMMS MENU - 1
   {&myID.local_ssid,   1, 1,0,255, sx(LEG2X),sy(TOPLINE+0*BVSPACE), 'A', "", CYAN_L,1,3,0, NULL},
   {&myID.local_pass,   1, 1,0,255, sx(LEG2X),sy(TOPLINE+0.75*BVSPACE+10), 'A', "", CYAN_L,1,3,0, NULL},   // long (32 chars) - offset Y, so under button
   {&wAutoConn, 		1, 1,0,1,   sx(READX+40),sy(TOPLINE+1.5*BVSPACE), 'T', "", CYAN_L,1,2,0, NULL},  // auto connect
   {&iHost, 		    1, 1,0,1,   sx(LEG2X),sy(TOPLINE+2.25*BVSPACE), 'A', "", CYAN_L,1,2,0, NULL},     // hostname
#define SET_STEP (SET_COM + 4)  
   // STEP MENU - 2
   {&pSet.s[0].volts,  2, 1, VDROPOUT, VMAXOP,  sx(LSETX), sy(TOPLINE+0*BVSPACE), 'F', "V", CYAN_L,2,3,2, NULL},    // V1, A1, T1
#define  SV_S1A (SET_STEP + 1)
   {&pSet.s[0].amps,   2, 1, IDROPOUT, IMAXOP,  sx(LSETX+DWIDTH2), sy(TOPLINE+1*BVSPACE), 'F', "A", CYAN_L,2,2,2, NULL},     // see control_setup()
   {&pSet.s[0].time,   2, 1, MINSTEP, 65535,  	 sx(LSETX), sy(TOPLINE+2*BVSPACE), 'U', "mS", CYAN_L,2,5,0, NULL},  
   {&pSet.s[0].steps,     2, 1, 0, 65535,  	 sx(LSETX), sy(TOPLINE+3*BVSPACE), 'U', "", CYAN_L,2,5,0, NULL},  // Steps
   {&pSet.s[1].volts,  2, 1, VDROPOUT, VMAXOP,  sx(RSETX), sy(TOPLINE+0*BVSPACE), 'F', "V", CYAN_L,2,3,2, NULL},  //V2, A2, T2
 #define  SV_S2A (SET_STEP + 5)  
   {&pSet.s[1].amps,   2, 1, IDROPOUT,IMAXOP,   sx(RSETX+DWIDTH2), sy(TOPLINE+1*BVSPACE), 'F', "A", CYAN_L,2,2,2, NULL},  // see control_setup()
   {&pSet.s[1].time,   2, 1, MINSTEP, 65535,  	 sx(RSETX), sy(TOPLINE+2*BVSPACE), 'U', "mS", CYAN_L,2,5,0, NULL},  
   {&pSet.s[1].steps,  2, 1, 0, 65535,  		 sx(RSETX), sy(TOPLINE+3*BVSPACE), 'U', "", CYAN_L,2,5,0, NULL},  // Steps
   {&pSet.slew,        2, 1, 0, 65535,    	     sx(LSETX), sy(TOPLINE+4*BVSPACE), 'U', "mS", CYAN_L,2,5,0, NULL},  // rise/fall mS
   {&pSet.cycles,      2, 1, 0, 65535,      	 sx(RSETX), sy(TOPLINE+4*BVSPACE), 'U', "", CYAN_L,2,5,0, NULL},  // cycles
     // {&pSet.triangle,  2, 1, 0, 1,      		   sx(RSETX+40), sy(TOPLINE+4*BVSPACE), 'T', "", CYAN_L,2,1,0, NULL},  // triangle
#define SET_CAL (SET_STEP + 10)
   //CAL MENU - 3
   {&calOff_V, 2, 1,-5.0, 5.0, sx(READX),sy(TOPLINE+0*BVSPACE +5), 'F', "", VOLT_COL,3,1,3, NULL},     // Calibration (variable => 0.0 on entering Cal menu)
   {&calOff_I, 2, 1,-2.0, 2.0, sx(READX),sy(TOPLINE+1*BVSPACE-5), 'F', "", AMP_COL,3,1,3, NULL},   
   {&calOffLo_T, 2, 1,-30, 30, sx(READX),sy(TOPLINE+2*BVSPACE-5), 'F', "", TEMP_COL,3,2,1, NULL},   
   {&calOffHi_T, 2, 1,-30, 30, sx(READX),sy(TOPLINE+3*BVSPACE-5), 'F', "", TEMP_COL,3,2,1, NULL},   

 #define SET_ID (SET_CAL + 4)
   // ID MENU  - NOT USED
   {&dummyS,  1, 1,0,255, sx(100),sy(TOPLINE), 'A', "kN",CYAN_L,9,12,0, NULL}, // test string edit
   {NULL, 0, 1,0,0, 0,0, 'X', "",CYAN_L,0,0,0, NULL} // EOM: .vp == NULL
 };

 
#define NUMREADS 10
valFmt readVals[NUMREADS]= { // max and min are hi/lo limits - set the maxima/ minima well out of bounds
   {&vReading, 3, 1,-1,200.0,sx(LSETX2), sy(TOPLINE+0*BVSPACE), 'F', "V", VOLT_COL,0,3,2, NULL},   // output volts - (void*) cast to "undo" volatile
#define RV_I 1
   {&iReading, 3, 1,0,50.0, sx(LSETX2), sy(TOPLINE+1*BVSPACE), 'F', "A", AMP_COL,0,3,2, NULL},  // output amps
   {&pReading, 3, 1,0,400.0, sx(LSETX2), sy(TOPLINE+2*BVSPACE), 'F', "W", WATT_COL,0,3,2, NULL},  // output watts
   {&rReading, 3, 1,0,999.9, sx(LSETX2), sy(TOPLINE+3*BVSPACE), 'F', "R", RES_COL,0,3,2, NULL},  // output watts
   {&tReading, 0, 1, 0,100.0,  sx(110), sy(TOPLEGS), 'F', "C", CYAN_M,0,3,0, NULL},           // temperature
   
   // COMMS Menu - 1
   {&IPreading, 1, 1, 0, 0, sx(VALX-20), sy(TOPLINE+3.5*BVSPACE), 'A', "C", CYAN_M,1,3,0, NULL},           // IP (string format)
   {&HostName,  1, 1, 0, 0,  sx(VALX-20), sy(TOPLINE+4*BVSPACE), 'A', "C", CYAN_M,1,3,0, NULL},         // fully qualified hostname (mDNS)
   // STEP MENU - 2 (NONE)
	
   // CAL MENU - 3 these three readings are "ADC read volts"
   {&vReading, 2, 1,-1,160.0,sx(LEG2X+20), sy(TOPLINE+0*BVSPACE+5), 'F', "", VOLT_COL, 3,2,3, NULL},  // output volts
   {&iReading, 2, 1,-1,35.0, sx(LEG2X+20), sy(TOPLINE+1*BVSPACE-5), 'F', "", AMP_COL,3,2,3, NULL},	 // output amps
   // rotations are displayed as indicators see drawIndicators()
   //{&tsRota,   2, 1,-99,99,   HMAX/2 - 18, VMAX/2 + sy(50), 'I', "", LIGHTYELLOW,3,1,0, NULL},  // screen rotation
   {&tReading, 2, 1, 0,85.0, sx(LEG2X+20), sy(TOPLINE+2*BVSPACE), 'F', "C", TEMP_COL,3,3,1, NULL}           // temperature
  // {NULL, 0, 1,0,0, 0,0, 'X', "",0,0,0, NULL} // EOM: .vp == NULL
 };   


 // TOUCH SCREEN BUTTONS
//  sizing
#ifdef ILI9488
  #define BUTSIZ 50 
#else
  #define BUTSIZ 40    
#endif
#define BUTEDGETHICK 2
#define BUTROUND 6
#define TEXTDOWN 4
// location
#define BUTX 280  // 275
#define BUTBOTV (240 - BUTSIZ)
#define BUTBOTV2 (240 - BUTSIZ/2)

#define BVSPACEX BVSPACE
#define TOPLINEX (TOPLINE - 5)
#define BUTMIDX ((320 - BUTSIZ)/2)    // this is scaled
#define BTOP 20
#define SBTOP (BTOP + BUTSIZ/2) // small buttons - align baseline
// LEGENDS
#define LEGMAX 19
struct legend {
   uint8_t font;  // font
   uint8_t textSize;     // textSize
   short xpos, ypos;     // screen location
   int8_t menu;          // which menu are these in?
   char text[LEGMAX];    // text to write
   uint16_t clr;         // colour of text
   bool *displayIf;		 // only display if TRUE
};
#define NUMLEGS 11	// also some static screen items in redrawScreen()
legend leg[NUMLEGS] = {

   // COMMS MENU - 1
   {1,  1,sx(2), sy(TOPLEGS), 1, "COMMS", HIGHLIGHTCOL, NULL},
   {1,  1,sx(2), sy(TOPLINE+3.5*BVSPACE), 1, "IP:", CYAN_M, NULL},
   {1,  1,sx(2), sy(TOPLINE+4*BVSPACE), 1, "HOST:", CYAN_M, NULL},
   // TRACK MENU - 2
   {1,  1,sx(2), sy(TOPLEGS), 2, "STEP MODE", HIGHLIGHTCOL, NULL},
   // CAL MENU -3
   {1,  1,sx(2), sy(TOPLEGS), 3, "CALIBRATION", HIGHLIGHTCOL, NULL},
   {1,  1,sx(READX), sy(TOPLEGS+20), 3, "OFFSET", HIGHLIGHTCOL, NULL},
   {1,  1,sx(LEG2X+34), sy(TOPLEGS+20), 3, "READING", HIGHLIGHTCOL, NULL},
   //settings menu 
   {1,  1,sx(2), sy(TOPLEGS), 4, "SETTINGS", HIGHLIGHTCOL, NULL},
   {1,  1,sx(BUTX-80), sy(TOPLEGS+8), 4, "VOLT", HIGHLIGHTCOL, NULL},
   {1,  1,sx(BUTX-80), sy(TOPLEGS+24), 4, "MEAS", HIGHLIGHTCOL, NULL},
   {0,  0,0, 0, -1, "0", 0, NULL} // EOM: .menu == -1
};


// callback routines  should contain non-blocking code (e.g. state machines)
// if blocking code is required, then yield() should be issued regularly to allow background (e.g. WiFi) tasks to process

//  callback routine COMMAND codes
#define CALL_CMD_START  1 // start cmd (first call)
#define CALL_CMD_PROC   0 // continue processing
#define CALL_CMD_STOP  -1 // cease processing

// callback routine RETURN codes - see ScreenCal and OSK for examples
#define CALL_PROC -1  	// Still processing
#define CALL_ERR  -10 	// error exit return any value <= -10
// Ok exit may be any value >= CALL_EX
#define CALL_EX    0
#define CALL_NOEX  1

// callback processing STATUS codes
#define CALL_ACTIVE 1 	// still processing (call me again)
#define CALL_DONE   2	// completed
#define CALL_START  3	// intialising - not yet called
#define CALL_IDLE   0 	// no call in progress

uint8_t callStatus = CALL_IDLE;
boolean wastouched = false;
// also used by switches
bool swPressed = false; 
short butDigit = 0; // start at 10^0
short butDir = 0;

uint8_t currentMenu = 0; // which menu are we in?

// callbacks here
#include "myLScreenCal.h" 
#include "myLOSK.h"
// also myTouchProcess.h

#define BUTTEXT 6 // 5 chars at most
struct button {
  int (* callback)(int cmd); // see above
  int8_t  menu;
  uint8_t nextMenu;   // same as menu, unless changing screens 
  uint8_t sv;     // index to the SetVal item to be edited
  uint8_t siz;     // val 0..7 - bit 0 == 0 :half height, bit 1 == 0 : half width; bit 2 = display button
  short   xpos, ypos;
  bool    border;
  char text[BUTTEXT];
  uint16_t selColour;	// some buttons are also indicators - so provide ability to change bg colours
  uint16_t unselColour;
  uint8_t *onIf;			// also display as "selected" if this is set
  bool *displayIf;		// only display button if TRUE
};

#define CHOFFY 15 // offsets for invisible buttons
#define CHOFFX (10)
// some setVal indices - used in tMode
#define LBUT 2
#define TBUT 3
#define WBUT 6
#define GSVBUT SET_STEP 
#define GSABUT (SET_STEP +1)
#define NOSETVAL 255
//int toggleLimit(int);
//int toggleTrack(int);
int setCCmode(int);
int setCVmode(int);
int setCPmode(int);
int setCRmode(int);
int startBAT(int);
int startST(int);
int setNORb(int);
int setZeroCal(int);
int screenCal(int);
int setDefs(int);
int exitCal(int);
int tsRotate(int);
int scrRotate(int);
int toggleWconn(int);
int vAuto(int);
int vMain(int);
int vKel(int);

#define NUMBUTS 50
button but[NUMBUTS] = {
  // MAIN MENU - 0
  {NULL,0, 0, 0, 7, sx(BUTX), sy(BTOP+0*BVSPACEX), false,"V", BBGHIGH, BBXHIGH, NULL, &notSTmode}, // invisible in ST mode
#define AMPS_BUTTON 1
  {NULL,0, 0, 1, 7, sx(BUTX), sy(BTOP+1*BVSPACEX), false,"A", BBGHIGH, BBXHIGH, NULL, &notSTmode},
  {NULL,0, 0, 2, 7, sx(BUTX), sy(BTOP+2*BVSPACEX), false,"P", BBGHIGH, BBXHIGH, NULL, &notSTmode}, 
  {NULL,0, 0, 3, 7, sx(BUTX), sy(BTOP+3*BVSPACEX), false,"R", BBGHIGH, BBXHIGH, NULL, &notSTmode}, 
  {NULL,0, 0, 4, 7, sx(BUTX), sy(BTOP+4*BVSPACEX), false,"LI", BBGHIGH, BBXHIGH, NULL, NULL}, // always visible
  {NULL,0, 4, NOSETVAL, 6, sx(READX), sy(0), false,"SET", BBSHIGH, BBSHIGH, NULL}, // SET menu  
  // these should all be XOR toggles, button lights if.
  {&setCVmode, 0, 0, NOSETVAL, 7, sx(0), sy(BTOP+0*BVSPACEX), false,"CV", BBMHIGH, BBMXHIGH, &inMode[MODE_CV], NULL},// MODE SETTING
  {&setCCmode, 0, 0, NOSETVAL, 7, sx(0), sy(BTOP+1*BVSPACEX), false,"CC", BBMHIGH, BBMXHIGH, &inMode[MODE_CC], NULL},
  {&setCPmode, 0, 0, NOSETVAL, 7, sx(0), sy(BTOP+2*BVSPACEX), false,"CP", BBMHIGH, BBMXHIGH, &inMode[MODE_CP], NULL}, 
  {&setCRmode, 0, 0, NOSETVAL, 7, sx(0), sy(BTOP+3*BVSPACEX), false,"CR", BBMHIGH, BBMXHIGH, &inMode[MODE_CR], NULL},  

  {&startBAT, 0, 0, NOSETVAL, 7, sx(30),VMAX - BUTSIZ, false,"BT", BBMHIGH, BBMXHIGH, &isModeB[MODE_BAT], NULL}, // Battery test, red when active
  {&startST,  0, 0, NOSETVAL, 7, sx(85),VMAX - BUTSIZ, false,"ST", BBMHIGH, BBMXHIGH, &isModeB[MODE_ST], NULL}, // same as BAT
  {&setNORb,  0, 0, NOSETVAL, 7, sx(140), VMAX - BUTSIZ, false,"NOR", BBMHIGH, BBMXHIGH, &isModeB[MODE_NOR], NULL},	// bright Green when active

// Settings menu
  {NULL,      4, 2, NOSETVAL, 7, (HMAX)/3, sy(BTOP+0.5*BVSPACEX), false,"STEP", BBSHIGH, BBSHIGH, NULL, NULL}, // STEP
  {&calEntry, 4, 3, NOSETVAL, 7, (HMAX)/3, sy(BTOP+2*BVSPACEX), false,"CAL", BBSHIGH, BBSHIGH, NULL, NULL}, //CAL
  {NULL,      4, 1, NOSETVAL, 7, (HMAX)/3, sy(BTOP+3.5*BVSPACEX), false,"COM", BBSHIGH, BBSHIGH, NULL, NULL}, // COM
  {&vAuto,    4, 4, NOSETVAL, 7,  sx(BUTX-80), sy(BTOP+0.5*BVSPACEX), false,"AUTO", BBGHIGH, BBXHIGH, NULL, NULL}, //v Measurement
  {&vMain,    4, 4, NOSETVAL, 7,  sx(BUTX-80), sy(BTOP+2*BVSPACEX), false,"MAIN", BBGHIGH, BBXHIGH, NULL, NULL}, 
  {&vKel,     4, 4, NOSETVAL, 7,  sx(BUTX-80), sy(BTOP+3.5*BVSPACEX), false,"KEL", BBGHIGH, BBXHIGH, NULL, NULL}, 

  // COM MENU - 1
  /*
  {NULL,1, 1, SET_COM, 6, VAL2X+0*IPSPACE,TOPLINE+10, false,"IP1", BBGHIGH, BBXHIGH  },
  {NULL,1, 1, SET_COM + 1, 6, VAL2X+1*IPSPACE,TOPLINE+10, false,"IP2", BBGHIGH, BBXHIGH  }, 
  {NULL,1, 1, SET_COM + 2, 6, VAL2X+2*IPSPACE,TOPLINE+10, false,"IP3", BBGHIGH, BBXHIGH  }, 
  {NULL,1, 1, SET_COM + 3, 6, VAL2X+3*IPSPACE,TOPLINE+10, false,"IP4", BBGHIGH, BBXHIGH  }, 
  */
  {NULL,1, 1, SET_COM, 	   			6, sx(BUTX), SBTOP + sy(0*BVSPACEX), false,"SS", BBGHIGH, BBXHIGH, NULL, NULL}, // SSID
  {NULL,1, 1, SET_COM + 1, 			6, sx(BUTX), SBTOP + sy(0.75*BVSPACEX), false,"PA", BBGHIGH, BBXHIGH, NULL, NULL}, //WiFi Pass
  {&toggleWconn,1, 1, SET_COM + 2,  6, sx(BUTX), SBTOP + sy(1.5*BVSPACEX), false,"AC", BBGHIGH, BBXHIGH, NULL, NULL}, // Toggle WiFi AutoConnect
  {NULL,1, 1, SET_COM + 3, 			6, sx(BUTX), SBTOP + sy(2.25*BVSPACEX), false,"HO", BBGHIGH, BBXHIGH, NULL, NULL}, // Hostname
  
  // STEP MENU - 2
  {NULL,2, 2, SET_STEP 	  , 7, sx(0), sy(BTOP+0*BVSPACE), false,"V1", BBGHIGH, BBXHIGH, NULL, NULL},  
  {NULL,2, 2, SET_STEP + 1, 7, sx(0), sy(BTOP+1*BVSPACE), false,"A1", BBGHIGH, BBXHIGH, NULL, NULL},  
  {NULL,2, 2, SET_STEP + 2, 7, sx(0), sy(BTOP+2*BVSPACE), false,"T1", BBGHIGH, BBXHIGH, NULL, NULL},  
  {NULL,2, 2, SET_STEP + 3, 7, sx(0), sy(BTOP+3*BVSPACE), false,"S1", BBGHIGH, BBXHIGH, NULL, NULL}, 
  {NULL,2, 2, SET_STEP + 4, 7, sx(BUTX), sy(BTOP+0*BVSPACE), false,"V2", BBGHIGH, BBXHIGH, NULL, NULL},  
  {NULL,2, 2, SET_STEP + 5, 7, sx(BUTX), sy(BTOP+1*BVSPACE), false,"A2", BBGHIGH, BBXHIGH, NULL, NULL},  
  {NULL,2, 2, SET_STEP + 6, 7, sx(BUTX), sy(BTOP+2*BVSPACE), false,"T2", BBGHIGH, BBXHIGH, NULL, NULL}, 
  {NULL,2, 2, SET_STEP + 7, 7, sx(BUTX), sy(BTOP+3*BVSPACE), false,"S2", BBGHIGH, BBXHIGH, NULL, NULL}, 
  {NULL,2, 2, SET_STEP + 8, 7, sx(0), sy(BTOP+4*BVSPACE), false,"R/F", BBGHIGH, BBXHIGH, NULL, NULL},  
  {NULL,2, 2, SET_STEP + 9, 7, sx(BUTX), sy(BTOP+4*BVSPACE), false,"CYC", BBGHIGH, BBXHIGH, NULL, NULL}, 
 // {&toggleTriangle,2, 2, SET_STEP + 8, 7, sx(BUTX), sy(BTOP+4*BVSPACE), false,"TRI", BBGHIGH, BBXHIGH, NULL, NULL},   // toggle triangle
 
  // CAL MENU - 3 Volts/amps
  {NULL,3, 3, SET_CAL, 7, 		sx(BUTX), sy(BTOP+0*BVSPACEX), 	false,	"V" , BBGHIGH, BBXHIGH, NULL, NULL}, 
  {NULL,3, 3, SET_CAL+1, 7, 	sx(BUTX), sy(BTOP+1*BVSPACEX), 	false,	"A" , BBMHIGH, BBMXHIGH, NULL, NULL}, 
  {NULL,3, 3, SET_CAL+2, 7, 	sx(BUTX), sy(BTOP+2*BVSPACEX), 	false,	"T-L" , BBRHIGH, BBRXHIGH, NULL, NULL}, 
  {NULL,3, 3, SET_CAL+3, 7, 	sx(BUTX), sy(BTOP+3*BVSPACEX), 	false,	"T-H" , BBRHIGH, BBRXHIGH, NULL, NULL}, 
  {&setZeroCal,3, 3, NOSETVAL, 7,(HMAX - BUTSIZ)/2-12, sy(BTOP)-10,   false,  "ZERO" , BBGHIGH, BBXHIGH, NULL, NULL}, 
  {&setDefs,3, 3, NOSETVAL, 7,(HMAX - BUTSIZ)/2-12, sy(BTOP)+BUTSIZ , 	false,	"DEF" , BBGHIGH, BBXHIGH, NULL, NULL}, 
  {&calSave,3, 0, NOSETVAL, 7, 	 (HMAX - BUTSIZ)/2-12, VMAX - BUTSIZ, false,"SAVE" , BBGHIGH, BBXHIGH, NULL, NULL}, // save and exit
  
  // CAL - SCREEN CAL  
  {&screenCal,3, 3, NOSETVAL, 7, sx(20), VMAX - BUTSIZ, 		false,	"TCH", BBSHIGH, BBSHIGH, NULL, NULL}, 
  {&tsRotate,3, 3, NOSETVAL, 7, (HMAX - BUTSIZ)/2-12, (VMAX - BUTSIZ)/2+10, false,"TROT" , BBGHIGH, BBXHIGH, NULL, NULL}, // in middle of screen - as rotation may be reversed
  {&scrRotate,3, 3, NOSETVAL, 7, (HMAX - BUTSIZ)/2-12, (VMAX - BUTSIZ)/2+70, false,"SROT" , BBGHIGH, BBXHIGH, NULL, NULL}, // below TROT

  // exit buttons (all menus except MAIN)
  {NULL,1, 0, NOSETVAL, 7, sx(BUTX), VMAX - BUTSIZ, false,"X", BBGHIGH, BBXHIGH, NULL, NULL}, // COMMS exit
  {NULL,2, 0, NOSETVAL, 7, sx(BUTX/2 - 14), VMAX - BUTSIZ, false,"X", BBGHIGH, BBXHIGH, NULL, NULL}, // STep  exit
  {NULL,4, 0, NOSETVAL, 7, sx(BUTX), VMAX - BUTSIZ, false,"X", BBGHIGH, BBXHIGH, NULL, NULL}, // SET exit
  {&exitCal,3, 0, NOSETVAL, 7,  sx(BUTX), VMAX - BUTSIZ, false,"X", BBGHIGH, BBXHIGH, NULL, NULL},  // CAL exit
  {NULL,-1, 0, NOSETVAL, 0, 0, 0, false,"", BBGHIGH, BBXHIGH, NULL, NULL} // EOM
};

/*
Colour tables for Toggle buttons and Indicators
Modes 
	Off [0] 
	On [1]: Inactive, e.g. not limiting or tracking = 100%
	On [2]: Active, e.g. limiting or tracking < 100%
for indicators, Off is BGCOLOR, rather than tColours[0]
*/
#define BBOFF 0 // track and limit (disabled)
#define BBINACT 1 // function enabled, but not active
#define BBACT 2 // function enabled, and active
#define WIFIACT 1 // green (autoconnect + connected)
#define WIFIOFF 0 // blue  (autoconnect off = idle)
#define WIFIINACT 2 // red (autoconnect + not connected = error)
uint16_t tColours[3] = {BBXHIGH, MY_DARKGREEN, MY_RED};

#endif
