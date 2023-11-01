#ifndef MYLCONTROL_H
#define MYLCONTROL_H
/*
 Control the DAC
 Exclusive: I2C control from these routines called by ControlTask()
 Watch out for stack size if writing to Serial from these routines (overflow may restart ESP). 
*/
int dxxx =0;
void IRAM_ATTR ads_ISR();
void onOff(int8_t channel, bool status);
void onOffKill(int8_t channel, bool status);
short SCPIsendGrpMessage(uint8_t tgrp, char *buf);
uint8_t setMode(uint8_t aMode);
uint8_t setModeB(uint8_t bMode);
int limitMe(float curVal, float setPoint, float readV, float readI, int modeX);
void errorKill(bool shutOff, bool DACzero, float reading, const char errMsg[], bool nonBlock);
int modeCalc(float readV, float readI, uint8_t c_speed, bool guessFirst, bool printMe);
bool daughterSense(void);
bool getESPviReadings(void);
bool addLog(bool mustLog);
void addLogItem(float setting, const char mode[], const char func[], bool tsec);
void addBatLogItem(float ocvolts, float ccvolts, float amps, float esr, float mah, float mwh);
void restartLog(void);
void restartBatLog(void);
void  averageVolts(void);
void printADSavg(void);
void printLog();
void printBatLog();
//int dacRead(void);

bool changedLocal = true; // last value changed was local (or remote)
//bool shutDown = false;
bool firstGuess = true; // deal with non-linear cases where first guess is outside the C_VARCOARSE window, also at switch on
float openV;

// coarse and fine control for main output with limiting and tracking
char estop[] = ":TRAC:ESTO";
int cVal = 0;
// must be non-blocking

settings t; // used by BAT and STEP modes. Needs to be persistent, as most times routines just exit.

/* ==== Battery Test ====
 * set and hold appropriate discharge setting
 * every BAT_MEASURE_INTERVAL (and at start) disconnect for several seconds and measure open circuit voltage.
 *   calculate ESR and display - need to stop normal resistance calculation
 * monitor for final voltage, when reached, measure ESR again.
 * go to NOR/Off mode.
 * display results - errorI: mod to hold until screen touched.
 */
long tLastBatUpdate, tBatStart, tSinceLastBatUpdate;
float batAH, batMAH, batWH, batMWH; // amp hrs and watt hours, running total
float fValue;
//#define BAT_MEASURE_INTERVAL 20 // seconds between battery measurements
#define BAT_WAIT_MEASURE_TIME 1000 // milliSeconds of zero current for battery recovery before measuring OC voltage
float batOCvolts = 999, batCCvolts = 999, batCurrent = 999;
bool batUpdateMode;
float esr;
bool preStart = false;
#define MS_TO_HRS 3600000 // (3600 * 1000)

float batVoltsFiltered;

#define BT_START      0
#define BT_FIRST      1
#define BT_RUNNING    2
#define BT_WAIT_MEAS  3
#define BT_DONE       4
uint8_t batTestState = BT_DONE;

