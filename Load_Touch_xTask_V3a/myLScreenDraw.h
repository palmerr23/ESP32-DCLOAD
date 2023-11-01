/* Screen Drawing routines 

Need to define HMAX/VMAX = screen width/height in pixels
SCREENROT - which orientation to use (also used by Touch)
*/
#ifndef MYSCREENDRAW_H
#define MYSCREENDRAW_H

#include "myLScreenDefs.h"
bool processTouchSwEnc(bool);
void printX(valFmt * xp,  short highlight, uint16_t txtcol, uint16_t highcol, uint16_t bgcol, bool editing);
//void printfloatX(void * vp, char* v2, void * ov, uint8_t fp,  uint8_t textSize, short pre, short post, short highlight, uint16_t x, short y, char fmt, uint16_t txtcol, uint16_t highcol, uint16_t bgcol);

void screenBegin(void)
{
	tft.begin();
	tft.fillScreen(BGCOL);
	tft.setRotation(sc.scrRot); // defaults - reset after reading profile from EEPROM
	ts.begin();
	ts.setRotation(sc.tsRot); 
}

// calculate the correct mode for toggles
short tMode(short sv){

	// Limit
	if (sv == LBUT)
	{	  
		if(_isLimiting)
			return BBACT;
		else
			return BBOFF;	  
	}

	// WiFi
	if (sv == WBUT)
	{
	  if (myID.autoConnect)
	  {
		if(WifiConnected)
		{
			//Serial.println("WBUT Act");
			return WIFIACT;
		}
		else
		{
			//Serial.println("WBUT InAct");
			return WIFIINACT;
		}
	  }
	  else // disabled	  
	  {
		   //Serial.println("WBUT Off");
		   return WIFIOFF;
	  }
	}
	
	return -1; // sv not found
}


// colour for button and indicator background - toggles
uint16_t tColour(short sv, char mode){
	// indicators as buttons except "off" is screen background colour
	short tm;
	tm = tMode(sv);
	if (tm <= 0)
	{ 
	  if (mode == 'I' || mode == 'U')
	    return BGCOL;
	  else
		  return BBXHIGH;
	}
	return tColours[tm];
}	
#define ABUT (BUTSIZ/3)
#define ABUTR (ABUT/2 -1)
#define ABUTX (BUTSIZ-10)
#define ABUTOFF 2
void drawIndicators(short aMenu)
{
	int8_t tm; 
	uint16_t xcol, xpos, ypos, butht;
	
	if (oskOn || calOn)
		return;
	tft.setFont(myFonts[0]);
	// every screen indicators
	
	// wireless connected & mode
	//tft.fillRoundRect(VALX+40, 0, ABUT, ABUT,  ABUTR, (WifiConnected)?IENABLE:ITRIG); 
	tft.setCursor(sx(READX + 70), sy(TOPLEGS));
	tft.setTextColor((WifiConnected)?IND_ONC:IND_OFFC);
  if(wifiStatus == WIFI_SOFTAP) 
	  tft.print("AP"); // only of connected as Soft AP
  else
   tft.print("W");
	
	// save to EE pending
	if (needToSaveEE) // save to EE indicator
		tft.setTextColor(MY_YELLOW);
	else
		tft.setTextColor(BGCOL);		
	tft.setCursor(sx(READX + 90), sy(15));
	tft.print("E");

  // voltage measurement mode
  tft.setCursor(sx(READX + 110), sy(TOPLEGS));
  tft.setTextColor(MY_GREEN);
  tft.print(pSet.vMode);
  tft.print(_cur_vMode);

	tft.setTextColor(BHIGHCOL);
	int ystart;
	int xstart;
  switch (aMenu)
  {
    case 0:		// main menu
	if (!notSTmode) // in TR mode main settings are hidden
	{
		tft.setFont(myFonts[1]);
		tft.setCursor(sx(200), sy(100));
		tft.setTextColor(MY_WHITE);
		tft.print("STep Mode");
	}	
		// limiting indicator
		//tm = tMode(LBUT);
		ystart = 1.5 * BVSPACE + 1;
		xstart = ABUTX;
		//xcol = tColour(LBUT, 'I');
		xcol = (_isLimiting) ? MY_RED : BGCOL;

		//if (tm == BBACT) xcol = (myBlink)? xcol : BGCOL;			
		tft.fillRoundRect(sx(xstart), sy(ystart) - ABUT, ABUT, ABUT,  ABUTR, xcol ); //BGCOL/

		// temp indicator
	
		xcol = (meas.ESPtemp < TEMP_FANSTART) ? MY_GREEN : MY_ORANGE;
		if(meas.ESPtemp >  TEMP_FANFULL)
			xcol = MY_RED;
		if(meas.ESPtemp >  TEMP_SHUTDOWN)
			xcol = (myBlink) ? xcol : BGCOL;				
		tft.fillRoundRect(sx(VALX), sy(6), sx(8), sx(8),  4, xcol); 		

		// fan on	

		break;
  
    case 1: // comms
		break;
  
    case 2: // step
		break;
	
	case 3: //cal
    tft.setFont(myFonts[1]);
    tft.setTextSize(1);
    tft.setCursor(sx(120), sy(140));
    tft.print(sc.tsRot);
    tft.setCursor(sx(120), sy(180));    
    tft.print(sc.scrRot);
		break;
  }
}

