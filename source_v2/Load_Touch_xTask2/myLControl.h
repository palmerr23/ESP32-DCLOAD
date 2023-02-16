#ifndef MYLCONTROL_H
#define MYLCONTROL_H
/*
Single instrument control for Load
*/
int dxxx =0;

void onOff(int8_t channel, bool status);
void onOffKill(int8_t channel, bool status);
short SCPIsendGrpMessage(uint8_t tgrp, char *buf);
uint8_t setMode(uint8_t aMode);
uint8_t setModeB(uint8_t bMode);
void errorI(bool shutOff, bool DACzero, float reading, char errMsg[]);
int modeCalc(float readV, float readI, uint8_t c_speed, bool guessFirst, bool printMe);
bool daughterSense(void);
bool getESPviReadings(void);
bool addLog(bool mustLog);
void addLogItem(float setting, char mode[], char func[], bool tsec);
void restartLog(void);
//int dacRead(void);

bool changedLocal = true; // last value changed was local (or remote)
//bool shutDown = false;
bool firstGuess = true; // deal with non-linear cases where first guess is outside the C_VARCOARSE window
float openV;

// coarse and fine control for main output with limiting and tracking
char estop[] = ":TRAC:ESTO";
int cVal = 0;
// must be non-blocking

settings t; // used by BAT and STEP modes. Needs to be persistent, as most times routines just exit.

// battery test
// set and hold appropriate discharge setting
// every 5 minutes (and at start) disconnect for several seconds and measure open circuit voltage.
//   calculate ESR and display - need to stop normal resistance calculation
// monitor for final voltage, when reached, measure ESR again.
// go to NOR/Off mode.
// display results - errorI: mod to hold until screen touched.
long tLastBatUpdate, tBatStart;
float batAH, batMAH; // amp hrs, running total
#define BAT_EVERY 20 // seconds
#define BAT_WAIT_MEASURE 1 // seconds
float batOCvolts = 999, batCCvolts = 999, batCurrent = 999;
bool batUpdateMode;
float esr;
bool preStart = false;
#define MS_TO_HRS (3600 * 1000)
// returns true while running
bool batTest(void)
{
  // 2 step start: 
  // step 1: turn On and set current to zero
  if (_curState == BMODE_SEL) // start mode
  {	 
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
  if (preStart && ((millis() - tLastBatUpdate) > BAT_WAIT_MEASURE * 1000))  
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
  if(millis() - tLastBatUpdate > (BAT_EVERY + BAT_WAIT_MEASURE) * 1000 && batUpdateMode)
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
      onOffKill(0, false);
      _curState = BMODE_OFF;    
      batAH += batCurrent * (millis() - tLastBatUpdate)/60000; // Amp Hours
      float batMAH = batAH * 1000;
	  delay(BAT_WAIT_MEASURE * 1000);
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
     Serial.printf("Starting STEP: %i cycle %i, set %3.2f\n",_curStage, _curCycle, t.current); 
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
      Serial.printf("*** Done STEP\n");
   }
  // if(t.current > 1.6) Serial.printf("Step_OC: slw %i, stp %i, sta %i, t.c %2.3fA, iT %2.3fA, iN %2.3fA\n", _slewing, _curStep, _curStage, t.current, iThisStep, iLastStep);
   return t;
}
bool SOAcheck(float vRead, float iRead)
{
/*	
	float setPoint;
	// don;t check for max current - can't set too high, error if overcurrent caught in limitCheck()
	// Limit current if Power > max
	if(dynSet.current * vRead > pMaxOp)
	{
		isLimiting = true;
		dynSet.current = pMaxOp / vRead;
	}
	else
		isLimiting = false;
*/	
	return isLimiting;
	
	/*
	// only exit SOA limiting if somewhat below Max Power (hysteresis)
	if ((isLimiting && (vRead * iRead < (pMaxOp - MAX_HYST))) || (!isLimiting && (vRead * iRead < pMaxOp)))
	{		
		isLimiting = false;		
		return false;
	}
	isLimiting = true;
	*/
	//errorI(true, true, vRead * iRead, "Maximum power exceeded");
	//Serial.print("Exceeded SOA\n");

	//return true;		
}
bool limitCheck(float vRead, float iRead)
{
	// errorI turns off output
	if (vRead > halCal[ADS_V].maxErr)
	{
		errorI(true, true, vRead, "Maximum voltage exceeded");
		return true;
	}
	if (iRead > halCal[ADS_I].maxErr)
	{		
		errorI(true, true, iRead, "Maximum current exceeded");
		return true;
	}
	return false;
}