// returns true while battery test is active
bool batTest(void) {
  int i;
  int bmi = pSet.logTime * 1000;
  // 2 step start: 
  // step 1: prestart - turn On and set current to zero
  if (_curState == BMODE_SEL) // start   
  { 
    tBatStart = tLastBatUpdate = millis();
    Serial.printf("Battery test - start %i\n", tLastBatUpdate);    
    onOff(0, true); // resets log
    dynSet.current = 0.0;   // turn discharge current to zero to measure OCV
    dynSet.power = 0.0;     // could be CP or CR mode
    dynSet.resistance = 9999.0;
    preStart = true;  
    _batStop = false;
    _curState = BMODE_RUN;  
    batTestState =  BT_FIRST;
    //restartBatLog();
    return true;
  }

  tSinceLastBatUpdate = (millis() - tLastBatUpdate);
    
  // prep step 2: after startup + measurement delay, log the initial values; then begin the test
  if (batTestState == BT_FIRST && tSinceLastBatUpdate > BAT_WAIT_MEASURE_TIME) 
  {       
    batOCvolts = meas.ADSvoltsAvg;
   //Serial.printf("Bat start 2:  CC %2.3fV, dacVal %i, set %2.3fA,\n", batOCvolts, _DACis, dynSet.current);
    //batCurrent = meas.ADSamps;
    //Serial.printf("Bat: 2 %2.3fV %2.3fA @ %i\n", batOCvolts, batCurrent, millis());    
    addLogItem(pSet.voltage,"TERM_V", "BAT", true); 
    if (pSet.mode == MODE_CC)   
      addLogItem(pSet.current,"DIS_A", "BAT", true);  
    if (pSet.mode == MODE_CP)
      addLogItem(pSet.power,"DIS_P", "BAT", true);
    if (pSet.mode == MODE_CR)
      addLogItem(pSet.resistance,"DIS_R", "BAT", true);
      
    restartBatLog();
    addBatLogItem(batOCvolts, 0.00, pSet.current, 0.0, 0.0, 0.0);   // assumes CC mode
   
    //batUpdateMode = false;
    batAH = 0.0;
    batWH = 0.0;
    
    dynSet = pSet; // turn discharge on after measuring OCV
    //preStart = false; // reset for next time.
    tLastBatUpdate = millis();
    batTestState = BT_RUNNING;
    return true;
  }

  // If we get to this point startup is complete and testing is active

  //  Are we finished?  filtered voltage < set point
  // _batStop by operator
  // _avg_ready stops exit before first average reading run is complete
  if (((batTestState == BT_RUNNING && meas.ADSvoltsAvg <= pSet.voltage) && _avg_ready) || _batStop) 
  {    
    Serial.printf("Bat test stop: %i %3.3f V, %3.3f A\n", millis(), meas.ADSvoltsAvg, meas.ADSamps);
    printADSavg();
    batCCvolts = meas.ADSvoltsAvg;
    if(batTestState ==  BT_RUNNING)
      batCurrent = meas.ADSamps; // update final measurement    
    fValue = batCurrent * (tSinceLastBatUpdate)/MS_TO_HRS; // Amp Hours since last interval.
    batAH += fValue;
    batMAH = batAH * 1000;
    batWH += fValue * batCCvolts;
    batMWH = batWH * 1000;
    tLastBatUpdate = millis();
    batTestState = BT_DONE;
    _batStop = false;
    return true;
  }
  // battery measurement interval - stop discharge to stabilise OC voltage
  if(batTestState ==  BT_RUNNING && tSinceLastBatUpdate > bmi) // && !batUpdateMode
  {
    // Serial.println("B");
    batCCvolts = meas.ADSvoltsAvg;
    batCurrent = meas.ADSamps;
    //Serial.printf("Bat: 3 CC %2.3fV, dacVal %i\n", batCCvolts, _DACis);
    printADSavg();
    if(dynSet.logTime == 0) // SYNC mode - one reading per cycle
      addLog(true);
    fValue = batCurrent * (tSinceLastBatUpdate)/MS_TO_HRS; // Amp Hours since last interval. Not discharging while waiting to measure, so add last period.
    batAH += fValue;
    batMAH = batAH * 1000;
    batWH += fValue * batCCvolts;
    batMWH = batWH * 1000;
    dynSet.current = 0.0; // discharge current to zero for OC voltage measure in next state.
    dynSet.power = 0.0;
    dynSet.resistance = 9999.0;
   // batUpdateMode = true; // delay another period and then measure OC volts
    batTestState =  BT_WAIT_MEAS;
    tLastBatUpdate = millis();
    //Serial.printf("@ Set %3.2fA ", dynSet.current);
    return true;
  }
 
  // battery measurement interval + battery recovery - measure and restart discharge
  // miss this activity if LI (bmi) == 0
  if(batTestState ==  BT_WAIT_MEAS && tSinceLastBatUpdate > BAT_WAIT_MEASURE_TIME  && bmi > 0) //&& batUpdateMode
  {
    //Serial.println("C");
    // measure ESR
    batOCvolts = meas.ADSvoltsAvg;   // Open circuit voltage
    //Serial.printf("Bat: 4 OC %2.3fV, dacVal %i, set %2.3fA\n", batOCvolts, _DACis, dynSet.current);
    printADSavg();
    //batCurrent = meas.ADSamps;
    esr = (batOCvolts - batCCvolts) / batCurrent;
    addLogItem(esr,"ESR", "BAT", true);
    addLogItem(batOCvolts,"OCV", "BAT", true);
    addLogItem(batAH *1000,"CAPmA", "BAT", true);  
    addLogItem(batWH *1000,"CAPmW", "BAT", true);
    addBatLogItem(batOCvolts, batCCvolts, batCurrent, esr, batMAH, batMWH);
    //Serial.printf("BAT regular log: OCV %2.3f, CCV %2.3f[%2.3f], I %2.3f, ESR %2.2f, AH %2.4f, WH %2.4f\n", 
            //    batOCvolts, batCCvolts, meas.ADSvoltsAvg, batCurrent, esr, batAH, batWH); // , DCurr %2.3f , dynSet.current
    dynSet = pSet; // turn discharge back on after measuring OCV and process settings updates   
    tLastBatUpdate = millis();
    //batUpdateMode = false;
    batTestState = BT_RUNNING;
    return true;
  }
  
   // All done, wait for OC voltage to stablise, log and exit.
   if(batTestState == BT_DONE && tSinceLastBatUpdate > BAT_WAIT_MEASURE_TIME) 
   {
    batOCvolts = meas.ADSvoltsAvg;
    esr = (batOCvolts - batCCvolts) / batCurrent;
    float elapsed = (float)(millis() - tBatStart)/ 1000.0;
    addLogItem(esr,"ESR", "BAT", true);
    addLogItem(batCCvolts,"CCV", "BAT", true);
    addLogItem(batAH,"CAPA", "BAT", true);
    addLogItem(batWH,"CAPW", "BAT", true);
    addLogItem(elapsed,"TIME", "BAT", true);
    addBatLogItem(batOCvolts, batCCvolts, batCurrent, esr, batMAH, batMWH);
    Serial.printf("Battery test completed. cap %3.3f AH [%3.3f mAH] %3.3f WH, esr %3.3f\n", batAH, batMAH, batWH, esr);
    char msgbuf[128];
    sprintf(msgbuf, "Battery test:\nCapacity\n%3.3f AH\n%3.3f mAH\n%3.3f WH\nESR %3.3f Ohms\nTime %3.1f S", batAH, batMAH, batWH, esr, elapsed);
    onOffKill(0, false);
    screenError(msgbuf, MY_DARKGREEN, -1, false);  // display until screen touch    
    //_batStop = false;
    _curState = BMODE_OFF;
    printLog();
    printBatLog();
    return false;        
  }
  return true;
}
/*
// battery test
// set and hold appropriate discharge setting
// every BAT_MEASURE_INTERVAL (and at start) disconnect for several seconds and measure open circuit voltage.
//   calculate ESR and display - need to stop normal resistance calculation
// monitor for final voltage, when reached, measure ESR again.
// go to NOR/Off mode.
// display results - errorKill: mod to hold until screen touched.
long tLastBatUpdate, tBatStart;
float batAH, batMAH, batWH, batMWH; // amp hrs, running total
float fValue;
#define BAT_MEASURE_INTERVAL 20 // seconds between battery measurements
#define BAT_EVERY 20 // seconds
#define BAT_WAIT_MEASURE_TIME 1 // seconds
float batOCvolts = 999, batCCvolts = 999, batCurrent = 999;
bool batUpdateMode;
float esr;
bool preStart = false;
#define MS_TO_HRS (3600 * 1000)
#define NUM_AVG_VOLTS 5 // times to read before exiting 
int termCount;
float avgTermV;
// returns true while running
bool batTest(void)
{
  // 2 step start: 
  // step 1: turn On and set current to zero
  if (_curState == BMODE_SEL) // start mode
  {	 
  termCount = 0; 
  avgTermV = 0.0;
	Serial.printf("Battery test - start\n");
	tLastBatUpdate = millis();
	dynSet = pSet; // use main screen settings
	onOff(0, true); // resets log
	dynSet.current = 0.0; 	// turn discharge current to zero to measure OCV
	dynSet.power = 0.0; 	// could be CP mode also
	preStart = true;	
	_curState = BMODE_RUN;  // after initial logging
	return true;
  }
  // step 2: after 100mS delay we log the initial values; then begin the test
  if (preStart && ((millis() - tLastBatUpdate) > BAT_WAIT_MEASURE_TIME * 1000))  
  {   //Serial.println("A");
	tLastBatUpdate = millis();
	batCCvolts = meas.ADSvolts;
	batCurrent = meas.ADSamps;
	addLogItem(pSet.voltage,"TERM_V", "BAT", true);	
	if (pSet.mode == MODE_CC)		
		addLogItem(pSet.current,"DIS_A", "BAT", true);	
	if (pSet.mode == MODE_CP)
		addLogItem(pSet.power,"DIS_P", "BAT", true);
	
	dynSet.current = pSet.current; // turn discharge back on after measuring OCV

	batOCvolts = 999;
	batUpdateMode = false;
	batAH = 0;
	_batStop = false;
	preStart = false; // reset for next time.
	return true;
  }
  // do stuff periodically
  if(millis() - tLastBatUpdate > BAT_EVERY * 1000 && !batUpdateMode)
  {  // Serial.println("B");
    batCCvolts = meas.ADSvolts;
    batCurrent = meas.ADSamps;
	if(dynSet.logTime == 0)	// SYNC mode - one reading per cycle
		addLog(true);
    batAH += batCurrent * (millis() - tLastBatUpdate)/MS_TO_HRS; // Amp Hours. Not discharging while waiting to measure, so add last period.
	batMAH = batAH * 1000;
    dynSet.current = 0.0; // discharge current to zero for OC voltage measure in next cycle.
	dynSet.power = 0.0;
    batUpdateMode = true; // delay another period and then measure OC volts
    //Serial.printf("@ Set %3.2fA ", dynSet.current);
	return true;
  }
  if(millis() - tLastBatUpdate > (BAT_EVERY + BAT_WAIT_MEASURE_TIME) * 1000 && batUpdateMode)
  {//Serial.println("C");
  // measure ESR
    batOCvolts = meas.ADSvolts;
    //batCurrent = meas.ADSamps;   
    esr = (batOCvolts - batCCvolts) / batCurrent;
	addLogItem(esr,"ESR", "BAT", true);
	addLogItem(batOCvolts,"OCV", "BAT", true);
	addLogItem(batAH,"CAP", "BAT", true);	
	addLogItem(batMAH,"CAPM", "BAT", true);
    Serial.printf("BAT regular ESR. OCV %2.2f, CCV %2.2f, I %2.2f, ESR %2.2f, AH %2.4f\n", batOCvolts, batCCvolts, batCurrent, esr, batAH); // , DCurr %2.3f , dynSet.current
	dynSet.current = pSet.current; // turn discharge back on after measuring OCV
	dynSet.power = pSet.power;
	tLastBatUpdate = millis();
	batUpdateMode = false;
	return true;
  }
  if (meas.ADSvolts <= pSet.voltage || _batStop) // reached terminal voltage?
  {
      _batStop = false;
      batCCvolts = meas.ADSvolts;
      batCurrent = meas.ADSamps; // final measurement
      if(! _batStop) // force stop
      {
        if(termCount < TERMCOUNTS)
        {
          termCount++;
          avgTermV += batCCvolts;
          return true;
        }
        if(avgTermV/TERMCOUNTS > pSet.voltage) // go around again
        {
          termCount = 0;
          avgTermV = 0.0;
        }
      }
      onOffKill(0, false);
      _curState = BMODE_OFF;    
      batAH += batCurrent * (millis() - tLastBatUpdate)/60000; // Amp Hours
      float batMAH = batAH * 1000;
      delay(BAT_WAIT_MEASURE_TIME * 1000);
      batOCvolts = meas.ADSvolts;
      esr = (batOCvolts - batCCvolts) / batCurrent;
      addLogItem(esr,"ESR", "BAT", true);
      addLogItem(batCCvolts,"CCV", "BAT", true);
      addLogItem(batAH,"CAP", "BAT", true);
      addLogItem(batMAH,"CAPM", "BAT", true);
      Serial.printf("Battery test - done. cap %3.3f AH [%3.3f mAH], esr %3.3f\n", batAH, batMAH, esr);
      char msgbuf[128];
      sprintf(msgbuf, "Battery test:\nCapacity %3.3f AH\n%3.3f mAH\nESR %3.3f Ohms\n", batAH, batMAH, esr);
      screenError(msgbuf, MY_DARKGREEN, 10, false);
      
      return false;        
  }
  return true;
}
*/
// calculate the correct dynamic setting in STEP mode.
// multiple dimension state variable machine
// _curStage: are we in an active dynamic mode - stage 0 or 1?
// _stepping: where are we in staircase mode?
// _slewing: in rise/fall mode

