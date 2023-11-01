#ifndef  MYPSU_H
#define  MYPSU_H
/* Configuration information for the PSU (slave) control hardware
*/

#define MODEL "DCL01"
#define HARD_VERSION 1 // not used other than profile.
#define EE_MODEL 12
// SCPI identification 
#define IDN_MANUF "Platypus"
#define IDN_MODEL MODEL
//#define IDN_SERIAL "2001" - use hostname variable
#define IDN_FIRMWARE SOFT_VERSION
#define IDN_OPTS "NONE"

#define IS_ESP32

// diagnostics
//#define CDEBUG2

#define CS_PIN        10

#ifdef IS_ESP32
  #define VPIN        35   // ADC1_6 Coarse control VO
  #define VPIN_S      34   // ADC1_7
  #define IPIN        32   // ADC1_4
  #define TPIN        33   // ADC1_5
  #define DACPIN      25   // FINE CONTROL VO
  #define TESTPIN     16   // Diagnostic only
  #define KELPIN      16   // voltage measurement mode
  #define EEPROM_SIZE 4096 // bytes available
 // #include "myLESP32ADC.h"
#endif

// main loop task timing
#define PROCESS_EVERY_VF    1     //  1 mS set dynamic values (inc slew).
#define PROCESS_EVERY_F     5     //   5 mS - switches (debounce)
#define PROCESS_EVERY_CTRL 10 //  10 mS - log and switch update
#define PROCESS_EVERY_M    100    //  100mS -   screen touch and update
#define PROCESS_EVERY_L    750   // 750 mS - display and readings; fan
#define PROCESS_EVERY_VL   10000  // 10  S - test for EE save
#define PROCESS_EVERY_T    3000         // for tesing/debug only
#define CYCLES_STEP        8
#define MAXLOG             512 // possibly 1000 for production
#define MAXBATLOG 512  // at 20s / log = 24000s = 6.6hrs of battery logging


#include "myLADS1115.h"
#include "Adafruit_MCP4725RP.h"
// mutex variable for interrupt exclusion, starts open
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define ADC_CHANNELS 4
#define ADSMUX_START 0  // channel for first reading

#define NUMCHANNELS 1 // number of slave instruments controlled
uint8_t currentChannel = 0; // index value of current channel (local control) - may be different from SCIPchan

struct chPins{
	uint8_t onPin;
	uint8_t offPin;
} chanPins[NUMCHANNELS] = {
	{SW_ON, SW_OFF}
};

/***************** I2C Hardware definitions ***********************/
#define I2C_SPEED	400000

#define DAC_BASE	 0x60 	// MCP 4725 - may be anywhere in 0x60 - 0x67
#define DAC_SCAN	 8
#define DAC_MASK 	 0x0fff  // 12 bits [0..4096]

#define ADS_ADDR 0x48
//#define ADS_RATE ADS1115_REG_CONFIG_DR_128SPS
//#define ADS_RATE ADS1115_REG_CONFIG_DR_250SPS // I2C commands take 500uS, so no point in very fast conversion
//#define ADS_RATE ADS1115_REG_CONFIG_DR_475SPS	// ~10mS for all 4 readings
#define ADS_RATE ADS1115_REG_CONFIG_DR_860SPS // too fast for all the I2C traffic
#define ADSWDT 20	// mS - set a lot longer than the standard conversion time and the control loop timer

#define ADS_GAIN GAIN_TWO // FSD: GAIN_ONE = 4.096V; GAIN_TWO = 2.048V 
#define ADS_MAXV 2.048
Adafruit_ADS1115 ads(ADS_ADDR);  
Adafruit_MCP4725 dac;
// ***** Primary ADC - ADS1115 *****
#define ADCsteps 		((1 << 15) -1)
#define ADCMAXVOLTS 	2.048
// **** Secondary ADC - ESP32 ****
#define ADCSTEPS_ESP 	((1 << 12) -1)
#define ADCMAXVOLTS_ESP 3.3 ESP