// returns setting error (set - val)
float settingsGap(int adcType)
{
	float iRead, vRead;
	if(isLimiting)
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
		//Serial.printf(" <SG-P %2.3f = %2.3f * %2.3f> ", dynSet.power, vRead, iRead);
			return dynSet.power - vRead * iRead;
			
		case MODE_CV : // CV mode
			return dynSet.voltage - vRead;		
	}
	return -1.0; // we don't get here
}
// returns setting error (set - val)/set
float settingsRatioGap(int adcType)
{
	float iRead, vRead;
	if(isLimiting) 
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
			if(dynSet.resistance < 1)
				return 0;
			else
				return 1.0 - (vRead/iRead)/dynSet.resistance;
		
		case MODE_CP : 
		//Serial.printf(" <SRG-P %2.3f = %2.3f * %2.3f> ", dynSet.power, vRead, iRead);
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
float iLast, vLast;
// slow (ADS readings, fine tune) - once through only
// or fast (ESP readings, coarse only) - up to COARSE_ITER iterations 
int control(uint8_t c_speed)
{
	float vSetpoint, iSetpoint, iTrack;
	float vRead, vReadS, iRead, tRead, vGap;
	float iGap, iGap2;	
	int jump, dacVal;
	bool rLimit;
	short fjump;
	bool guessFirst = false;	// *************need to fix this ************	

	if (!_outOn) // don't control if output is OFF
	{
	  // Serial.print("!");
	  return dacSet(0); // shouldn't get here - controlTask() should catch this.
	}
	
	// fast response to negative input voltage
	if(meas.ADSvolts < RADSNEG_V)
	{
	  onOffKill(0, false);
      errorI(true, true, meas.ADSvolts, "Reverse voltage\ndetected");     
	}
		
	if (c_speed == ESP_CONTROL)
	{		 
	  vRead  = meas.ESPvolts;   
	  iRead  = meas.ESPamps;
	  limitCheck(vRead, iRead);	  	// overvoltage / short circuit: error - disconnect
	 // SOAcheck(vRead, iRead);  		// reached SOA limits: reduce setting in dynset 
	  // readings too small for ESP control. CC mode independent of vRead.
	  // use ADS for low values - ESP readings unreliable.
	  if (meas.ADSamps < RESPMIN_I || ((dynSet.mode != MODE_CC) && meas.ADSvolts < RESPMIN_V) )
	  {
#ifdef C_DEBUG   
	if(_printMe) Serial.printf("Low FAST ESP counts: A %i C, V %i C\n", ADCcount.ESPa, ADCcount.ESPv);
#endif
		  return _lastDAC;	// can't FAST control when ESP readings are invalid, wait for ADS reading
	  }
	  // is gap enough to warrant a control cycle?
	  // always control if limiting
	  iGap = settingsGap(ADC_ESP);
	  iGap2 = settingsRatioGap(ADC_ESP);
	  if(abs(iGap) < C_VARCOARSE && abs(iGap2) < C_VARCOARSER)
	  {
#ifdef C_DEBUG   
	if(_printMe) Serial.printf("FAST not required, gap is : %2.3f\n", iGap, iGap2);
#endif
		  return _lastDAC; // no need for coarse control
	  }
	}
	else // ADS FINE control 
	{
		// should we try to control below RADSMIN_I?
//if (batUpdateMode && (dxxx % 20) == 0) Serial.print("^");
	  vRead  = meas.ADSvolts;   
	  iRead  = meas.ADSamps;
	  limitCheck(vRead, iRead);	  // overvoltage / short circuit
	  //SOAcheck(vRead, iRead);  // reached limits: error - disconnect???
	  iGap = settingsGap(ADC_ADS);
	  iGap2 = settingsRatioGap(ADC_ADS);
	  
	  // gap too small to iterate again
	  // test duplicated in main line
	  if((abs(iGap) < C_VARFINE || abs(iGap2) < C_VARFINER) && !batUpdateMode) // load is ON with zero current when reading battery OCV
	  {
#ifdef C_DEBUG   
	if(_printMe) Serial.printf("SLOW not required, gap is  %2.3f : %2.3f\n", iGap, iGap2);
#endif
		return _lastDAC;
	  }

	  if (vRead < RADSMIN_V && dynSet.mode != MODE_CV)
	  {
#ifdef C_DEBUG   
	if(_printMe) Serial.printf("Low SLOW ADS volts: %2.3fV [%i]\n", meas.ADSvolts, ADCcount.ADSv);
#endif		  
		 // return _lastDAC;	// can't control when readings are invalid, wait for ADS reading, slow creep up of current
	  }
	}
	float dv;
//if (batUpdateMode && (dxxx % 20) == 0) Serial.print("~");
#ifdef C_DEBUG   
   if(_printMe) Serial.printf("\nControl %c iRread %5.3f", (c_speed == ADS_CONTROL) ? 'A' : 'E', iRead);
   if(_printMe) Serial.printf(", vRread %5.3f, MC {\n", vRead);
#endif
//if (batUpdateMode && (dxxx++ % 20) == 0) Serial.printf("! %3.2f",dynSet.current);
	dacVal = modeCalc(vRead, iRead, c_speed, guessFirst, _printMe);
	dv = dacSet(dacVal); 
#ifdef C_DEBUG   
	if(_printMe) Serial.printf("\n} dacVal set %i ", dacVal);
	if(_printMe) Serial.printf(", Gaps %2.3f %2.3f \n", iGap, iGap2);
#endif
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
float rDUT;	// estimated from open circuit and lightly loaded voltages

int modeCalc(float readV, float readI, uint8_t c_speed, bool guessFirst, bool printMe)
{
  float setI, curVal, delta; 
  int DACval;
  float setPoint;
  int mode = dynSet.mode;

  switch (mode) // all modes except CC
  {
    // have no idea of correct values at switch on as I reading is Zero.
	// at a new setting, we make the assumption that the DUT has a linear V/I characteristic (resistive)
    // give a small kick initially, to get an initial I reading, then go from there.
    case MODE_CR : 
    case MODE_CP :		
    case MODE_CV : 
      if(readI < IDROPOUT)    
      {          
           DACval = KICK; // provide a small current to get things started
           if (printMe) Serial.printf(" Kick: not enough current. GF %i [%i]\n", guessFirst, DACval ); 
           firstGuess = true;		   
           return DACval;
      }         
	  // CV mode - I setting has brought V down to Zero  
	  // what about other modes?
      if (readV < VDROPOUT & mode == MODE_CV) // avoid stupid answers
      {
         DACval = KICK;
#ifdef C_DEBUG   
         if(printMe) Serial.printf("Kick: Not enough Voltage to guess %3.2f [%i]\n ", readV, DACval);  
#endif
         firstGuess = true;    
         return DACval;
      }        
      break;
    default:	// CC
      break;     
  }
  switch (mode)
  {	   
    case MODE_CC :	
	  if (dynSet.current * meas.ADSvolts > pMaxOp)
	  {
			isLimiting = true;
			setPoint = pMaxOp / meas.ADSvolts;	// reduce to stay within SOA
	  }
	  else 
	  {
			setPoint = dynSet.current;
			isLimiting = false;
	  }

      if(setPoint < IDROPOUT) // avoid divide by zero
      {
        DACval = 0;
        if(printMe) Serial.printf("I set value too small %2.3f [%i]\n ", setPoint, DACval);
        break;
      }
      curVal = readI; 
      if(firstGuess) //abs(curVal - setPoint)/setPoint < C_VARCOARSE) // damped increment
      {     
	     DACval = ampsToDAC(setPoint);
         firstGuess = false;
         if(printMe) Serial.printf(" CC. %3.2f [%i] ",setPoint, DACval); 
	  }
	  else // iterate
    {	  
       delta = setPoint - curVal;
		  // ampsToDAC is inaccurate at small values, so use HI cal values for delta.
			float d2 = delta * CDACHI_I / RDACHI_I;
			int dd = C_DAMPING * d2;
			DACval = _lastDAC + dd;     
			if(printMe) Serial.printf(" CC~ SP %2.2f DE %2.2f [%i + %i = %i]\n ", setPoint, delta, _lastDAC, dd, DACval); 
    }              
    break; // End CC
	  
    case MODE_CR : // CR mode
	  if (readV * readV / dynSet.resistance  > pMaxOp)
	  {
			isLimiting = true;
			setPoint = (readV * readV) / pMaxOp;	// increase to stay within SOA
	  }
	  else
	  {
			isLimiting = false;
	    setPoint = dynSet.resistance;
	  }
	
      if(setPoint < RDROPOUT ||  readV / setPoint < IDROPOUT) // avoid divide by zero
      {
        DACval = 0;
        if(printMe) Serial.printf(" R value too small %3.2f [%i]\n ", setPoint, DACval);
        break;
      }
      curVal = readV / readI; // dynamic resistance
      // if(abs(curVal - setPoint)/setPoint < C_VARCOARSE) // damped increment      
      if(firstGuess)
      {
          setI = readV / setPoint;  // assume linear DUT (Not true for PSU)
          DACval = ampsToDAC(setI);		  
          firstGuess = false; 
          if(printMe) Serial.printf(" CR. %3.2f [%i]\n",setI, DACval);
      }     
      else    
      { 
         delta = (readV / setPoint) - (readV / curVal)  ; // delta I
		 // ampsToDAC is inaccurate at small values, so use cal values directly.
		 float d2 = delta * CDACHI_I / RDACHI_I;
         DACval = _lastDAC + C_DAMPING * d2; 
         DACval = constrain(DACval, 0, ampsToDAC(dynSet.current)); // keep below the current limit.
         if(printMe) Serial.printf(" CR~ %2.2f [%i + %i = %i]\n", delta, _lastDAC ,d2, DACval); 
      } 
      break; // End CR
      
    case MODE_CP :	  
	  if (dynSet.power > pMaxOp) // shouldn't happen
	  {
			isLimiting = true;
			setPoint =  pMaxOp;	// reduce to stay within SOA
	  }
	  else 
	  {
		isLimiting = false;	
	    setPoint = dynSet.power;
	  }
      if(setPoint < PDROPOUT) // avoid divide by zero
      {
        DACval = 0;
        if(printMe) Serial.printf(" P value too small %3.2f [%i]\n ", setPoint, DACval);
        break;
      }
	  if(c_speed == ADC_ADS)
	  {  
			if(readV < RADSMIN_V) // too much current, can't estimate accurately at low voltage
			{
				DACval = _lastDAC - DAC_BIGJUMP;
				break;
			}
	  }
	  else // esp
	  {
			if(readV < RESPMIN_V) // active control here causes instability
			{
				DACval = _lastDAC; 
				break;
			}
	  }
      curVal = readV * readI; 

     if(firstGuess)
     {
           setI =  setPoint / readV;   
           DACval = ampsToDAC(setI);  
           firstGuess = false;
           if(printMe) Serial.printf(" CP. read %3.2fW, set %3.2fA [%i]", curVal, setI, DACval);                 
      }    
      else // iterate
      {	
         delta = (setPoint - curVal) / readV; // in amps
		 // ampsToDAC is inaccurate at small values, so use cal values directly.
		 // also makes an assumption of 0 ESR
		 /*
		 float d2 = delta * CDACHI_I / RDACHI_I;
         DACval = _lastDAC + C_DAMPING2 * d2;   
		  */
		 int d2 = C_DAMPING * (ampsToDAC(setPoint/readV) - ampsToDAC(curVal/readV)); // non-linear so need two conversions
		 int d3 = (SIGN(delta)) * ((abs(delta) < 0.1) ? DAC_SMALLJUMP : DAC_BIGJUMP);		
		 DACval = _lastDAC + d2;
		 DACval = constrain(DACval, 0, ampsToDAC(dynSet.current)); // keep below the current limit.
         if(printMe) Serial.printf(" CP~ read %3.2fW, delta %3.2f, SGN %i, set [%i + %i [%i] = %i] ", curVal, delta, SIGN(delta) , _lastDAC , d2, d3,DACval);   
      } 
	  
      break; // End CP

    case MODE_CV  : // serial calculation of rDUT to determine deltaI
	  if (dynSet.voltage * readI > pMaxOp)
	  {
			isLimiting = true;	  
			setPoint =  pMaxOp / readI ;	// reduce to stay within SOA
	  }
	  else
	  {
			isLimiting = false;
			setPoint = dynSet.voltage;	  
	  }
      if(setPoint < VDROPOUT) // avoid divide by zero
      {
        DACval = 0;
        if(printMe) Serial.printf(" V value too small %3.2f [%i]\n ", setPoint, DACval);
        break;
      }     
     
      if (readV < VDROPOUT) // turn off if no viable DUT voltage
		  return 0;
	  
	  // no sensible first guess for non-linear systems like a PSU?
	  // estimate only works as a first guess - or when V/I reading gaps are large, better to just use search function?
	  rDUT = abs((readV - _lastVolts)/(readI - _lastAmps)); // estimate DUT resistance from open circuit voltage and KICK or current V/I
      if(firstGuess)
      {   
		int deltaI = (_lastVolts - setPoint)/rDUT;       // I = E(diff)/R          
        DACval = _lastDAC + ampsToDAC(setI);          
        if(printMe) 
        {
           Serial.printf(" . rDUT %3.2f I %3.2f [%i], ", rDUT, setI, DACval);
           Serial.printf(" openV %3.2f, readV %3.2f, readI %3.2f \n", openV, readV, readI);
        }
        firstGuess = false;
      }  
      else // iterate:  find proportional gap between Voltage and desired V      
      {		
		delta = readV/setPoint;  
		// may need to modify when near solution
		// assume V : I
		//int d4 = C_DAMPING3 * (delta - 1)/rDUT; 
		float iNew = readI * delta;
		int delta4 = (ampsToDAC(iNew) - ampsToDAC(readI));
		int delta3 = 1.0 * (float)delta4;
		DACval = _lastDAC + delta3;  // voltage too high: more amps: DAC++
		DACval = constrain(DACval, 0, ampsToDAC(dynSet.current)); // ensure we don't go above the current limit.
		firstGuess = false;
		if(printMe) Serial.printf(" CV~  iNew %2.3f, delta %2.2f[%i], [%i + %i = %i] ", iNew, delta, delta4, _lastDAC, delta3, DACval);
      }            
      break; // End CV
	  default: // this is an error condition
		DACval = 0;	 
  }     
  _lastVolts = readV; // save the readings for next time (CV mode)
  _lastAmps  = readI;
 // if(DACval > CDAC_OPMAX) Serial.printf("DAC_O: %3.2f %i\n", setPoint, DACval);

  DACval = constrain(DACval, 0, CDAC_OPMAX); // ensure we don't go above the maximum allowed setting.
  return DACval;
}

uint8_t _DACaddr;
void control_setup(void)
{	
  _outOn = false;  // ensure output is off 
  pinMode(SW_ON, OUTPUT);
  digitalWrite(SW_ON, LOW); // OFF to start, when _outOn is true: in Output mode to drive LED. false (Off): Input mode to read switch.
  // Brief pulse to reset output relay (OFF)
  pinMode(SW_OFF, OUTPUT);
  digitalWrite(SW_OFF, HIGH);
  delay(50);
  digitalWrite(SW_OFF, LOW);
  pinMode(SW_OFF, INPUT_PULLDOWN); // always in Input mode after this 

  //float volts, realWorld;
  
  // **********  DAC***********
 // MCP45begin(WSTART, DIGI_V); 
  _DACaddr = probeDAC(DAC_BASE, DAC_SCAN);	// maybe anywhere in 0x60-0x67
  dac.begin(_DACaddr);
  dac.setVoltage(0, true);	// set DAC startup (EEPROM) and current value to 0
  delay(100); 				// EEPROM write takes some (undefined) time [could poll the status register]

 
  // watchdog?
  // wTimerBegin(100); // mS

  
  // ****************** ADC ***********
  // start ADS1115 ADC
  pinMode(ADSRDY, INPUT_PULLUP);
  ads.setGain(ADS_GAIN);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads.begin(I2C_SPEED);  // I2C speed, 1000000 (1M = FMPlus) is probably OK, 400000 is safe. Defaults to 128SPS.
  ads.setRate(7);	// 860SPS
 // Serial.println("ADS BEGIN done");
  //  delay(100);
  
  // start the regular conversion process
  attachInterrupt(digitalPinToInterrupt(ADSRDY), ads_ISR, FALLING);
 // Serial.println("ATTACH int done");
  //  delay(100);  
  ads.adsStartSingleDiffRdy(adsConfig, ADSMUX_START);
  
 // zero the output current reading
 // delay(50);
 // int iZero = readADS_I();
  
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
  
  // Fan: attach the channel to the GPIO to be controlled
  ledcAttachPin(PWM_PIN, PWMChannel);  
  // configure  PWM functionalitites
  ledcSetup(PWMChannel, PWM_FREQ, PWM_RESOLUTION);
  setFan(FAN_MIN_PWM);	// start the fan on idle
  
  
  // set the mode variables  
  setMode(pSet.mode); // set initial mode indicator (from EE)
  setModeB(MODE_NOR);
  setNORb(true);	// normal mode B
  //dynSet = pSet; // done in setMode and setModeB
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
		   halCal[DAC].lowCalVal = halCal[DAC].lowCalVal * 1.333;		// DAC 
		   halCal[DAC].midCalVal = halCal[DAC].midCalVal * 1.333;		// DAC 
		   halCal[DAC].hiCalVal  = halCal[DAC].hiCalVal  * 1.333;		// DAC 
		   // lowCalval will be zero for ADS_I and ESP_I
		   // midCalVal not used
		   halCal[ADS_I].hiCalVal = halCal[ADS_I].hiCalVal * 1.33;		// ADS I
		   halCal[ESP_I].hiCalVal = halCal[ESP_I].hiCalVal * 1.33;		// ESP I
		   valChanged(VAL_CHGD); 							// update values to EEPROM
			 Serial.print("New daughter board found");
		   errorI(false, false, 1, "New daughter board found.\nRecalibrate current.");
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
		   halCal[DAC].hiCalVal = RDACHI_I;				// DAC 
		   
		   halCal[ADS_I].hiCalVal = RADSHI_I;			// ADS I
		   halCal[ESP_I].hiCalVal = RESPHI_I;			// ESP I
		   valChanged(VAL_CHGD); 						// update values to EEPROM
		   errorI(false, false, 0, "Previous daughter\nboard missing.\nRecalibrate current.");
	   }
   }
   // settings
   setVals[SV_I].maxVal = iMaxOp;	// current
   setVals[SV_P].maxVal = pMaxOp;	// power

   //readVals[RV_I].maxVal = iMaxOp;
   setVals[SV_S1A].maxVal = iMaxOp; //S1.A
   setVals[SV_S2A].maxVal = iMaxOp; //S2.A
   // halCal limits
   halCal[ADS_I].maxVal = halCal[ESP_I].maxVal = iMaxOp;				// maxVal I
   halCal[ADS_I].maxErr = halCal[ESP_I].maxErr = iMaxOp + OVERAMPSLIM;  // maxErr I
   halCal[DAC].maxVal   = halCal[DAC].maxErr = iMaxOp;	// no extra headroom for DAC
  
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
	_outOn  = isOn;
	//Serial.printf("Changing On/Off [ch %i] to %i [pin %i]\n", i, isOn, chanPins[i].onPin);
	// physical control 
	if(isOn) 		// ON
	{
		dacSet(0);
		dynSet.current = 0;
		delay(50); // slow startup
		digitalWrite(SW_ON, HIGH);  // always in OUTPUT mode, except when being explicitly read
		_logHead = _logCount = 0;	// clear and restart the log
		_lastLogged = _tLogStarted = millis(); 
		dynSet = pSet;		
	}
	else 			// OFF
	{ 
		dacSet(0); // set control voltage to OFF	
		dynSet.current = 0;
		// pulse Off pin - may be a SCPI command		
		digitalWrite(SW_ON, LOW); // on pin LOW		
		pinMode(SW_OFF, OUTPUT); 			
		digitalWrite(SW_OFF, HIGH); 
		delay(1);
		digitalWrite(SW_OFF, LOW);
		pinMode(SW_OFF, INPUT_PULLDOWN);  // reset pin for panel control
		if(_curState == BMODE_RUN) setModeB(MODE_NOR); // cancel STEP or BAT
	}	
}
// display counts and converted values
void dispConv(void)
{
	/*
	Serial.printf("Vin  [%i] = %5.2f [%i counts = %5.2fV]\n", VIN,    myADC[VIN].curVal,  adsReadings[VIN],  myADC[VIN].curVolts );
	Serial.printf("Vout [%i] = %5.2f [%i counts = %5.2fV]\n", VOUT,   myADC[VOUT].curVal, adsReadings[VOUT], myADC[VOUT].curVolts );
	Serial.printf("Iout [%i] = %5.2f [%i counts = %5.2fV]\n", IOUT,   myADC[IOUT].curVal, adsReadings[IOUT], myADC[IOUT].curVolts );
	Serial.printf("Temp [%i] = %5.2f [%i counts = %5.2fV]\n\n", TEMP, myADC[TEMP].curVal, adsReadings[TEMP], myADC[TEMP].curVolts );
	*/
}

void errorI(bool shutOff, bool DACzero, float reading, char errMsg[])
{
	char msgBuf[128];
	if (shutOff){		
		onOffKill(-1, false);    // shut off output		
	}
	if(DACzero){
		//shutDown = true;  //change setpoints to safe values		
		dacSet(0);
		dynSet.current = 0;
	}
	// display screen error message, indicator, and store for SCPI 
	sprintf(msgBuf,"*** Error: \n%s \n[%5.2f]\n", errMsg, reading);
	screenError(msgBuf, ERR_BG_B, 10, false);
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