float vThisStep, vLastStep, iThisStep, iLastStep;
settings stepCalc(void)
{  
 int nextStage;

   float stepProp;
   if (_curState == BMODE_SEL) // start STEP mode
   {  // set immediately to s[0] values and then start transition to s[1]
     t = pSet; // copy across the major settings
     vLastStep = t.voltage; // start with NORmal mode settings, slew to first stage (see below)
     iLastStep = t.current;
     vThisStep = t.s[0].volts; // this will be state 0, slew to it
     iThisStep = t.s[0].amps;      
     t.power = vThisStep * iThisStep;
	   restartLog();
     t.resistance = constrain(vThisStep / iThisStep, 0,  RMAXOP);   // constrain for zero or tiny current  
     
     _curStage = 0;    
     nextStage = 1; 
     _stepTzero = millis();
     _curCycle = 0;
     _curStep = 0;
     stepProp = 0.0;
     _slewing = true;
     _curState = BMODE_RUN;  
    // Serial.printf("Starting STEP: %i cycle %i, set %3.2f\n",_curStage, _curCycle, t.current); 
     onOff(0, true);
     return t;
   }

   // step mode processing
   int timeInStep =  millis() - _stepTzero ;
   if (_slewing)  // adjust settings until we reach the target
   {       
      if(timeInStep < pSet.slew) //still slewing
      {        
          float slewProp = ((float)timeInStep)/pSet.slew;          
          t.voltage = fmap(slewProp, 0.0, 1.0, vLastStep, vThisStep); // slewing from last (== next) to this
          t.current = fmap(slewProp, 0.0, 1.0, iLastStep, iThisStep);
          t.power = fmap(slewProp, 0.0, 1.0, vLastStep * iLastStep, vThisStep *iThisStep);
          t.resistance = constrain(fmap(slewProp, 0, 1, vLastStep / iLastStep,  vThisStep / iThisStep), 0,  RMAXOP);      
      }
      else // finished slewing, start to dwell on step
      {             
          _slewing = false;
          //Serial.print('.');
          t.voltage = vThisStep;
          t.current = iThisStep;
          t.power = vThisStep * iThisStep;
          t.resistance = constrain(vThisStep / iThisStep, 0, RMAXOP); // divide by zero protection   
          //Serial.printf("Done slewing next %i, set %2.3f\n",_curStage, t.current);
      }
   } 
   else // finished dwelling, next step or stage
   {
      nextStage = (_curStage + 1) % NSTAGES;     
      if(timeInStep > t.s[_curStage].time) // next step
      {             
          // next step
          //Serial.print(':');             
          // leave settings where they are. Need these (reentrant code)
          t.voltage = vThisStep;
          t.current = iThisStep;
          t.power = vThisStep * iThisStep;
          t.resistance = constrain(vThisStep / iThisStep, 0, RMAXOP); // divide by zero protection 
		  if(pSet.logTime == 0)	// SYNC logging	
			addLog(true);
            
          _curStep++;    
          if(_curStep < t.s[_curStage].steps)   
          {
            stepProp = ((float)_curStep)/t.s[_curStage].steps;
            stepProp = constrain(stepProp, 0.0, 1.0); // divide by zero protection  

            // start to slew to next step      
            vLastStep = vThisStep;
            iLastStep = iThisStep;
            vThisStep = fmap(stepProp, 0, 1, t.s[_curStage].volts, t.s[nextStage].volts); 
            iThisStep = fmap(stepProp, 0, 1, t.s[_curStage].amps,  t.s[nextStage].amps);
            
          }     
          else // steps for this stage are complete, change stage
          {            
             //Serial.print('|');      
             // swap this for next      
             vLastStep = vThisStep; 
             iLastStep = iThisStep;
                          
             _curStage = (++_curStage) % NSTAGES; // change stage to s[0] or s[1]
             iThisStep = t.s[_curStage].amps;
             vThisStep = t.s[_curStage].volts;

             _curStep = 0;
             stepProp = 0.0;
            _curCycle++;
          }
          _slewing = (t.slew > 0); // don't slew if turned off 
          _stepTzero = millis();
          //Serial.printf("Done dwelling %i, cyc %i\n",_curStage, _curCycle);
      }
   }
   if(_curCycle > t.cycles && t.cycles != 0) // endless for zero
   {
      setModeB(MODE_NOR); // all done! turn off, back to NOR mode (should reinstate pSet values)
      onOffKill(0, false);
      _curState = BMODE_OFF;      
      t = pSet;  
      //Serial.printf("*** Done STEP\n");
   }
  // if(t.current > 1.6) Serial.printf("Step_OC: slw %i, stp %i, sta %i, t.c %2.3fA, iT %2.3fA, iN %2.3fA\n", _slewing, _curStep, _curStage, t.current, iThisStep, iLastStep);
   return t;
}

// SOA checking only from ADS values as ESP may cause false triggering
// settings may be changed or output turned off in main code
// true is bad!

// only ADS readings as ESP may not be accurate
bool SOAerror(void)
{
	// errorKill turns off output
	if (meas.ADSvolts > halCal[ADS_V].maxErr)
	{
		errorKill(true, true, meas.ADSvolts, "Maximum voltage exceeded", true);
		return true;
	}
	if (meas.ADSamps > halCal[ADS_I].maxErr)
	{		
		errorKill(true, true, meas.ADSamps, "Maximum current exceeded", true);
		return true;
	}
  float p = meas.ADSamps * meas.ADSvolts;
  if (p > pMaxOp + OVERPOWERLIMIT)
  {   
    errorKill(true, true, p, "Maximum power exceeded", true);
    return true;
  }
 
	return false;
}

// returns setting error: (set - val)
float settingsGap(int adcType)
{
	float iRead, vRead;
	if(_isLimiting)
		return -10.0; // always control if limiting
	iRead = (adcType == ADC_ADS) ? meas.ADSamps : meas.ESPamps;
	vRead = (adcType == ADC_ADS) ? meas.ADSvolts : meas.ESPvolts;
	switch (dynSet.mode)
	{
		case MODE_CC : // CR mode
			return dynSet.current - iRead;
			
		case MODE_CR : // CR mode
			return dynSet.resistance - vRead/iRead;
		
		case MODE_CP : // CP mode
		//Serial.printf(" <SG-P %2.3f [%2.3fW = %2.3fV * %2.3fA>\n", dynSet.power - vRead * iRead,dynSet.power, vRead, iRead);
			return dynSet.power - vRead * iRead;
			
		case MODE_CV : // CV mode
			return dynSet.voltage - vRead;		
	}
	return -1.0; // we don't get here
}

