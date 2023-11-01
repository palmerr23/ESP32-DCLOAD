/* Touch Screen Processing and Value Editing 
myTouchProcess.h
*/
#ifndef MYLTOUCHPROC_H
#define MYLTOUCHPROC_H

void postProcess(void);
int blockTouched(TS_Point p);
void updateVal(short highButton);
void printModes(void);

// wireless autoconnect
int toggleWconn(int x){
	myID.autoConnect = !myID.autoConnect;
Serial.printf("Wifi Auto = %i\n",myID.autoConnect);
	if(myID.autoConnect) // restart Wifi
	{
		wifiBegin();
	}
	else
	{
		wifiEnd(false); // leave the wifi adapter on
	}
	valChanged(VAL_CHGD); // no broadcast
	return CALL_NOEX;
}

// triangle/sawtooth STep mode
/*
int toggleTriangle(int x)
{
	pSet.triangle  = !pSet.triangle;
	//Serial.printf("[T %i] ",pSetX.trackSv);
			  	//Serial.print("$V");
	valChanged(VAL_CHGD) ;
	return CALL_NOEX;
}
//Off after finish sequenc
int toggleOffAfter(int x)
{
	pSet.offAfter  = !pSet.offAfter;
	//Serial.printf("[T %i] ",pSetX.trackSv);
			  	//Serial.print("$V");
	valChanged(VAL_CHGD) ;
	return CALL_NOEX;
}
*/
uint8_t setMode(uint8_t aMode)
{
	if (pSet.modeB == MODE_BAT && aMode == MODE_CV)	//  mode not allowed for Battery testing
		aMode = MODE_CC;
	pSet.mode = aMode;
	for (int i = 0; i < C_MODES; i++)		// XOR modes
	   inMode[i] = (i == aMode) ? true : false;
  firstGuess = true; // re-initialise modeCalc
	dirtyScreen = true;
	notSTmode = !(pSet.modeB == MODE_ST); // hide main screen settings in TR mode
	dynSet = pSet; // update dynamic settings
	valChanged(VAL_CHGD);
    return aMode;
}
// mode button callbacks, argument is a dummy
int setCCmode(int x)
{
  onOff(0, false); // can't change mode mid-flight
	pSet.mode  = setMode(MODE_CC);
	dynSet = pSet;
  firstGuess = true;
	return CALL_NOEX;
}

int setCVmode(int x)
{
	onOff(0, false); // can't change mode mid-flight
	pSet.mode  = setMode(MODE_CV);	
  firstGuess = true;
	return CALL_NOEX;
}
int setCPmode(int x)
{
  onOff(0, false); // can't change mode mid-flight
	pSet.mode  = setMode(MODE_CP);	
  firstGuess = true;
	return CALL_NOEX;
}
int setCRmode(int x)
{
  onOff(0, false); // can't change mode mid-flight
	pSet.mode  = setMode(MODE_CR);	
  firstGuess = true;
	return CALL_NOEX;
}

// B mode - independent of main mode setting, other than CV not allowed for MODE_BAT
// Sets the mode to STOP - individual routines control start/stop behaviour.
uint8_t setModeB(uint8_t bMode)
{		
	pSet.modeB = bMode;
	for (int i = 0; i < B_MODES; i++)	// disable other modes
	   isModeB[i] =  BMODE_OFF;
   
	if(bMode != MODE_NOR)
		isModeB[bMode] = BMODE_RUN; //running	   
	else
	{
		isModeB[bMode] = BMODE_SEL; //running	
		dynSet = pSet;
	}
  notSTmode = !(pSet.modeB == MODE_ST); // hide main screen settings in ST mode (BAT mode uses main settings for end values...)
	dirtyScreen = true;
    valChanged(VAL_CHGD_NOEE); //no need to store
	return bMode;
	//printModes();
}