// Control modes
#define ADS_CONTROL 1		// ADS values used, coarse or fine control
#define ESP_CONTROL 2		//  ESP values used, with sense check at low values.
#define COARSE_ITER  10		// number of read/set iterations in FAST control mode
#define C_VARCOARSE	 (5)  // abs diff for COARSE / FAST CONTROL
#define C_VARCOARSE_RAT (0.6)  // ratio diff for COARSE / FAST CONTROL
#define C_VARFINE	 (0.005)  // abs diff for ADS_CONTROL - 
#define C_VARFINER	 (0.005)  // ratio diff for ADS_CONTROL - 

// normal control damping factors
// too small: won't get close to exact result (<1 DAC step) 
// too large: hunting
#define C_DAMPING_C     (0.7)  // CC and current limiting 
#define C_DAMPING_VL    (0.5)  // CV at low currents
#define C_DAMPING_VH    (0.9)  // at high cureents
#define C_DAMPING_VI    (0.1)  // changeover current
#define C_DAMPING_VD    (1.05) // at low currents, even with good V differential, can't move away from minimum
//#define C_DAMPING_VI_MIN (0.01)
#define C_DAMPING_P     (0.5)  // CP
#define C_DAMPING_RH    (0.5)  // CR  0.05 - slow seeking 
#define C_DAMPING_RL    (0.1)  // CR  0.05 - slower seeking at low values
#define C_DAMPING_RUP   (0.9)  // faster rise to reduce current quickly
#define DACOVER 1.3 // ampsToDAC isn't perfect (+/- 10%), so allow some headroom in DAC limiting 

// current limiting for CP, CR, CV modes
#define C_DAMPING_LIM 4    
#define OVER_GAP 1.2
#define JUMP_GAP (OVER_GAP - 0.1)
//#define C_STEADY 	 1   	// in steady state control
//#define C_TRANS  	 2 		// currently slewing in STep mode

#define MINSTEP   10		// mS
/*****************************************************************************************
 ********** NEED TO CHANGE SOFTWARE VERSION (UPDATE PROFILE) IF THESE CHANGE  ************
 ********** OR FACTORY RESET 
 *****************************************************************************************/
#define MAXUINTVAL	32767	// biggest setting for 16 bit integer values. Maybe could be 65535? (min usually == 0)

#define VMAXOP		 (150.0)	// max output voltage setting (float). SOA check.
#define OVERVOLTLIM		5	// this is the legitimate max above operating
#define VDROPOUT    (0.05)  // min voltage for proper operation 

// ****** set these for NO DAUGHTER BOARD ******
#define IDROPOUT	 (0.001)
#define IMAXOP		 (15)	// max set current - fixed in daughterSense(). 
#define IMIN_ERR   (-0.1)  // avoid dropout on spurious readings near zero
#define OVERCURRLIM	  5		// this is the legitimate max above operating


// CP mode
#define RP_DROPOUT_V (0.1)    // Volts below which CP/CR modes will not attempt to regulate
#define PDROPOUT	 (0.1)
#define PMAXOP		 (150)	// also SOA check. No daughter - fixed in daughterSense()
#define OVERPOWERLIMIT  10 // error if more than this
//#define MAX_HYST	(PMAXOP/20)	// 5% drop before limiting is released

// CR mode
#define RDROPOUT	 (0.20) // // min current for proper operation (needs to be > RADSLO_I)
#define RMAXOP		 (999.99)

// CC
#define CC_GUESS  0.5 // take a guess to begin for jumps bigger than this

// CV

#define TMAXOP		 (65)	// 65 Deg C
#define MINSLEW 	  (5)	// mS

#define ADSMAXCOUNT	 0x7FFF // 15 bits
#define ESPMAXCOUNT  0x0FFF // 12 bits
#define ESP_SAFE	 100		// counts either side of min/max

#define MINLOG 100 	// mS minimum log interval
#define MAXLOGTIME 999998  // uint32 6 digits, no overflow

