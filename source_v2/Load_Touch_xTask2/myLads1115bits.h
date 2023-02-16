/* 
 * ADS1115bits - ISR that can't go into the library
 */
 #ifndef __LADSB1115_H__
#define __LADSB1115_H__
#include "myLADS1115.h"
float ADStoV(int ADCval);
float ADStoA(int ADCval);

// outside the object definition (ISR & child routines can't be in object)
volatile long adsISRcount = 0;
volatile bool adsReady = false, adsSetready = false;
long adSProcessCount = 0;
uint8_t adsMux = ADSMUX_START;
#define NUMADC 2   // ADS differeential mode
#define NUMESPADC 3 // single ended 
//int16_t espReadings[NUMESPADC];
uint8_t espPins [NUMESPADC] = {ADC_BV, ADC_BI, ADC_BT};
int16_t adsReadings[NUMADC];

const uint16_t adsConfig =
      ADS1015_REG_CONFIG_CQUE_1CONV | // Assert ALERT/RDY after one conversions
      ADS1015_REG_CONFIG_CPOL_ACTVLOW |       // Alert/Rdy active low   (default val)
      ADS_RATE |                              // sample rate selected above
      ADS1015_REG_CONFIG_MODE_SINGLE;         // Single-shot mode (default)
      //ADS1015_REG_CONFIG_CLAT_LATCH |  //  Latching comparator
      //ADS1015_REG_CONFIG_CMODE_STAD |   // Traditional comparator (default val)
      //ADS1015_REG_CONFIG_OS_SINGLE ;   // single shot start      
      // ADS1115_REG_CONFIG_MUX_SE |      // single ended conversion
      // ADS1015_REG_CONFIG_PGA_4_096V;   // PGA GAIN = 1 

// interrupt code in IRAM to avoid kernel panics
void IRAM_ATTR ads_ISR(){  
  portENTER_CRITICAL_ISR(&mux);
    adsReady = true;
  portEXIT_CRITICAL_ISR(&mux);
  //adsISRcount++; 
}

//long adsWdt; // ADS watchdog
void adsProcess(void){
  if (adsReady)
  {	// convert latest reading to "counts read" and "read-world" values
    if(adsMux == 1) // flag a complete set of readings
	{
	  adSProcessCount++;  // updated each 4 cycles = 16mS @ 250 sps
	  adsSetready = true; // complete set of readings has been processed
	}	  
		// conversions now done just before control() 
	  switch (adsMux)
	  {
		case (ADSMUXV):
	      ADCcount.ADSv = ads.getLastConversionResults();
	      meas.ADSvolts = ADStoV(ADCcount.ADSv);
		  if (!_outOn) // no load voltage used in estimating DUT ESR in CV mode
		  {
			_lastVolts = meas.ADSvolts;
			_lastAmps = 0.0;
		  }
	      break;
	    case (ADSMUXI):
	      ADCcount.ADSa = ads.getLastConversionResults();
	      meas.ADSamps = ADStoA(ADCcount.ADSa);
		  break;
	  }
						   
    // start next reading
	adsMux = ++adsMux % NUMADC;
	// don't bother about turning interrupts off - single shot mode
    ads.adsStartSingleDiffRdy(adsConfig | ADS_GAIN, adsMux); 
    adsReady = false; 
	//adsWdt = millis();
	//Serial.print("^");
  }
  /*
  else
	if (millis() - adsWdt > ADSWDT)	// watchdog timeout 
	{
	   Serial.printf("ADS watchdog timer - restarting %i mS\n",millis() - adsWdt);
	   ads.adsStartSingleDiffRdy(adsConfig | ADS_GAIN, adsMux); // any channel will do, sorted out next cycle
	   adsWdt = millis();
	}
	*/
}

void adsTest()
{
 // start ADS1115 ADC
 int rdg;
  ads.setGain(ADS_GAIN);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  pinMode(ADSRDY, INPUT_PULLUP);
  ads.begin(400000);  // I2C speed, 1000000 (1M = FMPlus) is probably OK, 400000 is safe
  Serial.println("ADS BEGIN done");
  Wire.beginTransmission (ADS_ADDR);
	if (Wire.endTransmission () == 0)
		Serial.printf("Found ADS1115 at 0x%x\n", ADS_ADDR);
	else
		Serial.printf("ADS1115 not found at 0x%x\n", ADS_ADDR);
  //  delay(100);
  //attachInterrupt(digitalPinToInterrupt(ADSRDY), ads_ISR, FALLING);
 // Serial.println("ATTACH int done");
 delay(1000);
  while(1)
  {
	  for (int i = 0; i < 4; i++)
	  {
		//rdg = ads.readADC_SingleEnded(i);		
		ads.adsStartSingleRdy(adsConfig,i); // no wait, needs suitable delay before reading
		delay(2);
		rdg = ads.getLastConversionResults();	
		Serial.printf("ADS ch[%i] = %i\n", i, rdg);
	  }
	  Serial.println();
	  delay(40);
  }
}
void printADSconfig(void)
{
	uint16_t rreg = ads.readReg(ADS_ADDR,1);
	Serial.printf("\nADS config: MUX 0x%01x, PGA 0x%01x, Mode 0x%01x, SPS 0x%1x, Modes 0x%02x\n", (rreg >> 12) & 0x7, (rreg >> 9) & 0x7, (rreg >> 8) & 0x1, (rreg >> 5) & 0x7, (rreg) & 0x1f);
	  
}
#endif