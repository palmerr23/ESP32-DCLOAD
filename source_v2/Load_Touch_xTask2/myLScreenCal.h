/* Touch Screen Calibrator
 and ADC Calibrator

Need to define HMAX/VMAX = screen width/height in pixels
These are the touch screen readings corresponding to pixels (0,0) & (HMAX,VMAX)
	int thmin = ;
	int tvmin = ;
	int thmax = ;
	int tvmax = ;
*/
#ifndef MYSCREENCAL_H
#define MYSCREENCAL_H

bool getESPviReadings(void);
inline float readESP_V(void);
void currentZeroCal(void);
void errorI(bool shutOff, bool DACzero, float reading, char errMsg[]);

bool calOn = false;
uint8_t calStage = 0, hitcount = 0;
int scx1 = 0, scy1 = 0, scx2 = 0, scy2 = 0;
float slopex, slopey;
#define SC_HITCOUNT  5
#define SC_DIST	20		// distance of '+' from corners
#define SC_LINELEN 20	// length of '+' bars
#define SC_BG RGB565(0,20,15)
#define SC_BG2 RGB565(31,10,0)
#define SC_HIGH  0xffff

void calStop(void){
	calOn = false;
	dirtyScreen = true;
	//callStatus == CALL_IDLE;
}

int screenCal(int cmd) // cmd is ignored
{
/*
screenCals sc; // make this global when not testing 
*/	calOn = true;
	switch (calStage)
	{
		case 0:	// Starting - instructions and first + to target, move on after 5 hits 
			Serial.println("*** ScreenCal 0");
			// draw screen
			tft.fillRect(0,0, HMAX, VMAX, SC_BG);
			tft.drawLine(SC_DIST - SC_LINELEN/2, SC_DIST , 
					     SC_DIST + SC_LINELEN/2, SC_DIST, SC_HIGH); //"-"
			tft.drawLine(SC_DIST, SC_DIST - SC_LINELEN/2, 
						 SC_DIST, SC_DIST + SC_LINELEN/2, SC_HIGH); //"|"
			tft.setFont(NULL);
			tft.setTextColor(SC_HIGH);
			tft.setTextSize(2);
			tft.setCursor(HMAX/2 - 100, VMAX/2);
			tft.printf("Touch cross %i times", SC_HITCOUNT+1);
			if( hitcount > 0 && hitcount <= SC_HITCOUNT)
			{
				Serial.print("#");
				scx1 += p.x;
				scy1 += p.y;
			}
			hitcount++;
			
			// collect SC_HITCOUNT touches
			if (hitcount > SC_HITCOUNT) // calculate averages
			{
				scx1 /= SC_HITCOUNT;
				scy1 /= SC_HITCOUNT;
				hitcount = 0;
				calStage = 1;
			}			
			return CALL_PROC; // not done
			//break;
			
		case 1: // second + to target
			Serial.println("*** ScreenCal 1");
			tft.fillRect(0,0, HMAX, VMAX, SC_BG2);
			tft.drawLine(HMAX - SC_DIST - SC_LINELEN/2, VMAX - SC_DIST , 
					     HMAX - SC_DIST + SC_LINELEN/2, VMAX - SC_DIST, SC_HIGH); //"-"
			tft.drawLine(HMAX - SC_DIST, VMAX - SC_DIST - SC_LINELEN/2, 
						 HMAX - SC_DIST, VMAX - SC_DIST + SC_LINELEN/2, SC_HIGH); //"|"
			tft.setFont(NULL);
			tft.setTextColor(SC_HIGH);
			tft.setTextSize(2);
			tft.setCursor(HMAX/2 - 100, VMAX/2);
			if( hitcount > 0 && hitcount <= SC_HITCOUNT)
			{
				Serial.print("@");
				scx2 += p.x;
				scy2 += p.y;
			}
			hitcount++;
			tft.printf("Touch cross %i times", SC_HITCOUNT+1);
			// collect SC_HITCOUNT touches
			if (hitcount > SC_HITCOUNT) // calculate averages
			{	scx2 /= SC_HITCOUNT;
				scy2 /= SC_HITCOUNT;
				calStage = 2;				
			}		
			return CALL_PROC; // more processing
			//break;		
			
		case 2: // exit 
			Serial.printf("Screen touch %i, %i | %i, %i\n",scx1,scy1,scx2,scy2);
			// calculate touch unit values for pixels 0,0 and XMAX, YMAX
			slopex = 1.0 * (scx2 - scx1)/ (HMAX - 2 * SC_DIST);
			slopey = 1.0 * (scy2 - scx1)/ (VMAX - 2 * SC_DIST);
			sc.thmin = scx1 - slopex * SC_DIST; // in touch units
			sc.tvmin = scy1 - slopey * SC_DIST;
			
			sc.thmax = scx2 + slopex * SC_DIST;
			sc.tvmax = scy2 + slopey * SC_DIST;
			Serial.printf("Screen cal %i, %i | %i, %i\n",sc.thmin,sc.tvmin,sc.thmax,sc.tvmax);
			// save
			// reset for next use
			hitcount = 0;
			calStage = 0;
			scx1 = 0; scy1 = 0; scx2 = 0; scy2 = 0;
			calStop();
			valChanged(VAL_CHGD);
			return CALL_EX; // done
			break;
	}
	return CALL_ERR; // just in case
}
int tsRotate(int x){ // rotate touch screen 180 degrees
	sc.tsRot = (sc.tsRot + 2 ) % 4;
	valChanged(VAL_CHGD);
	ts.setRotation(sc.tsRot);
	return CALL_EX;
}