int startBAT(int x)
{	
	if(pSet.modeB == MODE_BAT) // already in BAT mode, second press exits
	{
		_batStop = true;
		return CALL_NOEX;
	}
	setModeB(MODE_BAT); // turn off other modes
	//nextSeq(temp, MODE_BAT); // On/Off			
	if (pSet.mode == MODE_CV)	//  CR mode not allowed for Battery testing
	{
		setCCmode(1); // default to CC (argument is any integer)
		onOff(0, false); // turn off output (should already be there!)
		return CALL_NOEX;
	}
	// _curStage = 0; //??
	// 	_curCycle = 0; //??
	_curState = BMODE_SEL;
	return CALL_NOEX;
}
int startST(int x)
{	
	setModeB(MODE_ST); // turn off other modes
	//nextSeq(temp, MODE_ST); // On/Off
	_curState = BMODE_SEL;
	_curStage = 0;
	_curCycle = 0;
	return CALL_NOEX;
	
}
int setNORb(int x)
{
	if(pSet.modeB == MODE_BAT && _curState == BMODE_RUN) // let battery mode exit normally, but not if starting up.
	{
		 _batStop = true;
		 return CALL_NOEX;
	}
		
	setModeB(MODE_NOR); // turn off BAT or STEP mode
	_curState = BMODE_SEL;
	dynSet = pSet;	// go back to standard settings
	setMode(dynSet.mode);
	onOffKill(0, false); // turn off output
	return CALL_NOEX;
}
void printModes(void)
{	int i;
	Serial.printf("\nModes: mode %i, modeB %i\nAmodes: ", pSet.mode, pSet.modeB);
	for (i = 0; i < C_MODES; i++)	// disable other modes
	   Serial.printf(" %i = %i, ", i, inMode[i]);
	Serial.printf("\nBmodes: ");
	for (i = 0; i < B_MODES; i++)	// disable other modes
	   Serial.printf(" %i = %i, ", i, isModeB[i]);
	Serial.printf("\n, notSTmode %i\n",notSTmode);
}

// no longer used
void nextSeq(uint8_t mode, uint8_t but)
{
	switch (mode) // simple state machine
	{
		case BMODE_OFF :	
			isModeB[but] = BMODE_SEL;
			break;
		case BMODE_SEL :	// run / stop alternately, once selected
			isModeB[but] = BMODE_RUN;
			break;
		case BMODE_RUN :
			isModeB[but] = BMODE_SEL;
			break;
	}	
}
/*globals used by other routines
  highButton - the button currently highlighted on screen
  dirtyScreen - something has changed that requires the entire screen to be re-drawn (e.g. menu change)
  valChanged(VAL_CHGD) - requires settings to be redrawn/ changed in EEPROM / SCPI etc
 * parameter anyTouch: set TRUE to wait for toch anywhere on the screen (e.g. to clear error message)
 * returns: TRUE when anyTouch is TRUE and the screen has been touched, FALSE otherwise
 */