// draw legends on the menu screen
void drawLegends(short cMenu){
  short i = 0;
  uint16_t tempClr;
  if (oskOn || calOn)
		return;
  //for (short i =0; i< NUMLEGS; i++)
  while (leg[i].menu >= 0) // terminating item has -1 flag
  {
    if (leg[i].menu == cMenu)	  
	{
  	   // check for conditional display - print in Background colour to erase
	   if(leg[i].displayIf == NULL || *(leg[i].displayIf)) 
		 tempClr = leg[i].clr;
	   else
		 tempClr = BGCOL;

	   tft.setFont(myFonts[leg[i].font]);// set font and text size first, or cursor baseline will be wrong 
	   tft.setTextSize(leg[i].textSize);	
	   tft.setCursor(leg[i].xpos, leg[i].ypos);
	   tft.setTextColor(tempClr);
	   tft.print(leg[i].text);
	}    
    i++;
  }
}

#define DISP_BUT 0x04
void drawButtons(short highlight)   
{
  uint16_t  cwid, cht; 
  int16_t  xdum, ydum, xht, yht, brnd;
  uint16_t tClr, brdClr;
  
   if (oskOn || calOn)
		return;
	
   for (int i = 0; i<NUMBUTS; i++)
     if((but[i].menu == currentMenu) && (but[i].siz & DISP_BUT)) 
     {    
	  if(but[i].displayIf == NULL || *(but[i].displayIf))  // displayIF
	  {
			 
       xht = (but[i].siz & 2) ? BUTSIZ : BUTSIZ/2; // bitmapped size variable - only two sizes supported
       yht = (but[i].siz & 1) ? BUTSIZ : BUTSIZ/2;
       brnd = (!(but[i].siz & 3)) ? BUTROUND : BUTROUND/2; // either dimension is small: less rounding
        
        if(i == highlight)
        {
          tft.setTextColor(BHIGHCOL);
          brdClr = BBORDHIGH;
		  tClr = but[i].selColour;              
        }
        else
		{
          tft.setTextColor(BTEXTCOL);
          brdClr = BBORD;
		  tClr = but[i].unselColour;
        }
		if(but[i].onIf != NULL) // onIF
		{	uint8_t temp = *(but[i].onIf);
			if(temp)
			{
				tft.setTextColor(BONCOL);
				brdClr = BBORDHIGH;				
			}
			else
			{
				tft.setTextColor(BTEXTCOL);
				brdClr = BBORD;				
			}
			switch (temp) // three states: selected
			{
				case BMODE_OFF :	
					tClr = but[i].unselColour;  
					break;
				case BMODE_SEL :	// run / stop alternately, once selected
					tClr = but[i].selColour;
					break;
				case BMODE_RUN :
					tClr = IND_OFFC;  
					break;						
			}
		}
		// special colour treatment for Toggle buttons
		short tsv = but[i].sv;	
		if (tsv != NOSETVAL) // don't combine ifs - could cause pointer error
			if(setVals[tsv].fmt == 'T')
				tClr = tColour(tsv, 'B');

		
		//if(oldHigh != highlight) // context sensitive buttons get redrawn each time
		tft.fillRoundRect(but[i].xpos, but[i].ypos, xht, yht,  brnd, tClr); 
        
		if(but[i].border)
          for (short j = 0; j < BUTEDGETHICK; j++) // extra thickness on outside of basic outline
            tft.drawRoundRect(but[i].xpos-j,but[i].ypos-j, xht+2*j, yht+2*j,  brnd, brdClr);      
        switch (strlen(but[i].text))
        {
          case 1:
		  case 0:
            tft.setFont(myFonts[3]);
            break;
          case 2:
            tft.setFont(myFonts[2]);
            break;
		  case 3:
            tft.setFont(myFonts[1]);
            break;
          default: // 4 or more chars
            tft.setFont(myFonts[0]);
        }           
        tft.getTextBounds(but[i].text, 0,0, &xdum, &ydum, &cwid, &cht); 
        tft.setCursor(but[i].xpos + (xht-cwid)/2, but[i].ypos + (yht + cht)/2);
        tft.print(but[i].text);     
	  }
	 } // end if, if, for
     oldHigh = highlight;
}