// returns setting error: (set - val)/set
float settingsRatioGap(int adcType)
{
	float iRead, vRead;
	if(_isLimiting) 
		return -10.0; // always control if limiting

	iRead = (adcType == ADC_ADS) ? meas.ADSamps : meas.ESPamps;
	vRead = (adcType == ADC_ADS) ? meas.ADSvolts : meas.ESPvolts;
	switch (dynSet.mode)
	{
		case MODE_CC : 
			if(dynSet.current < SMALL_DIFF)
				return 0;
			else
				return 1.0 - (iRead)/dynSet.current;
			
		case MODE_CR : 
			if(dynSet.resistance < RDROPOUT)
				return 0;
			else
				return 1.0 - (vRead/iRead)/dynSet.resistance;
		
		case MODE_CP : 
		//Serial.printf(" <SRG-P %2.3f [%2.3fW = %2.3fV * %2.3fA]>\n", 1.0 - (vRead * iRead)/dynSet.power, dynSet.power, vRead, iRead);
			if(dynSet.power < MEDIUM_DIFF)
				return 0;
			else
				return 1.0 - (vRead * iRead)/dynSet.power;
			
		case MODE_CV : 
		
			if(dynSet.voltage < SMALL_DIFF)
				return 0;
			else
				return 1.0 - vRead/dynSet.voltage;		
	}
	return -1.0;  // we don't get here
}

/******************** main control process ****************/
// simply controls to the values calculated in modeCalc() or Bat
// in STEP mode, current setpoint values are set in main loop.
float iLast, vLast;
// slow (ADS readings, coarse/fine tune) 
// or fast (ESP readings, coarse only) 
// returns current DAC setting
int control(uint8_t c_speed)
{
	float vSetpoint, iSetpoint, iTrack;
	float vRead, vReadS, iRead, tRead, vGap;
	float iGap, iGap2;	
	int jump, dacVal;
	bool rLimit;
	// short fjump;
	bool guessFirst = false;	// *************need to fix code ************	
  switch (dynSet.mode)
  {
    case MODE_CC :
      if(abs(dynSet.current - lastSet.current) > CC_GUESS)
      { 
        //Serial.println("CC FG");
        guessFirst = true;
      }
      break;
   case MODE_CV :
      break;
   case MODE_CP :
      break;
   case MODE_CR :
      break;
      
  }
	if (!_outOn) // don't control if output is OFF
	{
	  return dacSet(DAC_MINCOND_C); // shouldn't get here - controlTask() should catch this.
	}	
  
  SOAerror();     // disconnects on SOA error
  
	// establish the variables for either ESP or ADS 
	if (c_speed == ESP_CONTROL)
	{		 
	  vRead  = meas.ESPvolts;   
	  iRead  = meas.ESPamps;
    if(dynSet.mode != MODE_CC) // wrong ESP readings cause strange results in outcome-seeking code.
      return _lastDAC; 
	  // readings too small for ESP control. CC mode independent of vRead.
	  // use ADS for low values - ESP readings unreliable.
	  if (meas.ADSamps < RESPMIN_I || ((dynSet.mode != MODE_CC) && meas.ADSvolts < RESPMIN_V) )
	  {
#ifdef C_DEBUG   
	//if(_printMe) Serial.printf("Low FAST ESP counts: A %i C, V %i C\n", ADCcount.ESPa, ADCcount.ESPv);
#endif
		  return _lastDAC;	// can't FAST control when ESP readings are invalid, wait for ADS reading
	  }
	  // is gap enough to warrant a control cycle?
	  // always control if limiting
	  iGap = settingsGap(ADC_ESP);
	  iGap2 = settingsRatioGap(ADC_ESP);
	  if(abs(iGap) < C_VARCOARSE && abs(iGap2) < C_VARCOARSE_RAT)
	  {
#ifdef C_DEBUG   
	    //if(_printMe) Serial.printf("FAST not required, gap is : %2.3f\n", iGap, iGap2);
#endif
      lastSet = dynSet; // keep track for changes.
		  return _lastDAC; // no need for coarse control
	  }
	}
	else // ADS FINE control 
	{
		// should we try to control below RADSMIN_I?
//if (batUpdateMode && (dxxx % 20) == 0) Serial.print("^");
	  vRead  = meas.ADSvolts;   
	  iRead  = meas.ADSamps;
	  iGap = settingsGap(ADC_ADS);
	  iGap2 = settingsRatioGap(ADC_ADS);
    bool ilim = (iRead - dynSet.current - MEDIUM_DIFF) > 0;

	  // gap too small to iterate again
	  // test duplicated in main line
    // don't need to current limit
	  if((abs(iGap) < C_VARFINE || abs(iGap2) < C_VARFINER) && !batUpdateMode && !ilim) // load is ON with zero current when reading battery OCV
	  {
#ifdef C_DEBUG   
	    //if(_printMe) Serial.printf("SLOW not required, gap is  %2.3f : %2.3f\n", iGap, iGap2);
#endif
		    lastSet = dynSet; // keep track for changes.
		    return _lastDAC;
	  }

	  if (vRead < RADSMIN_V && dynSet.mode != MODE_CV)      
	  {
#ifdef C_DEBUG   
	    //if(_printMe) Serial.printf("Low SLOW ADS volts: %2.3fV [%i]\n", meas.ADSvolts, ADCcount.ADSv);
#endif		  
		  // return _lastDAC;	// can't control when readings are invalid, wait for ADS reading, slow creep up of current
      // ignore this test for now
	  }
	}

//	float dv;
//if (batUpdateMode && (dxxx % 20) == 0) Serial.print("~");
#ifdef C_DEBUG   
   if(_printMe) Serial.printf("\nControl %c: iRread %5.3f", (c_speed == ADS_CONTROL) ? 'A' : 'E', iRead);
   if(_printMe) Serial.printf(", vRread %5.3f  MC = {\n", vRead);
#endif
//if (batUpdateMode && (dxxx++ % 20) == 0) Serial.printf("! %3.2f",dynSet.current);

  // modeCalc() does all the work to calculate the correct setting for now from mode, readings and time elapsed
	dacVal = modeCalc(vRead, iRead, c_speed, guessFirst, _printMe);
	dacSet(dacVal); 
#ifdef C_DEBUG   
	if(_printMe) Serial.printf("\n} dacVal set %i ", dacVal);
	if(_printMe) Serial.printf(", Gaps %2.3f %2.3f \n", iGap, iGap2);
#endif
    lastSet = dynSet; // keep track for changes.
    return dacVal;
}

// for the mode, calculate the optimum DAC setpoint given current DAC, volts and amps  
/*
 * return DAC value for desired setpoint in mode
 *  R (CR) mode set amps = volts / desired resistance
 *  P (CP) mode set amps = desired power / volts
 *  V (CV) mode set amps to achieve correct volts (difference between current and desired /volts
 *  
 *  iteration if value is less than C_VARCOARSE (needs to be big enough to capture calculation after first guess)
 * setPoint is V, A, R or P.
 * return DACval
 */
float rEst;	// estimated from open circuit and lightly loaded voltages
float lastSetpoint = 0;
bool cp_first = true;  // first CP limiting with big error

