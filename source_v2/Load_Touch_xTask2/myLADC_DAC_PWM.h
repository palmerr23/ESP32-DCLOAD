#ifndef  LADCDACPWM_H
#define  LADCDACPWM_H
// ********* HAL for ADC/DAC. ESP and ADS/MCP *********

// *************** move pin defs to myLinst.H

//  ESP ADC (secondary for V & I, primary for T)
#ifndef SOFT_VERSION // stand-alone testing
#define ADC_BV   35 //  ESP ADC1
#define ADC_BI   34 //  ESP ADC1
#define ADC_BT   32 //  ESP ADC2
#define PWM_PIN  26 
#endif

// setting PWM properties
const int PWMChannel = 0;

#include "Adafruit_MCP4725RP.h"
#ifndef SOFT_VERSION
Adafruit_MCP4725 dac;
#endif 

#include "myLADS1115.h"
#ifndef SOFT_VERSION
Adafruit_ADS1115 ads(0x48);
#endif 

// float version of map()
inline float fmap(float val, float fromLow, float fromHi, float toLow, float toHi)
{
	return (float)(toLow) + ((float)(val) - (float)(fromLow)) * ((float)(toHi) - (float)(toLow)) / ((float)(fromHi) - (float)(fromLow));
}
// convert counts to real-world values
inline float ADStoV(int ADCval) // volts at VOUT terminal
{ 
	float temp = fmap(ADCval, halCal[ADS_V].lowCalCount, halCal[ADS_V].hiCalCount, halCal[ADS_V].lowCalVal, halCal[ADS_V].hiCalVal);
    //if(temp < 0.0) return 0.0;
	return temp;
}
inline float ADStoA(int ADCval) // aggregate source current
{
	float temp = fmap(ADCval, halCal[ADS_I].lowCalCount, halCal[ADS_I].hiCalCount, halCal[ADS_I].lowCalVal, halCal[ADS_I].hiCalVal);
    if(temp < 0.0) return 0.0;
	return temp;
}

// test routines direct reads, no interrups
float readADS_V(void)
{
	float ADCval = ads.readADC_Differential_0_1();
	//Serial.printf(" [V %iC] ",ADCval);
	ADCcount.ADSv = 	ADCval;
	meas.ADSvolts = ADStoV(ADCcount.ADSv);
	if(meas.ADSvolts < halCal[ADS_V].minErr)
	{
		meas.ADSvolts = halCal[ADS_V].minErr;
		return ADC_TOO_LOW ; //OUT_OF_RANGE;
	}
	return meas.ADSvolts; 
}
float readADS_I(void)
{
	//int ADCval = ads.readADC_SingleEnded(2) ;
	float ADCval = ads.readADC_Differential_2_3();
	ADCcount.ADSa = 	ADCval;	
	meas.ADSamps = ADStoA(ADCcount.ADSa);
	if(meas.ADSamps < halCal[ADS_I].minErr)
	{
	  meas.ADSamps = halCal[ADS_I].minErr;
	  return ADC_TOO_LOW ; //OUT_OF_RANGE;
	}
	//Serial.printf(" [A %iC] ",ADCval);
	return meas.ADSamps;
}


// primitives for testing
// read values - return counts ******* They do not update the meas.x values *********
inline int CreadADS_V(void)
{  
	ADCcount.ADSv = ads.readADC_Differential_0_1();
    return ADCcount.ADSv ;
}

inline int CreadADS_A(void)
{ 
  ADCcount.ADSa = ads.readADC_Differential_2_3();
  return ADCcount.ADSa ;
 }

float ADStoVread(int ADCval) // ADC read voltage
{
  return (float) ADCval * ADCMAXVOLTS / ADCsteps;
}

// **** Secondary ADC - ESP32 ****

// simple linear approximation suffices

// primitives for testing
inline int CreadESP_V(void)
{  
  // throw one value away?
  return analogRead(ADC_BV);
}
// ESP read current 
inline int CreadESP_I(void)
{ 
  return analogRead(ADC_BI);
}
inline int CreadESP_T(void)
{ 
  return analogRead(ADC_BT);
}

// convert ESP ADC counts to real-world values
// ESP ADC is non -linear. Use linear portion and map counts to values
// voltages are shifted up 0.7V in hardware, other than Temp
// -1.0 is returned below the minimum legitimate count (i.e. too low to measure) to flag the value is illegitimate
// measured value is set to 0.0
inline float ESPtoV(int ADCval) // ESP_V counts to real volts 
{
	// offset for all readings
	return ESPOFF_V + fmap(ADCval, halCal[ESP_V].lowCalCount, halCal[ESP_V].hiCalCount, halCal[ESP_V].lowCalVal, halCal[ESP_V].hiCalVal);
  //return fmap(ADCval, CESPLO_V, CESPHI_V, 0, RESPHI_V) ;
}

inline float ESPtoI(int ADCval) // aggregate source current. ESP_V counts to real amps 
{
	return ESPOFF_I + fmap(ADCval, halCal[ESP_I].lowCalCount, halCal[ESP_I].hiCalCount, halCal[ESP_I].lowCalVal, halCal[ESP_I].hiCalVal);
  //return (float) fmap(ADCval, CESPLO_I, CESPHI_I, 0, RESPHI_I) ;
}