short readCntr = 0;
// non blocking code - one reading per call
void drawReadings(void) {  
  uint16_t tempClr;
  short i = 0;
  if (oskOn || calOn)
		return;
  if (readCntr >= NUMREADS)
	  readCntr = 0;	// start again

 // for (short i =0; i< NUMREADS; i++)
  {
		if(readVals[readCntr].menu == currentMenu)
		{
			// check for conditional display - print in Background colour to erase
			if(readVals[readCntr].displayIf == NULL || *(readVals[readCntr].displayIf)) 
			tempClr = readVals[readCntr].clr;
			else
			tempClr = BGCOL;		
			printX(&readVals[readCntr], butDigit, tempClr, tempClr, BGCOL, false);
		}
    readCntr++;
  }
}
  

// selected is the index to setVals[]
// highlight digit for editing
void drawSettings(short selected, short highDigit, bool all) 
{
  short i = 0;
  uint16_t tempClr;
  if (oskOn || calOn)
		return;
	
  changedVal_scrn = false;	
  //Serial.printf("Sets sel = %i, hd = %i, all = %i\n", selected, highDigit, all); delay(20);
  while (setVals[i].vp)//(short i = 0; i < NUMSETTINGS; i++)
  {
    if(setVals[i].menu == currentMenu) 
    {
	  // check for conditional display - print in Background colour to erase
			if(setVals[i].displayIf == NULL || *(setVals[i].displayIf)) 
				tempClr = setVals[i].clr;
			else
				tempClr = BGCOL;	
		  if (i == selected)
			{
					//Serial.print(" Sel ");
          // ensure a legitimate digit is highlighted       
//numtest = true;  
					printX(&setVals[i], highDigit, tempClr, HIGHLIGHTCOL, BGHIGH, true); 
//numtest = false;
			}
			else // don't always redraw out of focus settings
				if (all)
					printX(&setVals[i], highDigit, tempClr, tempClr, BGCOL, false);
			}
    i++;
  }
}