int modeCalc(float readV, float readI, uint8_t c_speed, bool guessFirst, bool printMe)
{
  float setI, curVal, deltaI; 
  int DACval;
  float setPoint;
  int mode = dynSet.mode;

  switch (mode) // all modes except CC
  {
    // have no idea of correct values at switch on as I reading is Zero.
	// at a new setting, we make the assumption that the DUT has a linear V/I characteristic (resistive)
    // give a small kick initially, to get an initial I reading, then go from there.
    case MODE_CC :
      firstGuess = guessFirst;
      break;
    case MODE_CR : 
    case MODE_CP :	
      if(readV < 	RP_DROPOUT_V)
          errorKill(true, true, meas.ADSvolts, "Insufficient voltage\nfor CP/CR mode\control.", true);     
          // CV tests also apply
    case MODE_CV : 
      if(readI < IDROPOUT || guessFirst)    
      {          
          DACval = ampsToDAC(KICK); // provide a small current to get things started
          //if (printMe) Serial.printf(" Kick: not enough current. GF %i [%i]\n", guessFirst, DACval ); 
          firstGuess = true;		   
          return DACval;
      }         
	  // CV mode - I setting has brought V down to Zero  
	  // what about other modes?
    /* -- moved to CV main code
      if (readV < VDROPOUT & mode == MODE_CV) // avoid stupid answers
      {
         DACval = KICK;
#ifdef C_DEBUG   
         if(printMe) Serial.printf("Kick CV: Not enough Voltage to guess %3.2f [%i]\n ", readV, DACval);  
#endif
         firstGuess = true;    
         return DACval;
      }
      */        
      break;
  }

  switch (mode)
  {	   
    case MODE_CC :	
  	  if (dynSet.current * meas.ADSvolts > pMaxOp)
  	  {
  			_isLimiting = true;
  			setPoint = pMaxOp / meas.ADSvolts;	// reduce to stay within SOA
       // if(printMe) Serial.printf("CC SOA limiting %2.3fA\n ", setPoint);
  	  }
  	  else 
  	  {
  			setPoint = dynSet.current;
  			_isLimiting = false;
  	  }
  
      if(setPoint < IDROPOUT) // avoid divide by zero
      {
        DACval = 0;
       //if(printMe) Serial.printf("CC I set value too small %2.3fA [%i]\n ", setPoint, DACval);
        break;
      }
      
      curVal = readI;       
      if(firstGuess) 
      {     
	     DACval = ampsToDAC(setPoint);
         guessFirst = false;  
         //if(printMe) Serial.printf(" CC-FG %3.2fA [%i] ",setPoint, DACval); 
	    }
  	  else // iterate with damped increment
      {	  
        deltaI = setPoint - curVal;
  		  // ampsToDAC is inaccurate at small values, so use HI cal values for deltaI.
  			//float d2 = deltaI * halCal[ADS_I].hiCalCount / halCal[ADS_I].hiCalVal;
        float d2 = ampsToDAC(setPoint) - ampsToDAC(curVal);
  			int dd = C_DAMPING_C * d2;
  			DACval = _lastDAC + dd;     
  			//if(printMe) Serial.printf(" CC~ SP %2.2f Di %2.2f [%i + %i[%3.2f] = %i] %i\n ", setPoint, deltaI, _lastDAC, dd, d2, DACval, ampsToDAC(setPoint)); 
      }              
      break; // End CC
    
    case MODE_CP :   
      curVal = readV * readI; 
      setPoint = dynSet.power;
      //if(printMe) Serial.printf(" CP set %3.3fW, read %3.3fW last %3.3fW ", setPoint, curVal, lastSetpoint);  
      if(setPoint < PDROPOUT) // avoid divide by zero (this should never be triggered)
      {
        DACval = 0;
        //Serial.printf(" P value too small %3.2f [%i] - Disconnecting\n", setPoint, DACval);
        errorKill(true, true, setPoint, "Power set too low\nto control.", true);
        break;
      }
      DACval = limitMe(curVal, setPoint, readV, readI, mode);
      if(_isLimiting)
        break;
     
      if(firstGuess) // initial setpoint assumes constant V. 
      {
           setI =  setPoint / readV;   
           DACval = ampsToDAC(setI);  
           firstGuess = false;
           setPoint = 0.01; // some small non-zero value
          // if(printMe) Serial.printf(" CP. read %3.2fW, set %3.2fA [%i]", curVal, setI, DACval);                 
      }    
      else // iterate
      { 
        int d2, d3;
        if(abs(curVal - dynSet.power) < VSMALL_DIFF) // if close to target
        {          
          setPoint = lastSetpoint; 
          DACval = _lastDAC;
         // Serial.print(".");          
        }
        else // normal CP operation - damped iteration
        {          
           deltaI = (setPoint - curVal) / readV; // in amps
           /*
           // ampsToDAC is inaccurate at small values, so use cal values directly.
           // also makes an assumption of 0 ESR
           float d2 = deltaI * halCal[ADS_I].hiCalCount / halCal[ADS_I].hiCalVal;
               DACval = _lastDAC + C_DAMPING_P * d2;   
            */
           d2 = C_DAMPING_P * (ampsToDAC(setPoint/readV) - ampsToDAC(curVal/readV)); // non-linear so need two conversions
           d3 = (SIGN(deltaI)) * ((abs(deltaI) < 0.1) ? DAC_SMALLJUMP : DAC_BIGJUMP);   
           DACval = _lastDAC + d2;
           DACval = constrain(DACval, 0, ampsToDAC(dynSet.current*DACOVER)); // keep below the current limit.  
           //if(printMe) Serial.printf(" || set %3.3f, deltA %3.3f, SGN %i, DAC [%i + %i [%i] = %i], A2d %i %c ", setPoint, deltaI, SIGN(deltaI) , _lastDAC,d2,d3,DACval, ampsToDAC(dynSet.current),(_isLimiting)? 'L' :' ');        
        }          
      }
       
      lastSetpoint = setPoint;
      break; // End CP
  
    case MODE_CR : // CR mode
      setPoint = dynSet.resistance;
      if(readI > VSMALL_DIFF) // avoid divide by zero error
        curVal = readV/readI;
      else
        curVal = 9999;
     // Serial.printf(" curVal %3.3f set %3.3f", curVal, setPoint);

      DACval = limitMe(curVal, setPoint, readV, readI, mode);
      if(_isLimiting) // exit limiting if current limiting // && curVal > setPoint + MEDIUM_DIFF
        break;
      //_isLimiting = false;
      if(setPoint < RDROPOUT ||  readV / setPoint < IDROPOUT) // avoid divide by zero
      {
        DACval = 0;
       // if(printMe) Serial.printf(" R value too small %3.2f [%i]\n ", setPoint, DACval);
        break;
      }
      curVal = readV / readI; // dynamic resistance
      // if(abs(curVal - setPoint)/setPoint < C_VARCOARSE) // damped increment      
      if(firstGuess)
      {     
          setI = readV / setPoint;  // assume linear DUT (Not true for PSU)
          //guess disabled, just stay with Kick value   
          DACval = KICK; //ampsToDAC(setI);		  
          firstGuess = false; 
         // if(printMe) Serial.printf(" CR guess %3.2f [%i]\n",setI, DACval);
      }     
      else    
      { 
         float d2 = ampsToDAC(readV / setPoint) - ampsToDAC(readV / curVal);
         bool goDown = d2 > 0;
         float d3 = d2 * C_DAMPING_RUP;
         if(goDown) // raise resistance quickly to reduce current. Some overshoot is OK
         {
           if(setPoint > 5) // need more damping at low resistances
             d3 = C_DAMPING_RH * d2;
           else
             d3 = C_DAMPING_RL * d2; // sliding scale?
         }
         deltaI = 0; // not used

         DACval = _lastDAC + d3; 
         // DACval = constrain(DACval, 0, ampsToDAC(dynSet.current*DACOVER)); // keep below the current limit.
        // if(printMe) Serial.printf(" CR~ deltaI %2.3f [%i + %3.2f [%3.2f] = %i]\n", deltaI, _lastDAC, d3, d2, DACval); 
      } 
      break; // End CR

    case MODE_CV  : // serial calculation of rEst to determine deltaI
      setPoint = dynSet.voltage;
      curVal = readV;
     // Serial.print("CV ");
      DACval = limitMe(curVal, setPoint, readV, readI, mode);
      // exit from current limiting when voltage drops below CV point.   
      if(_isLimiting)
      {
        if(readV - MEDIUM_DIFF < dynSet.voltage)
        {
          //Serial.printf("CV-LOFF D %i\n",DACval);
           _isLimiting = false; // resume CV 
        }
        else  // stay with limiting setting
        {
          //Serial.print("CVL ");
          break;
        }
      }
      /*
      if(_isLimiting)
      {
          errorKill(true, true, meas.ADSamps, "Over current in CV mode\n", true);   
          break;
      }
      */
      if(setPoint < VDROPOUT) // avoid divide by zero
      {
        DACval = 0;
        //if(printMe) Serial.printf(" V setpoint too small %3.2f [%i]\n ", setPoint, DACval);
        break;
      }     
     
      if (readV < VDROPOUT) // turn off if no viable DUT voltage
      {
       // if(printMe) Serial.printf(" V reading value too small for control %03.3fV\n ", readV);
  	    DACval = 0;
        break;
      }
  	  
  	  // no sensible first guess for non-linear systems like a PSU?
  	  // estimate only works as a first guess - or when V/I reading gaps are large, better to just use search function?
  	  rEst = abs((readV - _lastVolts)/(readI - _lastAmps)); // estimate DUT resistance from open circuit voltage and KICK or current V/I
      if(firstGuess)
      {   
  	    int deltaI = (_lastVolts - setPoint)/rEst;       // I = E(diff)/R          
        DACval = _lastDAC + ampsToDAC(setI);     
        /*     
        if(printMe) 
        {
           Serial.printf(" CVFG rEst %3.2f I %3.2f [%i], ", rEst, setI, DACval);
           Serial.printf(" openV %3.2f, readV %3.2f, readI %3.2f \n", openV, readV, readI);
        }
        */
        //Serial.printf("CV FG DV %i", DACval);
        firstGuess = false;
      }  
      else // iterate:  find proportional gap between Voltage and desired V      
      {		
    		float delta = readV/setPoint;   // assumes constant R (i.e. I proportional to V)
    		// may need to modify when near solution
    		// assume V : I
    		//int d4 = C_DAMPING_V * (delta - 1)/rEst; 
        // Assumesw linear DUT R 
        float iNew = readI * delta; 
    		int delta1 = (ampsToDAC(iNew) - ampsToDAC(readI)); // voltage too high: more amps    		
       // different damping ratios for low and high currents, only active when Vread > Vset
    		float delta2 =  (float)delta1 * ((delta < C_DAMPING_VD) ? C_DAMPING_VL : C_DAMPING_VH);
        if(delta1 == 0 && delta > C_DAMPING_VD) // at very low currents
          delta2 = 1;
    		DACval = _lastDAC + delta2;  
    		DACval = constrain(DACval, 0, ampsToDAC(dynSet.current * DACOVER)); // ensure we don't go above the current limit.
    		firstGuess = false;
    	//	Serial.printf(" CV~  iNew %2.3f, delta %2.2f[%i], [%i + %2.3f = %i][%1.1f] ",
    		     //     iNew, delta, delta1, _lastDAC, delta2, DACval, (delta < C_DAMPING_VD)? C_DAMPING_VL : C_DAMPING_VH);
      }            
      break; // End CV
      
	  default: // this is an error condition
		  DACval = 0;	 
  }  // end switch   
  _lastVolts = readV; // save the readings for next time (CV mode)
  _lastAmps  = readI;
 // if(DACval > CDAC_OPMAX) Serial.printf("DAC_O: %3.2f %i\n", setPoint, DACval);

  DACval = constrain(DACval, 0, CDAC_OPMAX); // ensure we don't go above the maximum allowed setting.
  return DACval;
}

