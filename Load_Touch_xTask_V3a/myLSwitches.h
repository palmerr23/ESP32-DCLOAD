/* Encoder and panel switches

;
*/
#ifndef MYSWITCHES_H
#define MYSWITCHES_H
void onOff(int8_t channel, bool status);
// ** Tac switches and encoder **//
uint16_t oldHigh = -1;
short power = 0;
short currentDigit = 4;

long newPosition = 0, oldPosition = 0;    // no action first time around
inline long getEncPos(void)
{
#ifdef ALTENCODER
	return enc.getCount();
#else
	return enc.getPosition();
#endif	
}
#ifndef ALTENCODER
void rotate(ESPRotary& enc)
{
  newPosition = getEncPos();
  valChanged(VAL_CHGD) ;
  //Serial.print("E ");
}
#endif

// fixed this to use the setvals pre and post numbers
void clickedR(Button2& thisbut) { // up
  if (highButton < 0 || highButton >= NUMBUTS) // nothing selected
    return;
  int svx = but[highButton].sv;
  if (butDigit < (setVals[svx].pre - 1)) 
     butDigit++;
  else    // fix position if new setvalue is selected
    butDigit = setVals[svx].pre - 1;
  //Serial.printf("ButR = %i\n",butDigit);

  swPressed = true;
  butDir = 1;
  valChanged(VAL_CHGD) ;
}

// out of butDigit range also checked/fixed in displaySettings()
void clickedL(Button2& thisbut) {  //down  
  if (highButton < 0 || highButton >= NUMBUTS)
     return;
     
  int svx = but[highButton].sv; 
  if (butDigit >= (-setVals[svx].post))
     butDigit--;
  else
     butDigit = -setVals[svx].post;
  //Serial.printf("ButL = %i\n",butDigit);
   swPressed = true;
   butDir = -1;
   valChanged(VAL_CHGD);
}


#define DEBOUNCE_OO  5
uint8_t onSwitch, offSwitch;
int oo_samestate = 0;
// more complex conditions, so can't use Button2 library
// used by xTask
void processOnOffSw(void)
{
	uint8_t onVal, offVal;
	pinMode(SW_ON, INPUT_PULLDOWN); // pin drives LED, so only Input while reading
	// SW_OFF is always in Input mode, except when being pulsed by an Off command
	onVal = digitalRead(SW_ON);
	offVal = digitalRead(SW_OFF);	
	pinMode(SW_ON, OUTPUT);
	digitalWrite(SW_ON, (_outOn)? HIGH : LOW); // re-assert LED status using first channel - will be the same for all channels
	
	//debounce: must have same state for DEBOUNCE_OO cycles
	if(onVal == onSwitch && offVal == offSwitch)
		oo_samestate++;
	else
	{
		onSwitch = onVal;
		offSwitch = offVal; 
		oo_samestate = 0;
		return;
	}	
	
	if (oo_samestate >= DEBOUNCE_OO)
	{
		if (onVal == HIGH && !_outOn) // switch on, if not already on
		{
			onOff(-1, true);
		}
		if (offVal == HIGH && _outOn) // switch off
		{
			onOff(-1, false);
		}	
	}	
}

#endif