void redrawScreen(void){
  short lsv; 
  
  if (oskOn || calOn)
		return;
  
  tft.setRotation(sc.scrRot);
  ts.setRotation(sc.tsRot);
  tft.fillScreen(BGCOL);  
  tft.setTextColor(TEXTCOL);
  drawLegends(currentMenu);

  if(highButton < 0 || highButton > NUMBUTS)
	  lsv = -1;
  else
	  lsv = but[highButton].sv;
//Serial.printf("3: %i ", lsv);
//Serial.print("D");
  drawSettings(lsv, butDigit, true); 
  drawButtons(highButton);
  // readings and indicators redrawn each cycle in main code

  // bits and pieces of screen drawing
  switch (currentMenu)
  {
    case 0: // main
      tft.setFont(myFonts[1]);
      tft.setCursor(sx(5), sy(TOPLEGS));
	    // do this here, rather than as a Reading to save update time.
      tft.print(myID.instName);
      tft.drawFastVLine(sx(180), sy(25), sy(170), CYAN_M); // vertical line between readings and settings
      break;
  
    case 1: // COMMS
	    tft.drawFastHLine(sx(2), sy(180), sx(260), CYAN_M); // horz line separating panels
      break;
  
    case 2: //STEP
	    tft.drawFastVLine(sx(BUTX*0.55), sy(20), sy(170), CYAN_M); // vertical line between readings and settings
      break;
	  
	case 3: //CAL
	   tft.drawFastHLine(LEG2X, TOPLINE+3*BVSPACE2,  HMAX -  2 * LEG2X, CYAN_M);
	   tft.drawFastHLine(LEG2X, TOPLINE+7*BVSPACE2,  HMAX -  2 * LEG2X, CYAN_M);
     break;
  }
  dirtyScreen = false;
  changedVal_scrn = false;	
}