// *****************ADC calibration ********************
int calEntry(int dummy)
{
	calOff_I = 0.0; 
	calOff_V = 0.0; 
	return 0;
}
int calSave (int dummy)
{
    // adjust value of High Cal values (halCal[X].hiCalVal) up or down by offset value
	// dont allow Hi cal if cal point values are too low.
	
	if (abs(calOff_V) >= SMALL_DIFF)
	{
		if(meas.ADSvolts  > 10)
		{
			Serial.printf("ADS{V} hiCalVal from: %5.3f [%i], ", halCal[ADS_V].hiCalVal, halCal[ADS_V].hiCalCount);
			halCal[ADS_V].hiCalCount = ADCcount.ADSv;
			halCal[ADS_V].hiCalVal = meas.ADSvolts + calOff_V;
			Serial.printf("to: %5.3f [%i]\n", halCal[ADS_V].hiCalVal,halCal[ADS_V].hiCalCount);
			//readESP_V();
			Serial.printf("ESP{V} hiCalVal from: %5.3f [%i], ", halCal[ESP_V].hiCalVal, halCal[ESP_V].hiCalCount);
			halCal[ESP_V].hiCalCount = ADCcount.ESPv;
			//halCal[ESP_V].lowCalVal = RESPLO_V; // not at zero.
			halCal[ESP_V].hiCalVal =  meas.ADSvolts + calOff_V ; // same as for ADS
			Serial.printf("to: %5.3f [%i]\n", halCal[ESP_V].hiCalVal, halCal[ESP_V].hiCalCount);
			valChanged(VAL_CHGD);
		} else
			errorI(false, false, meas.ADSvolts , "Value too low for voltage calibration.");
	}
	if (abs(calOff_I) >= SMALL_DIFF)
	{
		if(meas.ADSamps > 1)
		{
		//	temp = calOff_I * myADC[IOUT].maxVal / myADC[IOUT].curVal; 
		//	Serial.printf("Current max was: %5.3f, change by %5.3f to %5.3f\n", myADC[IOUT].maxVal, temp, myADC[IOUT].maxVal + temp);	
			Serial.printf("ADS{i} hiCalVal from: %5.3f [%i], ", halCal[ADS_I].hiCalVal, halCal[ADS_I].hiCalCount);
			halCal[ADS_I].hiCalCount = ADCcount.ADSa;
			halCal[ADS_I].hiCalVal = meas.ADSamps + calOff_I;
			Serial.printf("to: %5.3f [%i]\n", halCal[ADS_I].hiCalVal,halCal[ADS_I].hiCalCount);
				
			Serial.printf("ESP{i} hiCalVal from: %5.3f [%i], ", halCal[ESP_I].hiCalVal, halCal[ESP_I].hiCalCount);
			halCal[ESP_I].hiCalCount = ADCcount.ESPa;
			//halCal[ESP_I].lowCalVal = RESPLO_I; // not at zero.
			halCal[ESP_I].hiCalVal = halCal[ADS_I].hiCalVal; // same as for ADS	
			Serial.printf("to: %5.3f [%i]\n", halCal[DAC].hiCalVal, halCal[DAC].hiCalCount);
			
			Serial.printf("DAC hiCalVal from: %5.3f [%i], ", halCal[DAC].hiCalVal, halCal[DAC].hiCalCount);
			halCal[DAC].hiCalCount = DACis;
			halCal[DAC].hiCalVal = halCal[ADS_I].hiCalVal; // same as for ADS	
			Serial.printf("to: %5.3f [%i]\n", halCal[DAC].hiCalVal, halCal[DAC].hiCalCount);
			
			valChanged(VAL_CHGD);
		} else
			errorI(false, false, meas.ADSamps , "Value too low for current calibration.");
	}

	if(abs(calOffLo) > SMALL_DIFF)
	{
		Serial.printf("ESP_T: loCalTemp from: %5.3f [%i], ", halCal[ESP_T].lowCalVal, halCal[ESP_T].lowCalCount);
		halCal[ESP_T].lowCalVal += calOffLo;
		valChanged(VAL_CHGD);
		Serial.printf("to: %5.3f [%i]\n", halCal[ESP_T].lowCalVal, halCal[ESP_T].lowCalCount);
	}
	if(abs(calOffHi) > SMALL_DIFF)
	{
		Serial.printf("ESP_T: hiCalTemp from: %5.3f [%i], ", halCal[ESP_T].hiCalVal, halCal[ESP_T].hiCalCount);
		halCal[ESP_T].hiCalVal += calOffHi;
		valChanged(VAL_CHGD);
		Serial.printf("to: %5.3f [%i]\n", halCal[ESP_T].hiCalVal, halCal[ESP_T].hiCalCount);
	}
	calOffHi = calOffLo = 0.0;
	return 0;
}

