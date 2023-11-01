/* myInst.h
Basic definitions for all instruments and the Control Module
*/
#ifndef MYLINST_H
#define MYLINST_H

//#define ESP8266
//#define NANO
#define ESP_32

#ifdef ILI9488
// need to include this before the pin (re)definitions
#include <Arduino_GFX_Library.h>
#endif

#include <EEPROM.h>
#include <WiFi.h>

#define EEAUTOSAVE 1	// 1 for save to EEPROM (production), 0 (!= 1) for no save
#define SAVE_EE_AFTER 3	// VL cycles - 3 for production

struct NullStream : public Stream{ // bit bucket for production debug info
 NullStream( void ) { return; }
 int available( void ) { return 0; }
 void flush( void ) { return; }
 int peek( void ) { return -1; }
 int read( void ){ return -1; };
 size_t write( uint8_t u_Data ){ return u_Data, 0x01; }
};
NullStream noDebug;

// switch Serial debugging on and off
#define SDEBUG1 Serial	// testing - information
//#define SDEBUG1 noDebug // production - information

#define SDEBUG2 Serial	// testing - errors
//#define SDEBUG2 noDebug // production - errors

#define IDLEN 16
char dns_id[IDLEN] = "ESP32_5";
// BOARD REV C/D/E - Control v13 onwards
#define ENC_A 		25  // Scrubber control encoder // remapped from 32
#define ENC_B 		33  // Note: Encoder switch is 
#define CNTRL_R 	14	// Scrubber buttons 
#define CNTRL_L 	27

//  ESP ADC (secondary for V & I, primary for T)
#define ADC_BV   	34 //  ESP ADC1
#define ADC_BI   	35 //  ESP ADC1
#define ADC_BT   	32 //  ESP ADC1 
#define PWM_PIN  	26 
#define FANPIN		PWM_PIN  // GPIO 16/17 are NC on WROVER (return to pin 26) - REv C boards (need to patch FAN from CONTROL Pin 10 to pin 14.
//#define RXD2 		  16 
#define DPRESENT	17
//#define TXD2 		  17 // don't use for Load.
#define ADSRDY 		2
//#define DACPIN 		255 // was 25

#define SW_ON 		4 	// hi is active 
#define SW_OFF 		12  // hi is active 
#define TFT_TOUCH_CS 15
#define TFT_TOUCH_IRQ 255 // not used - jumper is open

#define L_BUT  CNTRL_L
#define R_BUT  CNTRL_R

// Generics
#define SENSE_A 	34 // GPIO or ADC
#define SENSE_B 	35
#define SENSE_C 	32
#define SENSE_D 	33

#define TFT_SS 		5
#define SPI_SCK 	18
#define MISO 		  19
#define MOSI 		  23
#define T_IRQ 		2
#define T_CS 		  15

#define LTFT_DC 	13
#define LTFT_CS   TFT_SS
#define LTFT_MOSI	MOSI
#define LTFT_CLK 	SPI_SCK
#define LTFT_MISO MISO
#define LTFT_RST 	255 //-1
//#define TFT_IRQ 	255  // jumper is open

#define SDA 		  21
#define SCL 		  22

//#define OPTO_IN 	3 	// RXD
//#define OPTO_OUT 	1 	// TXD

// ***** rotary encoder and L-R switches
//#define ENCODER_DO_NOT_USE_INTERRUPTS 
#define DEBOUNCE 10
#define ENC_CLIX 2
#define ALTENCODER //- use interrupt driven ESP32Encoder library
#ifdef ALTENCODER
	#include <ESP32Encoder.h>
	ESP32Encoder enc;
#else
	#include "ESPRotary.h"
	ESPRotary enc = ESPRotary(ENC_B, ENC_A, ENC_CLIX);
#endif
#include "Button2.h"

//Button2 but  = Button2(ENC_SW); // not used
Button2 butL = Button2(L_BUT, INPUT_PULLDOWN, false);
Button2 butR = Button2(R_BUT, INPUT_PULLDOWN, false);