// process touch screen, switches and encoder
bool firstCall = true;
short procBut = -1, oldButton = -1;
bool processTouchSwEnc(bool anyTouch = false)
{
  short blockT, lsv = -1, retVal, cmd;
  char lfmt = '\0';
  int cb = CALL_EX;	// default callback result
  bool oskStatus;
  
  newPosition = getEncPos(); // encoder change?
  
  // anyTouch just tests for a screen touch. Blocks any other processing 
  if (anyTouch) 
  {
    if (ts.touched()) 
      return true;
    return false;
  }
  
  if (ts.touched())       // keep checking until released - then use value.
    {
      p = ts.getPoint(); 
      wastouched = true;    
    } else 
    {
      if (wastouched) 
      {   
			wastouched = false;
			blockT = blockTouched(p); 
			// Serial.printf("%i [%i] (%i, %i) [%i, %i]\n",blockT, currentMenu, p.x, p.y, pxh(p.x), pxv(p.y));
			// keep last touched block until another legal block is touched - ignored for OSK and ScreenCal
			if (blockT >= 0 && !oskActive()) // ignore touches outside buttons and while OSK is displayed.
			{	
				//Serial.printf("Touch: block = %i [M %i], status %i, %s\n",blockT, currentMenu, callStatus, (but[blockT].callback != NULL) ? "CB" : "No CB" );
				// callbacks may persist for multiple cycles (state-variable) and must be non-blocking
				// not currently in an active callback
				if (callStatus == CALL_IDLE) 
				{	
					highButton = blockT; // persistent record of last menu button pressed
					lsv = but[blockT].sv; 
          if(lsv != NOSETVAL)
					  lfmt = setVals[lsv].fmt; 
					//Serial.printf(" setting %i, format[%c]\n", lsv, lfmt);	delay(10);
					
					if(but[blockT].callback != NULL)  //need to process a *new* callback?
					{												
						procBut = blockT;
						firstCall = true;	
						callStatus = CALL_START;
						//Serial.printf("Callback starting\n");					
					}	

					//  OSK needs special initialisation: touched block's related setVal.sv has ASCII ('A' or 'S') format
					if ((lfmt == 'A'|| lfmt == 'S') && !oskOn )
					{
						//Serial.printf("OSK starting on [%s]\n",(char *)setVals[lsv].vp);
						oskStart((char *)setVals[lsv].vp, 130); // nominal ypos, OSK could calc this for bottom part of the screen
					}					
				}
			}
			if (oskActive())
				oskStatus = oskProcess(CALL_CMD_PROC);

			// callback on originally highlighted button (screenCal and OSK) ??? for toggles?
			// use procBut here to maintain thread, despite what's touched on the screen
			if (but[procBut].callback != NULL && callStatus != CALL_IDLE)
			{
				if (firstCall) 
				{
					cmd = CALL_CMD_START;
					firstCall == false;
				}
				else
					cmd = CALL_CMD_PROC;
				
				//Serial.print("C ");
				delay(10);
				cb = but[procBut].callback(cmd); // issue callback
				
				switch (cb)
				{
					case CALL_PROC:
						callStatus = CALL_ACTIVE;
						retVal = procBut; // possibly some post-processing 
					break;
					case CALL_EX:
						callStatus = CALL_IDLE;
						wastouched = false;
						retVal = -1; // possibly some post-processing 
						break;
					case CALL_NOEX: 
						callStatus = CALL_IDLE;
						wastouched = false;
						retVal = procBut; // retain focus on this button
						break;
					case CALL_ERR:
						callStatus = CALL_IDLE;
						wastouched = false;
						return false; // it's broken, so just exit						
				}
			} // end callback processing 	
				
		  } // end wasTouched

	} // end null processing (not touched now, or before)
	
		
	// encoder and buttons 	- independent of screen touch
	if (callStatus == CALL_IDLE)  // not during active callbacks 
	{
		// establish setVal and format for this button
		if (highButton >= 0 && highButton < NUMBUTS)
		{	
			lsv = but[highButton].sv; 	
			if(lsv == NOSETVAL)
				lfmt = 'Z';
			else
				lfmt = setVals[lsv].fmt;  			
		}
		else 
		{
			lsv = -1;
			lfmt ='Z';
		}
				
	  	// encoder processing	  
		if (newPosition != oldPosition) // encoder rotated, update value associated with last block touched
		{
	//Serial.printf("Rx %i, %i", newPosition,oldPosition);
 
		  updateVal(highButton);      
		  oldPosition = newPosition; 
		//  	Serial.print("N");
		  valChanged(VAL_CHGD);		  
    
		}
		
		if (swPressed && lfmt == 'T') // L or R switch pressed (Toggles)
		{	
	//Serial.print("T ");
			*(uint8_t*)setVals[lsv].vp = (butDir > 0) ? false : true;  	
			//  button colours are handled dynamically in drawIndicators and drawButtons
					  	Serial.print("P");
			valChanged(VAL_CHGD);	 				
			swPressed = false;						
		}	  
					
		// whole screen redraw
		if ((dirtyScreen || (highButton != oldButton)) && !holdScreen) 
		 {   
		 //Serial.printf("1: %i ", lsv);
		 	//Serial.print("R"); delay(1);
		   drawSettings(lsv, butDigit, true);
		   drawButtons(highButton);
		   drawReadings();
		   drawIndicators(currentMenu);
		  // dirtyScreen = false;
		   changedVal_scrn = false;	 	
		   oldButton = highButton; 
		 }
		 
		 // less radical redraw
		 if (changedVal_scrn && !holdScreen)
		 {
  			//Serial.printf("W [%i][%i]",changedVal_scrn, true);
  			// Serial.print("Q"); delay(1);
  			drawButtons(highButton);			  
  			drawIndicators(currentMenu);	
  	//Serial.printf("2: %i ", lsv);
  	//Serial.print('$');
  
  			drawSettings(lsv, butDigit, (oldButton != highButton)? true : false);
  			changedVal_scrn = false;	
  //Serial.printf("WX [%i][%i]\n", changedVal_scrn, true);			
		 }
	 } // CALL_IDLE
  return wastouched;
}