int exitCal(int x){
// Nothing to do
	return 0;	
}

// only use when Volts/current are both known to be zero (i.e. disconnected)
int setZeroCal(int x)
{
	currentZeroCal(); // update the current zero cal while we're here
	getESPviReadings(); // ADS always fresh reading, ESP not always
	
	Serial.printf("Zero counts: ADS V %i, ESP V %i, \n", ADCcount.ADSv, ADCcount.ESPv);
	// assume difference is only in the 'zero value' count not the offset to the readable value
	Serial.printf("ADS: lowCalVal from: V %5.3f [%i] I, %5.3f [%i]\n", halCal[ADS_V].lowCalVal, halCal[ADS_V].lowCalCount, halCal[ADS_I].lowCalVal, halCal[ADS_I].lowCalCount);
	halCal[ADS_V].lowCalCount = ADCcount.ADSv; //adjust from defined values + CADSBASEPLUS_V 
	Serial.printf("                 to: V %5.3f [%i]\n", halCal[ADS_V].lowCalVal, halCal[ADS_V].lowCalCount);
	
	Serial.printf("ESP: lowCalVal from: V %5.3f [%i] I, %5.3f [%i]\n", halCal[ESP_V].lowCalVal, halCal[ESP_V].lowCalCount, halCal[ESP_I].lowCalVal, halCal[ESP_I].lowCalCount);
	halCal[ESP_V].lowCalCount = ADCcount.ESPv;  // + CESPBASEPLUS_V 
	Serial.printf("                 to: V %5.3f [%i]\n\n", halCal[ESP_V].lowCalVal, halCal[ESP_V].lowCalCount);
	valChanged(VAL_CHGD);
	

	
	return 0;	
}
// only auto set current zero, as voltage sense is on the wrong side of the relay.
void currentZeroCal(void)
{
	  if(_outOn == false)
	  {	
		  halCal[ADS_I].lowCalVal = 0.0; // values must be zero for this to work
		  halCal[ESP_I].lowCalVal = RESPLO_I; // offset for low currents
		  
		 // delay(10);	// let things settle
		 // getESPviReadings();
		 // delay(10);
		  getESPviReadings();
		  halCal[ADS_I].lowCalCount = ADCcount.ADSa; // values must be zero for this to work
		  halCal[ESP_I].lowCalCount = ADCcount.ESPa;

		// Serial.printf("Current zero: ESP count = %i\n",  halCal[ESP_I].lowCalCount);
		
	  }
	  // this is a dynamic value: do not set valChanged

}

void printHalCal()
{
  Serial.printf("HalCal: dPresent = %c\n", (myID.daughter_present) ? 'T' :'F');
  for(int i = 0; i < NUM_ADC_CHANS; i++)
  {
    Serial.printf("%i: Min %03.3f, ", i, halCal[i].minVal);
    Serial.printf("Max %3.3f, ", halCal[i].maxVal);
    
    Serial.printf("minErr %3.3f, ", halCal[i].minErr);
    Serial.printf("maxErr %3.3f, ", halCal[i].maxErr);
    
    Serial.printf("lowCal %3.3f, ", halCal[i].lowCalVal);    
    Serial.printf("hiCal %3.3f, ", halCal[i].hiCalVal);
    
    Serial.printf("lowCalC %5i, ", halCal[i].lowCalCount);
    Serial.printf("hiCalC %5i\n", halCal[i].hiCalCount);
/*
  float   halCal[i].midCalVal;    // three point, if needed (DAC, maybe ESPADC)
  uint16_t  halCal[i].midCalCount;  // three point, if needed
  */
  }
    Serial.printf("DAC midCal %3.3f, midCalC %5i\n",  halCal[DAC].midCalVal, halCal[DAC].midCalCount);
}


#endif