// current limiting function - common to CP, CR, CV modes
// SOA check done in control()
int limitMe(float curVal, float setPoint, float readV, float readI, int mode)
{
    int DACval = 0;
    bool limTest = false;
    switch (mode)
    {
      case MODE_CV : 
      case MODE_CR :
        limTest = curVal > (setPoint + MEDIUM_DIFF);  // current rises with increasing setPoint
        break;    
  
      case MODE_CP :
        limTest = curVal < (setPoint + MEDIUM_DIFF); // current falls with increasing setPoint
        break;      
    }
     //Serial.print("~");
    if ((readI + MEDIUM_DIFF) > dynSet.current) // too much current - so limit
    {        
      // && limTest  
      //Serial.print("|");   
      if(!_isLimiting && readI > dynSet.current * OVER_GAP) // First guess - only for a significant gap
      {           
        _isLimiting = true;  // indicator light
        DACval = ampsToDAC(dynSet.current * JUMP_GAP); // slightly over limit point (so that we don't exit limiting)
        lastSetpoint = dynSet.current * readV; // Poor assumption that V will stay constant          
        //cp_first = false;
        //Serial.printf(" *First lim* %3.3f ", dynSet.current * JUMP_GAP);             
      }
      else // iterate up/down to limit current
      {
        if(abs(readI - dynSet.current) > SMALL_DIFF) // not at limit current
        {           
           _isLimiting = true;  // indicator light
          // guess at proportion of the gap between last reading and target
          float gap = readI - dynSet.current;   
          float prop = 1 -(1- dynSet.current/readI)/C_DAMPING_LIM;                    
          DACval =  _lastDAC * prop; // change by that damped step
          
          //Serial.printf(" L %s prop %1.3f, LSP %2.3f ", (gap > 0)? "<<" :">>", prop, lastSetpoint); 
        }  
        else // continue to limit at current point unless now below the mode setpoint
        {
          _isLimiting = true;  
          DACval = _lastDAC; 
        //  Serial.print(":");          
        }
      }      
    }
   else
   {
      _isLimiting = false;
      DACval = _lastDAC; 
    }
    return DACval;
}
/*
 switch (mode)
          {

            case MODE_CV : 
              lastSetpoint = readV;
               if(readV > dynSet.voltage + MEDIUM_DIFF)
                _isLimiting = false;
              break;
            case MODE_CR :
              lastSetpoint = readV / dynSet.current; 
              if((readI > SMALL_DIFF) && (readV/readI < dynSet.resistance + MEDIUM_DIFF)) // avoid divide by zero
                _isLimiting = false;
              break;        
            case MODE_CP :
              lastSetpoint = dynSet.current * readV;
              if(readV * readI < dynSet.power + MEDIUM_DIFF )
                _isLimiting = false;
              break;      
            case MODE_CC : // not sure we need this one
              lastSetpoint = dynSet.current; 
              break; 
          }
 */