// recompile with different software version when changing
#ifdef SOATEST
  #define VMAXOP     (20.0) // reduced values for testing
  #define IMAXOP     (1.5)  // half desired values with daughter
  #define PMAXOP     (20.0) // half
  #define TMAXOP     (26) 
  #define OVERCURRLIM    0.25 
  #define OVERVOLTLIM    2 
  #define OVERPOWERLIMIT  2
#endif

#define SHORTCIRCUIT (IMAXOP+OVERCURRLIM)   // When to trip short circuit protection - saved in HAL profile - changed by control_setup()
#define VOMAXLIM     (VMAXOP+OVERVOLTLIM) // throw an error and turn off

float   iMaxOp = IMAXOP;    // used in calculations - modified if daughter board present. See control_setup() and SOA check.
float   pMaxOp = PMAXOP;    

#define OUT_OF_RANGE (-1.0)	// real world value returned when count is outside acceptable limits
// design or prototype CAL values
// For voltage and Current measurements, below the "CxxxLO_X" values the real world value is unmeasurable. 
//    -1 will be returned by the measurement routine.
// the exception is ADS voltage, where differential measurement may return small negative values 
//    (> -0.3V) if reverse voltage is applied to the load.
//ADS1115 ADC

/*****************************************************************************************
 ********** NEED TO CHANGE SOFTWARE VERSION (UPDATE PROFILE) IF THESE CHANGE  ************
 *****************************************************************************************/
// used by settingsGap()
#define ADC_ADS		1
#define ADC_ESP		2	

// low cal point
#define CADSLO_V  	(0)		// count offset for zero volts in. Differential - so zero.
// min viable result - may be a bit above flatline
#define RADSMIN_V 	(0.10)  // can't control in CV mode below this point	
#define RADSNEG_V   (-0.5)	// reverse voltage test (less than a MOSFET body diode drop)
#define ADC_TOO_LOW   (-2) // value returned if ADC count is below bottom limit. 
#ifdef KELVIN_MOD
  #define RADSHI_V	(47.9)	// real world
  #define CADSHI_V  	(9074)	// counts
#else
  #define RADSHI_V  (24.2)  // real world
  #define CADSHI_V    (2520)  // counts
#endif

// Min real current value must be zero for currentZeroCal() to work properly
#define RADSLO_I	(0) // ADS low Current (should be lower than min possible setting: IDROPOUT)
#define CADSLO_I	(13)
#define RADSMIN_I 	(0.01)  	// min viable result (return zero below this point) 

#define RADSHI_I	(3.0)
#define CADSHI_I	(3360)
#define RADS_MIN_R (-1) 

//ESP ADC - values will have zero offsets
#define ESPREADS	5	// multiple reads to gain better precision - each takes 85uS - 5 is too many, causes dropped interrupts
#define CESPLO_V	720	// reading at no volts input (flatline) - this gets changed by Cal()
#define ESPOFF_V	(0.65)	// offset for all voltages
#define RESPMIN_V 	(3.0)  	// min viable result. > ESPOFF_V  
#ifdef KELVIN_MOD
  #define RESPHI_V	(47.9)
  #define CESPHI_V	(2096)	
#else
  #define RESPHI_V  (24.5)
  #define CESPHI_V  (10000) 
#endif
// Min real current value must be zero for currentZeroCal() to work properly
#define CESPBASE_I	720	// reading at no amps input (flatline) - this gets changed by Cal()
#define RESPLO_I	(0.0)	// count at no amps 
#define CESPBASEPLUS_I	16
#define CESPLO_I	(CESPBASE_I+CESPBASEPLUS_I)	// zero offset: count @ I = 0
#define ESPOFF_I	(0.0) // now zero-based 
#define RESPMIN_I 	(0.30)  // min viable result. > ESPOFF_I

#define RESPHI_I	(1.728)
#define CESPHI_I	(855)	// ESP values will have low value offsets

#define RESPLO_T	(22)
#define CESPLO_T	(1972)	// ESP values will have low value offsets