inline float ESPtoT(int ADCval) // ESP_T counts to Temp 
{
	return fmap(ADCval, halCal[ESP_T].lowCalCount, halCal[ESP_T].hiCalCount, halCal[ESP_T].lowCalVal, halCal[ESP_T].hiCalVal);
  //return fmap(ADCval, CESPLO_T, CESPHI_T, RESPLO_T, RESPHI_T);
}

// read and store ESP raw ADC counts and converted values
// ******* USE THESE FOR MAINLINE PR0CESSING ******
inline float readESP_V(void)
{
	int ADCval = 0;
	/*  don't need great accuracy */
	analogRead(ADC_BV); // throw away the first one
	for (int i = 0; i < ESPREADS; i++)
	   ADCval += analogRead(ADC_BV);
    ADCval = ADCval/ESPREADS;

	ADCcount.ESPv = ADCval;// = analogRead(ADC_BV);
	//Serial.printf(" [%i] ",ADCval);
	meas.ESPvolts = ESPtoV(ADCval);
	if(meas.ESPvolts < halCal[ESP_V].minErr)
	{
		meas.ESPvolts = halCal[ESP_V].minErr;
		return ADC_TOO_LOW ; //OUT_OF_RANGE;
	}
	//meas.ESPvolts = fmap(ADCval, CESPLO_V, CESPHI_V, RESPLO_V, RESPHI_V) ;
	return meas.ESPvolts;
}

inline float readESP_I(void)
{
	int ADCval = 0;
	/* don't need great accuracy - single read error = +/- 5 counts	*/
	analogRead(ADC_BI); // throw away the first one
	for (int i = 0; i < ESPREADS; i++)
	   ADCval += analogRead(ADC_BI);
	//Serial.printf(" [%i] ",ADCval);
	ADCval = ADCval/ESPREADS;

	ADCcount.ESPa = ADCval;// = analogRead(ADC_BI);
	meas.ESPamps = ESPtoI(ADCval);
	if(meas.ESPamps < halCal[ESP_I].minErr)
	{
		meas.ESPamps = 	halCal[ESP_I].minErr;
		return ADC_TOO_LOW ; //OUT_OF_RANGE;
	}
	//meas.ESPamps =  fmap(ADCval, CESPLO_I, CESPHI_I, RESPLO_I, RESPHI_I) ;
	return 	meas.ESPamps;
}

// simple aggregate function to update VI counts and measurements from ESP
// returns false if one or more readings are out of range
bool getESPviReadings(void)
{
	float v, i;
	  v = readESP_V();  	 
	  i = readESP_I();
	  return !(v < halCal[ESP_V].minErr || i < halCal[ESP_I].minErr);
}

inline float readESP_T(void)
{
	int ADCval = analogRead(ADC_BT);
	ADCcount.ESPt = 	ADCval;
	//Serial.printf(" %i ",ADCval);
	// map works OK outside the range
	meas.ESPtemp =  ESPtoT(ADCval);
	if(meas.ESPtemp < halCal[ESP_T].minErr)
	{
		meas.ESPtemp = 	halCal[ESP_T].minErr;
		return ADC_TOO_LOW ; //OUT_OF_RANGE;
	}
	//meas.ESPtemp = fmap(ADCval, CESPLO_T, CESPHI_T, RESPLO_T, RESPHI_T);
	return meas.ESPtemp;
}

// **** DAC MCP4725 ****
// three point approximation 
// still not perfect at low values - control loop will manage, result should be low side.
int ampsToDAC(float amps)
{   int dacVal;
	if(amps < RDACMID_I)
		dacVal = fmap(amps, RDACLO_I, RDACMID_I, CDACLO_I, CDACMID_I);
	else
		dacVal = fmap(amps, RDACMID_I, RDACHI_I, CDACMID_I, CDACHI_I);
	dacVal	= constrain(dacVal, 0, CDAC_OPMAX);	
#ifdef C_DEBUG   
	//Serial.printf("A2D: %2.3f, %i\n", amps,dacVal);
#endif
	return dacVal;
}

// set dac by counts
int dacSet(int DACval)
 { 
  DACval = constrain(DACval, 0, CDAC_OPMAX);
  //Serial.printf(" |DS: %i| ", DACval);
  dac.setVoltage(DACval, false);
  _lastDAC = DACval;
  return DACval;
}

int dacRead(void)
{
	return dac.getRegValue(MCP4725_READ_DAC);
}
// set dac by current
// returns dac count for fine / coarse adjustment
inline int dacSetAmps(float amps)
{
	return dacSet(ampsToDAC(amps));
}

int setFan(int PWMpercent)
{ 
  int tempP = constrain(PWMpercent * MAXPWM / 100, 0, MAXPWM);
  ledcWrite(PWMChannel, tempP);
  _curPWM = tempP;
  return tempP;
  //Serial.printf(" [%i]",tempP);
}
// fan speed in percent
int readFan(void)
{
		return _curPWM = 100 * _curPWM / MAXPWM;
}

#endif