volatile int debugSlow = 0;
#define SLOWDOWN 200
// control task - suspends after each cycle, waiting for ADS interrupt to resume
// calls actual control code after checking ADS/ESP ADC inputs and state
// If both ADS readings are done [adsSetready], then control(ADS) if there's a settings-readings gap 
// In between, control(ESP) only if ESP/ADS ADC readings differ significantly
void controlTask(void *pvParameters)
{
  long tim;
  while(1) // infinite
  {
    tim = micros();
    adsProcess(); 
#ifdef C_DEBUG
    if (debugSlow > SLOWDOWN) 
      debugSlow = 0;
    debugSlow++;
#endif
    //Serial.printf("C" );
    
    // fast response to negative input voltage - will stall process for 
    if(meas.ADSvolts < RADSNEG_V)
    {
      errorKill(true, true, meas.ADSvolts, "Reverse voltage\ndetected", true);     
    }    
    if(adsSetready)
      averageVolts();
    if (_outOn) // only control if output is On
    {
      if(adsSetready) // slow/fine control (ADS)
      {
        adsCount++;        
        adsSetready = false;
        //
#ifdef C_DEBUG // slow down control for debug
        if(debugSlow == SLOWDOWN)
        {
          //Serial.printf("\n ADS Gap %2.3f ", settingsGap(ADC_ADS));
#endif      
          // running average for Battery test
         // if(pSet.modeB == MODE_BAT)
         
          float sg  = settingsGap(ADC_ADS); 
          float srg = settingsRatioGap(ADC_ADS);
          bool ilim = (meas.ADSamps - dynSet.current - MEDIUM_DIFF) > 0;
          if (abs(sg) > C_VARFINE || ilim)// && abs(srg) > C_VARFINER)   // only control if required
          {        
            _DACis = control(ADS_CONTROL); // use ADS, fine tune
            //Serial.print("< ");  
          }        
#ifdef C_DEBUG 
         //Serial.printf("> %1.3f, %1.3f\n", sg, srg); //Serial.printf("[%c %1.2f]", (settingsGap(ADC_ADS) > 0) ? '>' : '<');
        }
#endif
      } 
      else // betweeen samples coarse control (ESP readings)
      {  
#ifdef C_DEBUG 
if(debugSlow == SLOWDOWN)
        {
#endif         
          //int  cCount = 0;
          if(getESPviReadings()) 
          { 
            float sg  = settingsGap(ADC_ESP);         
            float srg = settingsRatioGap(ADC_ESP);
            bool ilim = (meas.ESPamps - dynSet.current - BIG_DIFF) > 0; // substantially over current limit point           
            // don't try control if readings are out of range
            if (ilim || (!_isLimiting  && abs(sg) > C_VARCOARSE && abs(srg) > C_VARCOARSE_RAT))  // need both tests to cover small and large currents
            {                
              //Serial.printf("ESP Control ilim %i, sg %2.3f, srg %2.3f\n",ilim,sg, srg);
              //Serial.printf("[%c %1.3f, %1.3f: A %1.3f, E %1.3f]\n", (sg > 0) ? '>' : '<', sg ,srg,  meas.ADSamps, meas.ESPamps);
              _DACis = control(ESP_CONTROL); // use ESP, coarse only
              //cCount++;
            }
          }
#ifdef C_DEBUG 
        }
#endif  
      } //ESP
   } // _outOn
   else // output is off
     _DACis = dacSet(DAC_MINCOND_C);
   
   long td = micros() - tim;
   controlTime += (td);
   loops++;
   //if(td > 2100) Serial.print("*");
#ifdef C_DEBUG 
   testPin != testPin;
   //digitalWrite(TESTPIN, (testPin) ? HIGH : LOW);
#endif   
   adsReadStart(); // start new reading. On completion interrupt will re-start this process
   vTaskSuspend(NULL); // suspend forever until another ADS reading comes along (watchdog in loop())
  } // loop
   vTaskDelete(NULL); // no "break" in loop, so never gets here
}

uint8_t _DACaddr;
void control_setup(void)
{	
  _outOn = false;  // ensure output is off 
  _cur_vMode = 'M'; // always read voltage from Main when OFF
  pinMode(KELPIN, OUTPUT);
  digitalWrite(KELPIN, LOW); // start in MAIN mode
  
  pinMode(SW_ON, OUTPUT);
  digitalWrite(SW_ON, LOW); // OFF to start, when _outOn is true: in Output mode to drive LED. false (Off): Input mode to read switch.
  // Brief pulse to ensure output relay is OFF
  pinMode(SW_OFF, OUTPUT);
  digitalWrite(SW_OFF, HIGH);
  delay(50);
  digitalWrite(SW_OFF, LOW);
  pinMode(SW_OFF, INPUT_PULLDOWN); // always in Input mode after this 

  // **********  DAC***********
  _DACaddr = probeDAC(DAC_BASE, DAC_SCAN);	// maybe anywhere in 0x60-0x67
  dac.begin(_DACaddr);
  dac.setVoltage(0, true);	// set DAC startup (EEPROM) and current value to 0
  delay(100); 				// EEPROM write takes some (undefined) time [could poll the status register]

  // ****************** ADC ***********
  // start ADS1115 ADC
  pinMode(ADSRDY, INPUT_PULLUP);
  ads.setGain(ADS_GAIN);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads.begin(I2C_SPEED);  // I2C speed, 1000000 (1M = FMPlus) is probably OK, 400000 is safe. Defaults to 128SPS.
  ads.setRate(7);	// 860SPS
 // Serial.println("ADS BEGIN done");
  //  delay(100);
  
  // esp ADC
  analogReadResolution(12);
  //analogSetSamples(3);
  pinMode(ADC_BV, INPUT); // ext pullups
  pinMode(ADC_BI, INPUT);
  pinMode(ADC_BT, INPUT);
  
/********** ENCODER and Tac Switches *************/
#ifdef ALTENCODER
  enc.attachSingleEdge(ENC_B, ENC_A);
  enc.setFilter(1023);
  //enc.attachHalfQuad(ENC_A, ENC_B);
  //enc.attachFullQuad(ENC_A, ENC_B);
  enc.clearCount();
#else
  enc.setChangedHandler(rotate);
#endif

  butL.setPressedHandler(clickedL);
  butR.setPressedHandler(clickedR);
  
  daughterSense();

  // configure  PWM functionalitites
  ledcSetup(PWMChannel, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN, PWMChannel);  
  setFan(FAN_MIN_PWM);	// start the fan on idle  
  
  // set the mode variables  
  setMode(pSet.mode); // set initial mode indicator (from EE)
  setModeB(MODE_NOR);
  setNORb(true);	// normal mode B
  //dynSet = pSet; // done in setMode and setModeB

   _cur_vMode = pSet.vMode;
   if(pSet.vMode == 'A')
     _cur_vMode = 'K'; // Kelvin preference in Auto

  _lastLogged = millis();
}

// sense daughter board
bool daughterSense(void)
{
   pinMode(DPRESENT, INPUT_PULLUP); // no ext pullup, pulled down for daughter present
   delay(5);  
   bool dFound =  (digitalRead(DPRESENT) == LOW);  
   if(dFound) // found daughter board
   {
	   Serial.printf("Found daughter board\n");
   	 bool alreadyFound = myID.daughter_present;
	   myID.daughter_present = true; // EEPROM value
	   daughter_mul = 2;
	   iMaxOp = IMAXOP * 2;		// #define values are for no daughter.
	   pMaxOp = PMAXOP * 2;
	   // current set values

	   if (!alreadyFound) // daughter wasn't there, now is
	   {			   
		   // trust existing calibration settings: more current for the same counts
       // 1.333 current multiplier due to voltage divider using resistances on the outputs of the INA current sense amps.
		   halCal[DAC].lowCalVal = halCal[DAC].lowCalVal * 1.333;		// DAC 
		   halCal[DAC].midCalVal = halCal[DAC].midCalVal * 1.333;		// DAC 
		   halCal[DAC].hiCalVal  = halCal[DAC].hiCalVal  * 1.333;		// DAC 
		   // lowCalval will be automatically set for ADS_I and ESP_I
		   // midCalVal is not used
		   halCal[ADS_I].hiCalVal = halCal[ADS_I].hiCalVal * 1.33;		// ADS I
       halCal[ADS_I].maxErr = SHORTCIRCUIT * 2;          
		   halCal[ESP_I].hiCalVal = halCal[ESP_I].hiCalVal * 1.33;		// ESP I
       halCal[ESP_I].maxErr = SHORTCIRCUIT * 2;   
		   valChanged(VAL_CHGD); 							// update values to EEPROM
			 Serial.print("New daughter board found - recalibrate current.");
		   errorKill(false, false, 1, "New daughter board found.\nRecalibrate current.", false);
	   }
   } else // daughter may have been removed
   {    
	   Serial.printf("No daughter board\n");
	   bool alreadyFound = myID.daughter_present; // EEPROM value
	   daughter_mul = 1;
	   iMaxOp = IMAXOP;
	   pMaxOp = PMAXOP;
	   myID.daughter_present = false;
	   // daughter_mul, iMaxOp, setVals are default values
	   if (alreadyFound) // daughter was there, now not
	   {			   
		   // calibration settings - back to defaults
		   halCal[DAC].lowCalVal = RDACLO_I;			// DAC - hard-coded value was without daughter
		   halCal[DAC].midCalVal = RDACMID_I;			// DAC - cal point real val 
		   halCal[DAC].hiCalVal  = RDACHI_I;				// DAC 
		   
		   halCal[ADS_I].hiCalVal = RADSHI_I;			// ADS I
		   halCal[ESP_I].hiCalVal = RESPHI_I;			// ESP I
		   valChanged(VAL_CHGD); 						// update values to EEPROM
       Serial.print("Previous daughter board missing - recalibrate current.");
		   errorKill(false, false, 0, "Previous daughter\nboard missing.\nRecalibrate current.", false);
	   }
   }
   // update current values and limits into screen definitions
   setVals[SV_I].maxVal = iMaxOp;	// current
   setVals[SV_P].maxVal = pMaxOp;	// power

   //readVals[RV_I].maxVal = iMaxOp;
   setVals[SV_S1A].maxVal = iMaxOp; //S1.A
   setVals[SV_S2A].maxVal = iMaxOp; //S2.A
   
   // halCal current limits - either daughter or not
   halCal[ADS_I].maxVal = halCal[ESP_I].maxVal = iMaxOp;				
   halCal[ADS_I].maxErr = halCal[ESP_I].maxErr = iMaxOp + OVERCURRLIM;  
  
   return dFound;
}