#define RESPHI_T	(60)
#define CESPHI_T	(800)	// ESP values will have low value offsets

// need to define ADS_GAIN  // FSD:  GAIN_TWO = 2.048V 
// ADS1115: +/- 15 bits 
// Hardware abstraction and calibration for ADCs (HAL) 

//**** MCP4725 DAC
// three point approximation. Non-linear at low values
// cal values are without daughter. 
// * values get adjusted by Control_Setup() when daughter/not changes
#define MAXDAC 		4095
#define RDACLO_I 	(0.006) // * adjust this for daughter - see ampsToDAC() and daughterSense()
#define CDACLO_I 	(35)	// make this a bit less than the real value to ensure turn off
#define DAC_MINCOND_C (0) //  MOSFETS not conducting when Off.
#define RDACMID_I 	(0.06)  // * adjust this for daughter
#define CDACMID_OFF 25		// curve is sharper for values below RDACMID_I
#define CDACMID_I 	(CDACLO_I + CDACMID_OFF) 
#define RDACHI_I 	(1.683) // * adjust this for daughter
#define CDACHI_I 	(500)
#define CDAC_OPMAX  (MAXDAC)  // dac for max operational amps 
#define KICK 		 0.01     // mA - small setting to move output off zero amps
#define DAC_SMALLJUMP	1
#define DAC_BIGJUMP		10

#define KMINV 0.1   // minimum volts for Kelvin test to succeed

struct hal_cal {
  // real world values
  float 	minVal;	   	  // limiting safe operating real values
  float 	maxVal;
  float 	minErr;	   	  // manage error if outside these real value limits
  float 	maxErr; 
  // cal real world
  float   lowCalVal;    // real value at low cal point
  float		midCalVal;	  // three point, if needed (DAC, maybe ESPADC)
  float   hiCalVal;     // real value at high cal point
  // cal ADC counts
  uint16_t 	lowCalCount;  // counts at low cal point  
  uint16_t	midCalCount;  // three point, if needed
  uint16_t 	hiCalCount;
};

#define NUM_CALS 6
#define ADS_V 	0
#define ADS_I 	1
#define ESP_V 	2
#define ESP_I 	3
#define ESP_T 	4
#define DAC		  5

// initally set low/high cal values from design params
// ***** Amps/volts Min real values must be zero for calibration functions to work properly
// mid values only used for DAC linearisation, ignored for ADCs
hal_cal halCal[NUM_CALS] = {
	{VDROPOUT, VMAXOP, RADSNEG_V, VOMAXLIM, 0.0, 0.0, RADSHI_V, CADSLO_V, 0, CADSHI_V},  // ADS_V - minErr < 0 for reverse voltage detection
	{IDROPOUT, IMAXOP, IMIN_ERR, SHORTCIRCUIT, 	0.0, 0.0, RADSHI_I, CADSLO_I, 0, CADSHI_I},  // ADS_I - see control_setup()
	{VDROPOUT, VMAXOP, RADSNEG_V, VOMAXLIM,   	0.0, 0.0, RESPHI_V, CESPLO_V, 0, CESPHI_V},  // ESP_V
	{IDROPOUT, IMAXOP, IMIN_ERR, SHORTCIRCUIT, 		0.0, 0.0, RESPHI_I, CESPLO_I, 0, CESPHI_I},  // ESP_I
	{10,       TMAXOP, 10, TMAXOP, 		RESPLO_T, 0.0, RESPHI_T, CESPLO_T, 0, CESPHI_T},  // ESP_T
	{0,             0, RADS_MIN_R, VOMAXLIM, 	RDACLO_I, RDACMID_I, RDACHI_I, CDACLO_I, CDACMID_I, CDACHI_I}  // DAC - see ampsToDAC(). minVal/maxVal not used.
};
hal_cal hal_defs[NUM_CALS]; // factory defaults - set before EE read
// settings BEFORE saved values have been read from EEPROM
// move most of these to the control profile.

#define MCPMAXVAL 255