#define BORDER_HL 3 // extra space beyond char footprint of highlight
// Fancy print - with highlighting for currently editing value.
// Only works with readings or settings
// Assumes fixed width font, of width "N".
// prints leading spaces to pad to desired size (align decimals)
// fmt: 'F' print as float; 'I'  'U' 'L' - unsigned 8,  16, 32  bit int (decimal justified); 'P' - float as percentage, 'B' - bracketed Int; 'T' = On/off (1/0; T > 0.1); 'S' = String; 'A' = safe string (no unprintable chars)
void printX(valFmt * xp,  short highlight, uint16_t txtcol, uint16_t highcol, uint16_t bgcol, bool editing)
{  
  if(holdScreen)
    return;
  // bgcol = RGB565(0,0,30); // testing block out
  char ch, buf[128], fmt;
  uint16_t xcursor;
  uint16_t  cwid, cht, x, y, pre, post; 
  int16_t  xdum = 100, ydum = 100;
  short digit, powVal;
  float val, pownum, tempVal, max, min;
  uint8_t cCount = 0, fp;
  short i;
  int maxi;
  x = xp->xpos;
  y = xp->ypos;
  pre = xp->pre;
  post = xp->post;
  fp = xp->font;
  fmt = xp->fmt;
  max = xp->maxVal;  // always a float
  min = xp->minVal; 
  xcursor = x;
  powVal = pre - 1;
  bool printedDot = false;
  bool leadZero = false; 	// true == zeros, false == spaces. Should be a parameter
  bool overflow = false;	// value is greater than allowed
    //Serial.print("H = ");Serial.println(highlight);
  // find the size of the "en" character block - assumes mono-spaced font
  tft.setFont(myFonts[xp->font]);
  tft.setTextSize(xp->textSize);
  if (fp >= 0) // GFX font
  {
    tft.getTextBounds("MMMMMMMMMM", 0,0, &xdum, &ydum, &cwid, &cht); // only use cwid, cht
    cwid = cwid / 10 + 1; // allow for the additional space between characters
	cht++;	// a bit more height
  }
  else // GLCD font
  {
    cwid = 5 * xp->textSize; // 8 or 7?
    cht = 8 * xp->textSize;
  }
  // assumes  fmt in "%[pre].[post]f"

//Serial.print("Decimal ");Serial.println(decimal);
  // shift to array index value and skip "."
  if (highlight <= pre)
    highlight--;
    
  // wipe out previous text: n characters + decimal (BG highlight for value being edited)
  //tft.fillRect(x - BORDER_HL, y - cht - BORDER_HL , (pre+post+2)* cwid + BORDER_HL * 2, cht + BORDER_HL * 2, bgcol ); 
  
  tft.setCursor(x, y);
  // ignore decimals for anything but Float format
  short nChars = (fmt == 'F')?(pre + post + 1) : pre;
     //Serial.print(fmt);Serial.print(" nChars ");Serial.println(nChars);
  short blocklen = pre + post;
  switch (fmt){
    case 'T' : // Toggle
      tft.fillRect(x  + cwid * cCount, y - cht - BORDER_HL , cwid*3 , cht + BORDER_HL * 2, bgcol); //bgcol
	  tft.setTextColor(txtcol);
      tft.print((*(uint8_t*)(xp->vp) > 0)? "On " : "Off");
      return;
	case 'Q' :  // boolean print the first char in v2 string for T second for F
	  i = (*((bool*)(xp->vp))) ? 0 : 1;
//	  Serial.printf("X: %i %i %i %i %i\n", i, pSetX.trackSv, pSetX.trackSa, vTrack, cTrack);
      tft.setTextColor(txtcol);
	  tft.print((char*)(xp->fmt + i));
      tft.print(" "); // rubbish erase!!!
      return;
   // use SAFE 'A' format for all string variables
   case 'A' : // SAFE string text (may have characters outside 0x20 .. 0x7e range)
      strcpy(buf, (char*)(xp->vp)); // substitute unprintable chars
       //Serial.println(buf);
      for (i = 0; i < strlen(buf); i++)
        if (buf[i] < 0x20 || buf[i] > 0x7d)
          buf[i] = '*';
      //Serial.println(buf);
      //delay(500);
	  tft.setTextColor(txtcol);
      tft.print(buf);
      tft.print(" "); // rubbish erase!!!
      return;
    case 'S' :  // unvalidated string - really only for const strings
      tft.setTextColor(txtcol);
	  tft.print((char*)(xp->vp));
      tft.print(" "); // rubbish erase!!!
      return;
    case 'B': // Bracketed 
      tft.setTextColor(txtcol);
	  tft.print("[");
      cCount++;
    case 'I': // 16-bit  Integer: convert value to float for display)
      val = *((int16_t*)(xp->vp));      	  
  	  if (val > max + MEDIUM_DIFF)
  	  {
  	    overflow = true;
  	  //  Serial.printf("prinfX I overflow: V %1.1f, M %1.1f\n",val, max) ;
  	  }
      break;
	  case 'U': // 16-bit unsigned Integer: convert value to float for display
      val = *((uint16_t*)(xp->vp));      
      if (val > max + MEDIUM_DIFF)
      {
        overflow = true;
      //  Serial.printf("prinfX U overflow: V %1.1f, M %1.1f\n",val, max) ;
      }
      break;
   case 'L': // 32-bit unsigned Integer: convert value to float for display         
      val = *((uint32_t*)(xp->vp));  
      if (val > max + MEDIUM_DIFF)
      {
        overflow = true;
        //Serial.printf("prinfX L overflow: V %1.1f, M %1.1f\n",val, max) ;
      }
      break; 
    case 'F': // Float
      val = *((float*)(xp->vp));
	  blocklen++; // decimal point
	  
	  if (val > max)
	  {
		overflow = true;
		//Serial.printf("prinfX F overflow: V %3.2f, M %3.2f\n",val, max) ;
	  }
	 // Serial.printf("PFX: %6.4f [round %i]\n", val, post);
	  val = myRound(val, post); // round to appropriate number of digits for display (stop the 2.99 .. 3.00 flashing issue	  
      break;
    case 'P': // Float as percentage
      val = *((float*)(xp->vp)) * 100;
	  //Serial.printf(" %5.1f%\n", val);
      break;
  }
  // sign is to left of defined display block (only if min value < 0)
  if(min < 0)
   tft.fillRect(x-cwid, y - cht - BORDER_HL , cwid , cht + BORDER_HL * 2, bgcol); // erase block. sign and decimal extras. bgcol
 // else
	// tft.fillRect(x, y - cht - BORDER_HL , cwid * (blocklen) , cht + BORDER_HL * 2, RED); // erase block. decimal only.
 
  if (overflow)	// blank space, print ***
  {	 
	  tft.fillRect(x-cwid, y - cht - BORDER_HL , (cwid * blocklen + 1), cht + BORDER_HL * 2, bgcol);
	  tft.setTextColor(txtcol);
	  tft.print("***");
	  return;
  }
 

 // tft.fillRect(x - cwid, y - cht - BORDER_HL , cwid , cht + BORDER_HL * 2, bgcol); // erase sign block
  //cCount++;
  if (val < 0) // sign
  {
	 val = -val;
	 tft.setCursor(x - cwid, y);	 
	 // tft.fillRect(x  + cwid * cCount, y - cht - BORDER_HL , cwid , cht + BORDER_HL * 2, bgcol); //erase extra block
	 tft.setTextColor(txtcol);
     tft.print("-");
	 tft.setCursor(x, y);
	 // nChars--; // reduce the post decimal width by 1
  }
  
//if(numtest)  Serial.printf("\nPFX %5.3f \n", val);
	// print the value one character at a time, highlight digit if indicated
  for (short i = 0; i < nChars ; i++)
  {
	// ** Because this takes a long time, check for interrupt post-processing between characters 

	
	// y is baseline, erase block as each char is redrawn
    if(powVal == -1) //decimal point precedes numeral
    {
      tft.setTextColor(txtcol);
	  tft.fillRect(x  + cwid * cCount, y - cht - BORDER_HL , cwid , cht + BORDER_HL * 2, bgcol); //erase decimal point block  
      tft.print(".");
      cCount++;
	  i++; // added in an extra char for floats
      printedDot = true; // no spaces after decimal
    }
	if(cCount < nChars) // allow for decimal
		tft.fillRect(x  + cwid * cCount, y - cht - BORDER_HL , cwid , cht + BORDER_HL * 2, bgcol); // erase block 
	
    if (powVal == highlight+1) // highlighted digit
      tft.setTextColor(highcol);
    else
      tft.setTextColor(txtcol);

    // from most signif digit; extract the digit; subtract its value from the number (i.e. calc remainder)
    pownum = pow(10, powVal);
//if(numtest) Serial.printf("PN %6.3f | ", pownum);
    tempVal = val/pownum; // myRound(pownum, -powVal);
//if(numtest) Serial.printf("TV %8.5f | ", tempVal);
//if(numtest && powVal == -post) Serial.print("*");
	//if(i == nChars -1)
	   //digit =  myRound(tempVal + SMALL_DIFF,1); //  truncate to single digit integer (round **final** digit to one decimal place to avoid trunc issue)
    //else
  	digit = tempVal + VSMALL_DIFF;  // float to integer truncate, avoid rounding errors 
    //if (digit < 0)        digit = 0;
    val = val - digit * pownum;    
//if(numtest) Serial.printf("D[%i] %i | RV %6.3f\n", powVal,digit, val);
  
    // print the digit, or a leading space
    itoa(digit, buf, 10);
    // print leading spaces when leading zeros are not required.
    // leading zeros for Bracketed format 
    if(digit == 0 && !leadZero && !printedDot && fmt != 'B' && !editing)
    {
       if (powVal == 0) // single zero before decimal
	   {
        tft.print(buf);
		//if(numtest) Serial.printf("[%s]", buf);
	   }
       else
	   {
		//   if(numtest) Serial.print("[ ]");
        tft.print(" ");
	   }
    }
    else 
    {
      tft.print(buf);
	  //if(numtest) Serial.printf("[%s]", buf);
      printedDot = true;
    }
    powVal--;
    cCount++;
  } // end character loop

  if(strlen(xp->units)) // text post-nominal (usually unit type, e.g. mA)
  {
	// TextSize and extended character set only work for inbuilt font.
	if (fp > 0) 
	  tft.setFont(myFonts[fp -1]);	
	//tft.print (' '); 
	tft.print(xp->units);
	tft.setFont(myFonts[xp->font]);
  }

  if (fmt == 'P')
  {
    tft.print("%");
  }

  if (fmt == 'B')
    tft.print("]");
}  