//used by SCPI as well as panel buttons
//sets Onoff and kills any running mode
// status false = OFF
void onOffKill(int8_t channel, bool isOn)
{
	onOff(channel, isOn); // hardware on/off
			
	if(!isOn) // stop any  running BAT or STEP mode operations
	{
		setModeB(MODE_NOR); 
		_curState = BMODE_OFF;  
		dacSet(0);
		dynSet = pSet;	// go back to standard settings
	}	
}

void onOff(int8_t channel, bool isOn)
{
	//Serial.printf("Changing On/Off [ch %i] to %i [pin %i]\n", i, isOn, chanPins[i].onPin);
	// physical control 
	if(isOn) 		// turn ON 
	{
		dacSet(0);  // ramp up from zero
		dynSet.current = 0;
		delay(50); // slow startup
		digitalWrite(SW_ON, HIGH);  // GPIO is always in OUTPUT mode, except when being explicitly read
		_logHead = _logCount = 0;	// clear and restart the log
		_lastLogged = _tLogStarted = millis(); 
    firstGuess = true; // control first guess (modeCalc() )
		dynSet = pSet;
    _avg_ready = false; // re-starting the average
    
    // read Main and Kelvin voltages - decide which to use
    digitalWrite(KELPIN, LOW); // should already be in MAIN mode
    float mVolts, kVolts;
    vTaskDelay(100); 
    mVolts = meas.ADSvoltsAvg;
    printADSavg();
    digitalWrite(KELPIN, HIGH); //
    vTaskDelay(150); // includes time for relay
    printADSavg();
    kVolts = meas.ADSvoltsAvg;
     _cur_vMode = 'M'; 
    if ((pSet.vMode == 'A' || pSet.vMode == 'K') && (kVolts + MIDDLE_DIFF > mVolts) && (kVolts > KMINV))
      _cur_vMode = 'K';
    else
      digitalWrite(KELPIN, LOW);
    if((pSet.vMode == 'K') && (_cur_vMode != 'K'))
    {
      screenError("Kelvin inputs\nnot connected?\nReverting to Main", ERR_BG_A ,5, false); // Kelvin inputs not connedted in K mode
      //Serial.println("Kmode - no input");
    }
    //Serial.printf("On: vM %2.3f, vK %2.3f %c %c\n", mVolts, kVolts, pSet.vMode,_cur_vMode);
    currentMenu = 0;  // back to main screen when on
    dirtyScreen = true;
	}
	else 			// turn OFF
	{ 
		dacSet(DAC_MINCOND_C); // set control voltage to nearly OFF	(sustain voltage measurement)
		dynSet.current = 0;
    _isLimiting = false; // clear the marker
		// pulse Off pin - may be a SCPI command		
		digitalWrite(SW_ON, LOW); // on pin LOW		
		pinMode(SW_OFF, OUTPUT); 			
		digitalWrite(SW_OFF, HIGH); 
		delay(1);
		digitalWrite(SW_OFF, LOW);
		pinMode(SW_OFF, INPUT_PULLDOWN);  // reset pin for panel control
    // go back to MAIN voltage measurement
		if(_curState == BMODE_RUN) setModeB(MODE_NOR); // cancel STEP or BAT
    // set up for turning back on.
    lastSet.power = 0;
    lastSet.voltage = 0;
    lastSet.current = 0;
    lastSet.resistance = 9999;
    _cur_vMode = 'M'; // always read voltage from Main when OFF
	}	
    _outOn  = isOn;
}

// control of GPIO is in onOff()
int vAuto(int)
{
  onOff(0,false); // can't change modes when ON
  pSet.vMode = 'A';  
  _cur_vMode = 'K'; // Kelvin preference in Auto
  dirtyScreen = true;
  return 0;
}
int vMain(int)
{
   onOff(0,false);
   _cur_vMode = pSet.vMode = 'M';
   dirtyScreen = true;
   return 0;
}
int vKel(int)
{
   onOff(0,false);
   _cur_vMode = pSet.vMode = 'K';  
   dirtyScreen = true;
   return 0;
}

/*
// display counts and converted values
void dispConv(void)
{

	Serial.printf("Vin  [%i] = %5.2f [%i counts = %5.2fV]\n", VIN,    myADC[VIN].curVal,  adsReadings[VIN],  myADC[VIN].curVolts );
	Serial.printf("Vout [%i] = %5.2f [%i counts = %5.2fV]\n", VOUT,   myADC[VOUT].curVal, adsReadings[VOUT], myADC[VOUT].curVolts );
	Serial.printf("Iout [%i] = %5.2f [%i counts = %5.2fV]\n", IOUT,   myADC[IOUT].curVal, adsReadings[IOUT], myADC[IOUT].curVolts );
	Serial.printf("Temp [%i] = %5.2f [%i counts = %5.2fV]\n\n", TEMP, myADC[TEMP].curVal, adsReadings[TEMP], myADC[TEMP].curVolts );

}
  */
// use non-blocking mode for calls from Control() functions or other code as needed
void errorKill(bool shutOff, bool DACzero, float reading, const char errMsg[], bool nonBlocking)
{
	//char msgBuf[128];
	if (shutOff)
	{		
		onOffKill(-1, false);    // shut off output		
	}
	if(DACzero)
	{
		//shutDown = true;  //change setpoints to safe values		
		dacSet(0);
		dynSet.current = 0;
	}
	// display screen error message, indicator, and store for SCPI 
	sprintf(scrE.message,"** Error **\n%s\n[%5.2f]\n", errMsg, reading);
  if(nonBlocking)
  {
    newScrError = true;
    scrE.bgcol = ERR_BG_B;
    scrE.onTime = -1; // wait until touched
    scrE.logo = false;  
    Serial.printf("Non blocking kill, %s\n",   scrE.message);
  }
  else
	  screenError(scrE.message, ERR_BG_B, -1, false);
}


int fanControl(void)
{
	float ff;
	int fs;
	if(meas.ESPtemp > TMAXOP & _outOn)	// overtemp first detection
		onOffKill(-1, false);
		
	if(meas.ESPtemp < TEMP_FANSTART)
	{
		return setFan(FAN_MIN_PWM);		
	}
	else
	{
		ff = fmap(meas.ESPtemp, TEMP_FANSTART, TEMP_FANFULL, FAN_MIN_PWM, 100);	
		return setFan(ff);
	}
}

#endif