int blockTouched(TS_Point p)
{
	uint8_t xbut, ybut;
    // which button has been pressed in the current menu?
    for (int i = 0; i< NUMBUTS; i++)
    { //tb[i] = {but[i].xpos, 320, but[i].ypos, but[i].ypos + BUTSIZ }
	  xbut = (but[i].siz & 0x02) ? BUTSIZ : BUTSIZ/2; // half size buttons
	  ybut = (but[i].siz & 0x01) ? BUTSIZ : BUTSIZ/2;
      if (but[i].menu == currentMenu 
            && pxh(p.x) >= but[i].xpos && pxh(p.x) <= (but[i].xpos + xbut) 
            && pxv(p.y) >= but[i].ypos && pxv(p.y) <= (but[i].ypos + ybut)
          )
      {
		if(but[i].displayIf == NULL || *(but[i].displayIf)) // conditional display
		{		  /*     
			Serial.printf("CM = %i, Touched (%i,%i) [%i, %i]",currentMenu, p.x, p.y, pxh(p.x),pxv(p.y));
			Serial.print(but[i].text);  
			Serial.print(", Highlight ");
			Serial.println(highButton);
			*/
			// change menu if required
			if (currentMenu != but[i].nextMenu)
			{
				highButton = 999;  // no highlight in new menu
				dirtyScreen = true;
			}
			currentMenu = but[i].nextMenu; // could be the same as this one        

			return i; // no need to try the rest of the blocks
        } 
	  }
    }
    return -1;
}
// update the setting value using the (touch button) index to the settings array and the (L&R buttons) digit to change
// encoder position is global
// for simplicity, all arithmetic is done in float: convert *vp -> float -> process -> save as native format
// alters butdigit if it's outside the range of the input.
void updateVal(short btn)
{
  float vTemp;
  char fTemp;
  uint8_t svx;
 // float minVal, maxVal;     
  
  if (btn > NUMBUTS  || btn < 0) // no button highlighted
    return;

  svx = but[btn].sv;
  fTemp = setVals[svx].fmt;	
  if (fTemp == 'I' && butDigit < 0) // integers can't alter 0.xx values. Move to 1's digit
	  butDigit = 0;

 // minVal = setVals[svx].minVal;
  //maxVal = setVals[svx].maxVal;

  //Serial.printf("updateVal: btn %i, digit %i, setting %i, ",btn, digit,svx);
   //Serial.print(fTemp);
    switch (fTemp) // ensure digit being highlighted/edited is within pre-post range
	{
	  case 'I': //  8 bit unsigned Integer 
	  case 'U': //  16 bit unsigned Integer 
    case 'L': //  32 bit unsigned Integer     
	  case 'B': // bracketed integer
	  case 'F': // float
		if (butDigit >= setVals[svx].pre)    butDigit = setVals[svx].pre -1;
		if (butDigit < (-setVals[svx].post)) butDigit = -setVals[svx].post;
		break;
	  // do nothing for text and toggle
	}
   switch (fTemp) 
   {
     case 'T': // Toggle
       vTemp = *(uint8_t*)setVals[svx].vp;
       butDigit = 0; // true/false always +/- 1
       break;    
     case 'I': //  8 bit unsigned Integer 
       vTemp = *(uint8_t*)setVals[svx].vp;
       break;
	   case 'U': //  16 bit unsigned Integer 
       vTemp = *(uint16_t*)setVals[svx].vp;
       break;
     case 'L': //  32 bit unsigned Integer 
       vTemp = *(uint32_t*)setVals[svx].vp;
       break;
     case 'B': // bracketed integer
       vTemp = *(uint8_t*)setVals[svx].vp ;
       break;
     case 'F': // float
       vTemp = *(float*)setVals[svx].vp ;
       break;    
  }
//Serial.printf("val %6.4f [%c]",vTemp, fTemp);
  if (newPosition > oldPosition) // increment
  {
   //Serial.print("+");
    //Serial.print(setVals[svx].maxVal);
    //  maximum set value 
    if ((vTemp + pow(10, butDigit) ) <=  setVals[svx].maxVal)
        vTemp += pow(10, butDigit);
    else  
        vTemp = setVals[svx].maxVal;
  }
  else // decrement
  {
   // Serial.print("-");
    //  minimum set value      
    if((vTemp - pow(10, butDigit) ) >=  setVals[svx].minVal)
      vTemp -= pow(10, butDigit);
    else  
      vTemp = setVals[svx].minVal;
  }
 //Serial.printf("-> %6.4f\n",vTemp);
  //Serial.println(vTemp);
    switch (fTemp) {
    case 'T': // Toggle
      *(uint8_t*)setVals[svx].vp = (uint8_t)vTemp;
      break;    
    case 'I': // 8 bit Integer 
      *(uint8_t*)setVals[svx].vp = (uint8_t)vTemp;
      break;
	  case 'U': // 16 bit unsigned Integer 
      *(uint16_t*)setVals[svx].vp = (uint16_t)vTemp;
      break;
    case 'L': // 32 bit unsigned Integer 
      *(uint32_t*)setVals[svx].vp = (uint32_t)vTemp;
      break;
    case 'B': // bracketed integer
      *(uint8_t*)setVals[svx].vp = (uint8_t)vTemp;
      break;
    case 'F': // float
      *(float*)setVals[svx].vp = vTemp;
      break;    
  }
  valChanged(VAL_CHGD);
  changedLocal = true;
}

void postProcess(void)
{
	// redundant
}
#endif
