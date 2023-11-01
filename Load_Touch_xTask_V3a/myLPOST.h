#ifndef  MYLPOST_H
#define  MYLPOST_H
#define STR(x) STR_IMPL_(x)
#include "myI2Cscan.h"
bool i2cTest(uint8_t i2cAddress);
int probeDAC(uint8_t addrbase, uint8_t span);

char postBuf[128];

// ******************** tests single channel ONLY 
bool post(void){
	char tempS[32];
	int postErrs = 0;
	int temp;
	strcpy(postBuf, "POST:\n");
	// fill postBuf[] if any test fails, then return false
	
	// test for I2C devices
	Wire.begin();	
	i2cScan();
	// ADS1115
	if (!i2cTest(ADS_ADDR))
	{
		strcat(postBuf, "No " );
		postErrs++;
	}
	strcat(postBuf, "ADC at 0x");
	strcat(postBuf, itoa(ADS_ADDR, tempS, 16));
	strcat(postBuf, "\n");
		
	//MCP4725 DAC (scan range)
	temp = probeDAC(DAC_BASE,DAC_SCAN);
	if (temp < 0)
	{
		strcat(postBuf, "No DAC at 0x");
		strcat(postBuf, itoa(DAC_BASE, tempS, 16));
		strcat(postBuf, " - 0x");
		strcat(postBuf, itoa(DAC_BASE+DAC_SCAN-1, tempS, 16));
		strcat(postBuf, "\n");
		postErrs++;
	} else
	{
		strcat(postBuf, "DAC at 0x");
		strcat(postBuf, itoa(temp, tempS, 16));
		strcat(postBuf, "\n");
	}
	
	// screen failure will be obvious
	
	if (postErrs > 0)
	{
		Serial.printf("\n%s", postBuf);
		return false;
	}
	Serial.println("Passed POST");
	strcat(postBuf, "Passed POST\n");
	return true;
}

bool i2cTest(uint8_t i2cAddress){		
	Wire.beginTransmission (i2cAddress);
	if (Wire.endTransmission () == 0)
		return true; 
	return false;
}
int probeDAC(uint8_t addrbase, uint8_t span)
{
	for(int i =0; i < span; i++)
		if(i2cTest(addrbase+i))
		{
			//Serial.printf("Found DAC at 0x%2x\n",addrbase+i);
			return (addrbase+i);
		}
	return -1;
	
}
#endif