#define ADSBITS 15

/*
	contains all stored identification data - name, network, etc
*/
#define SSIDLEN 34		// > 32 + null
#define NETPASSLEN 64	// > 63 + null
#define NAMELEN 16		// displayed name, used to identify
uint8_t netCIDR = 24;	// default Class C network - to calculate netmask 

struct netParams { // general network spec
  IPAddress local, gateway, subnet;
  bool autoconnect; // maybe not relevant, other than to enable AP mode?
};

struct wifiParams { // Wifi credentials
  char ssid[SSIDLEN],
       pass[NETPASSLEN];
  bool autoconnect;  
};

// definitions for our own local network as base station 
#define ESPNETNAME "ESPINST"
#define ESPNETPW 	"ESPPW99X"

#ifdef REALNET
  #include "wifiSecrets.h"
#else
  #define LOCAL_SS "MySSID"
  #define LOCAL_PASS "MyPass"
#endif
struct instID {
	IPAddress prefIP;	// preferred IP
	IPAddress gateIP;	
	IPAddress netmask;	
    char local_ssid[SSIDLEN];
	char local_pass[NETPASSLEN];	// always assumes DHCP on WiFi
	char esp_ssid[SSIDLEN];
	char esp_pass[NETPASSLEN];	// always assumes DHCP on WiFi
	bool autoConnect;			// maybe "enabled"?
	char instName[NAMELEN];
	bool daughter_present;		// changed by control_setup()
} myID = {{192,168,50,1}, {192,168,50,1},{192,168,50,255}, LOCAL_SS, LOCAL_PASS, ESPNETNAME, ESPNETPW, true, "MyLoad", false};

bool IamAP = false;	// am I the AP?

// screen pixel limits
// screen pixel limits
#ifdef ILI9341
	#define HMAX 320
	#define VMAX 240
	#define SCREENROT 3 // SCREEN rotation default
	#define TOUCHROT 1
#else // ILI9488
	#define HMAX 480	// not used everywhere. when using sx() 320 is max
	#define VMAX 320    // not used everywhere. when using sy() 240 is max
	#define SCREENROT 3 // SCREEN rotation default
	#define TOUCHROT 1
#endif

// various global defs
#define UDP_PORT 8888 
#define TELNET_PORT 5025 // SCPI std
#define MAXTRACKGRP 127
#define HOSTLEN 48

// updated on connection
IPAddress myIP 			= {192,168,1,200};
IPAddress myBroadcastIP = {192,168,1,255};
IPAddress mySubnetMask  = {255,255,255,0};

IPAddress myAP_IP       = {192,168,50,1};
IPAddress myAP_GATEWAY  = {192,168,50,0};
IPAddress myAP_BroadcastIP = {192,168,50,255};
IPAddress myAP_SubnetMask  = {255,255,255,0};

char myHostName[HOSTLEN];
char IPstring[16];
uint8_t myNetCDIR = 24;

#define WIFISTORED 6		// stored WiFi credentials
#define WIFIMAXCONN	127
struct WiFiStore
{
  char  ssid[SSIDLEN];
  char  pass[NETPASSLEN]; 
  int8_t lastConn;
} myWiFi[WIFISTORED];

bool dirtyScreen = true; // any routine can cause the screen to be redrawn
//bool valChanged(VAL_CHGD);
volatile bool changedVal_scrn = false, changedVal_EE = false, changedVal_remote = false; // changed settings values need to be addressed by several modules
bool needToSaveEE = false;

// on screen keyboard active - disable various functions
bool oskOn = false;

#define SCPI_BUFLEN 256
#define SCPI_SHORTCMD 4
// SCPI stream timeouts (Seconds) -1 == never
#define SCPIUDPTIMEOUT 		10	// (S)
#define SCPISERIALTIMEOUT 	-1	// (S)
#define SCPITELNETTIMEOUT 	-1	// (S) 
#define HEART_MSG "HEARTBEAT"

// test values - not for production
float xDumVal = 0.0;
#endif /* MYINST_H */