#define HLEFT_E (HMAX/10)
#define WIDTH_E (HMAX*4/5)
#define VTOP_E (VMAX/10)
#define HEIGHT_E (VMAX*4/5)
#define LINESP_E 24
#define TEXTSIZ_E 2
#define BORDER_E 2
#define MARGIN_E 10
#define ROUND_E 10
// onTime in seconds
#include "platyImg2.h"

void waitOne(void) // mS
{
    feedLoopWDT();  // Just in case
    vTaskDelay(1);  // hand control to FreeRTOS
    yield();
}
// onTime < 0 == wait for screen touch
void screenError(const char * message, uint16_t bgcol, int16_t onTime, bool logo)
{    
#ifdef NO_ERRORS
	return;
#endif
    char scrBuf[128];
   strcpy(scrBuf, message);  
   if(onTime < 0)
   {
     strcat(scrBuf, "\n\nWaiting for screen touch...");
   }
	 int i, ycursr = VTOP_E + LINESP_E;
   tft.setRotation(sc.scrRot);
	 tft.fillRoundRect(HLEFT_E, VTOP_E, WIDTH_E, HEIGHT_E, ROUND_E, bgcol);
	 for(i = 0; i < BORDER_E; i++)
		tft.drawRoundRect(HLEFT_E + i, VTOP_E + i, WIDTH_E - 2 * i, HEIGHT_E - 2 * i, ROUND_E, HIGHLIGHTCOL);
	 tft.setTextColor(HIGHLIGHTCOL);
	 tft.setFont(&FONT1); 
	 //tft.setTextSize(TEXTSIZ_E);
	 tft.setCursor(HLEFT_E + MARGIN_E, ycursr );
	 for (i = 0; i < strlen(scrBuf); i++)
	 {
		 if (scrBuf[i] == '\n' || scrBuf[i] == '\r' )
		 { 
			ycursr += LINESP_E;
			tft.setCursor(HLEFT_E + MARGIN_E, ycursr);
		 } else
			 tft.print(scrBuf[i]);
	 }
	 if(logo) // 87w x 64h pixels
#ifdef ILI9488 // needs FG colour as well as BG
		tft.drawBitmap(HMAX/2-96/2, VTOP_E + HEIGHT_E - 96 - MARGIN_E, platy_img2, 128, 96, bgcol, MY_WHITE);
#else
		tft.drawBitmap(HMAX/2-87/2, VTOP_E + HEIGHT_E - 64 - MARGIN_E, platy_img2, 128, 96, BGCOL);
#endif
int cntr = 0;
   if(onTime <= 0) // display message and wait
   {      
      holdScreen = true;
      //while(!processTouchSwEnc(true)) // anyTouch _scrTouched
      while(!_scrTouched)
      {
         waitOne();
         _scrTouched = processTouchSwEnc(true);
        // if(cntr++ > 500) {Serial.print("."); cntr = 0;}
      }
      holdScreen = false;
    }

    else    
  	 for (i = 0; i < onTime * 1000; i++) 
  	 {
  		 waitOne();
  	 }
	 dirtyScreen = true;
}
void splashScreen(void)
{
	char buf[256];
	sprintf(buf, "WiFi Programmable\nDC Load\nModel %s\nVersion %i\n", MODEL, SOFT_VERSION);
#ifdef ILI9341
	screenError(buf, ERR_BG_A ,5, false); // no bitmap for 2.8"
#else
		screenError(buf, ERR_BG_A ,5, true);
#endif
}
#ifdef ILI9341
void ili9341Diag(void)
{
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
} 
#endif
#endif