// these alter behaviour of control() stop hunting, control dVdT
#define HYSTERESIS 		0       // (MCP steps) Voltage coarse control. 
#define BIGJUMP			10		// when to damp a coarse jump
#define DAMP_VOLT	    (0.8)	// damping factor for big jumps of coarse voltage
#define FHYSTERESIS 	8       // V fine control (slightly more than half the steps/mV value)
//#define SHYSTERESIS 1         // SMPS control
#define DAMP_ILIMIT     (0.8)   // damping factor for current limiting (to reduce hunting)
#define I_HYST			(0.003)	// hysteresis for iLimit
#define IND_OFF			0		// limiting disabled (indicator)
#define IND_EN			1		// limiting enabled
#define IND_ACT			2		// enabled and actively limiting

#define DVDT			(0.1)	// desired maximum rate of voltage change V per mS - used to stop overshoot with big voltage changes
#define DVDC			(DVDT * MS_CYCLE) // max DV per control cycle

#define WSTART       (0x00)   // start_up (safe) value for main wiper (VO)
#define DSTART       (DAC_MASK/2)   // start fine control mid-range
#define VSTART       (0.0)    // start_up main voltage

#define VINTERNAL_S   (7)     // SMPS voltage required to sustain internal supplies

#define VSMALL_DIFF 0.00001	// generic value for "significant" difference to floats
#define SMALL_DIFF 0.001  // generic value for "significant" difference to floats
#define SMALLISH_DIFF 0.008 // a bit less than MEDIUM
#define MEDIUM_DIFF 0.01	// 99%
#define MIDDLE_DIFF 0.1
#define LARGE_DIFF  0.5   // used for coarse limiting 
#define BIG_DIFF    1.0   // used for coarse limiting 

// settings.mode
#define C_MODES  5
#define MODE_CV  1
#define MODE_CC  2
#define MODE_CP  3
#define MODE_CR  4
uint8_t inMode[C_MODES]; // used by display to colour buttons
char modeNames[C_MODES][3] = {"", "CV", "CC", "CP", "CR"};

#define C_VMODES 3
char vModeNames[4] = "AMK";

// settings.modeB
#define B_MODES   3
#define MODE_OFF  0	// neither BAT or STEP selected
#define MODE_NOR MODE_OFF
#define MODE_ST   1 //  step
#define MODE_BAT  2 // battery test
char modeNamesB[B_MODES][5] = {"NOR", "STEP", "BAT"};


// values for _curState
#define BMODE_OFF  0	// unselected  
#define BMODE_SEL 1   	// selected, but idle - for STEP or BAT modes when button pushed. Both exit to NOR mode - controlled by stepCalc() and batCalc().
#define BMODE_RUN  2   	// selected, running
uint8_t isModeB[B_MODES]; // which mode is active?
uint8_t _curState; 		// RUN state for ST selected mode

// STEP mode has N cycles of N steps between N stages (2)
uint16_t _curCycle = 0;	// which cycle are we in for STEP
uint8_t _curStage = 0;	// start with s[0] transient
uint16_t _curStep;		// current step in triangle / sawtooth
bool _slewing = false;
long _stepTzero;	// start of slew or dwell time
bool notSTmode; // see setMode() - may not need this.

// real world converted values
#define ADSMUXV 0	// match readings to ADS channels
#define ADSMUXI 1
struct measurements {
	float ADSvolts;
  float ADSvoltsAvg;
	float ESPvolts;
	float ADSamps;
	float ESPamps;
	float ESPtemp;
} volatile meas;

float _lastVolts, _lastAmps; // no load voltage used in estimating DUT ESR in CV mode

// raw counts from ADCs
struct ADCcounts {
	int16_t ADSv;
	int16_t ADSa;
	int16_t ESPv;
	int16_t ESPa;
	int16_t ESPt;
} volatile ADCcount;		
// spread the temps for hysteresis
// fan should be full on, moderately above room temp (say 10 deg).
#define TEMP_FANSTART 	(28.0)  // fan idles below this (deg C)
#define TEMP_FANFULL 	(35.0)  // fan at 100%
#define TEMP_SHUTDOWN 	(65.0)  // turn output off
#define FAN_MIN_PWM		30	// percent
// settings to save between sessions
// each 
struct transient {
	float volts;
	float amps;
	uint16_t time; // millis
	uint16_t steps;
};

#define NSTAGES 2	// number of transient states 
struct settings {
	float    voltage;		// set point
	float    current;		// set point
	float    resistance;		// set point
	float    power;			// set point
  char     vMode;     // voltage measurement mode
	struct   transient s[NSTAGES];// set points, STEP mode //s1, s2
	uint16_t slew;		    // STEP mode rise/fall time: millis/A
	uint16_t cycles;		// STEP mode min is 1 
	uint8_t  mode;			// control mode: CC is default
	uint8_t  modeB;			// TR, BATtery test or STep mode
	uint32_t logTime;		// time between log samples (0 == auto)
  } pSet = {10.0, 0.1, 100.0, 20.0, 'A', {{10.0,0.1,10, 0}, {10,2.0,10, 5}},  0, 100, MODE_CC, MODE_OFF, 100};
settings dynSet, lastSet;
// don't use vMode in dynSet - it gets overwritten dynamically
char _cur_vMode = 'M';

bool	  _outOn; 			// output is on 
bool    _batStop = false;	// flag to exit BAT mode gracefully
//bool 	limiting;  		    // set by control()
bool 	  _isLimiting = false; // SOA limiting
uint8_t daughter_mul = 1;	// set by control_setup(): 1 for none, 2 for daughter board
int 	  _lastDAC = 0;		// last DAC setting - to calculate delta in fine mode
int 	  _DACis;				// current DAC setting
// circular log buffer 
// next item to fill = (_logCount + _logHead) % MAXLOG 
uint32_t _logCount = 0;	// number of current entries
uint32_t _logHead = 0; // head pointer
uint32_t _tLogStarted;	// time when this logging sequence began (mS)
struct logs { // 32 bytes per entry
	uint32_t time;
	float volts;
	float amps;
	float setting;
	char mode[8];
	char function[8];	
} logData[MAXLOG];

#ifdef SAVEBATLOG
uint32_t _batLogCount = 0;  // number of current entries
uint32_t _batLogHead = 0; // head pointer
uint32_t _tBatLogStarted;  // time when this logging sequence began (mS)
bool _batLogIsMostRecent = false;
// battery test log
struct batlog { // 24 bytes per entry
  uint32_t time;
  float ocvolts;
  float ccvolts;
  float amps;
  float esr;
  float mah;
  float mwh;
} batLogData[MAXBATLOG];

#endif

// calibration menu
float calOff_I = 0.0,  
	  calOff_V = 0.0; // setting variables for offset calculation 

short  highButton = -1;	// currently selected screen button
// setting PWM properties
#define PWM_FREQ  15000 
#define PWM_RESOLUTION  8
#define MAXPWM  ((1 << PWM_RESOLUTION) -1)
#define FANIND	(MAXPWM/2)
#define FANIND2	(MAXPWM*7/8)
bool fanIsOn; // fan on indicator
int _curPWM;
uint32_t _lastLogged; // last log item written

#define WIFI_DISCON 0
#define WIFI_CONN   1
#define WIFI_SOFTAP 2
bool    WifiConnected;  // flag set by WiFi (merge with wifiStatus?)
uint8_t wifiStatus = WIFI_DISCON;

// screen error and errorPkt types
#define MSG_NOTIFY		0x00	// lowest level, local nofication, not an error
#define MSG_ERROR		0x01	// local error, no need to transmit
#define MSG_BCAST		0x02	// an error message that needs to be displayed remotely (e.g. Teensy I2C failure, no serial packets...)

struct scrErr {
    char message[128];
    uint16_t bgcol;
    int16_t onTime;
    bool logo;
}
scrE;

#endif
