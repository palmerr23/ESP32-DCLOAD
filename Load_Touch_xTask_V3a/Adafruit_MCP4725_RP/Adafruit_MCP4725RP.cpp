/**************************************************************************/
/*!
    @file     Adafruit_MCP4725.cpp
    @author   K.Townsend (Adafruit Industries)

        I2C Driver for Microchip's MCP4725 I2C DAC

        This is a library for the Adafruit MCP4725 breakout
        ----> http://www.adafruit.com/products/935

        Adafruit invests time and resources providing this open source code,
        please support Adafruit and open-source hardware by purchasing
        products from Adafruit!

        @section  HISTORY

*/
/**************************************************************************/

#include "Adafruit_MCP4725RP.h"

/**************************************************************************/
/*!
    @brief  Instantiates a new MCP4725 class
*/
/**************************************************************************/
Adafruit_MCP4725::Adafruit_MCP4725() {}

/**************************************************************************/
/*!
    @brief  Setups the hardware and checks the DAC was found
    @param i2c_address The I2C address of the DAC, defaults to 0x62
    @param wire The I2C TwoWire object to use, defaults to &Wire
    @returns True if DAC was found on the I2C address.
*/
/**************************************************************************/
bool Adafruit_MCP4725::begin(uint8_t i2c_address, TwoWire *wire) {
  if (i2c_dev) {
    delete i2c_dev;
  }

  i2c_dev = new Adafruit_I2CDevice(i2c_address, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  return true;
}

/**************************************************************************/
/*!
    @brief  Sets the output voltage to a fraction of source vref.  (Value
            can be 0..4095)

    @param[in]  output
                The 12-bit value representing the relationship between
                the DAC's input voltage and its output voltage.
    @param[in]  writeEEPROM
                If this value is true, 'output' will also be written
                to the MCP4725's internal non-volatile memory, meaning
                that the DAC will retain the current voltage output
                after power-down or reset.
  
    @returns True if able to write the value over I2C
*/
/**************************************************************************/
bool Adafruit_MCP4725::setVoltage(uint16_t output, bool writeEEPROM) {
  //i2c_dev->setSpeed(i2c_frequency); // Set I2C frequency to desired speed

  uint8_t packet[3];
  output = output & MCP4725_DAC_MASK;	// 12 bits only

  if (writeEEPROM) {
    packet[0] = MCP4725_CMD_WRITEDACEEPROM;
  } else {
    packet[0] = MCP4725_CMD_WRITEDAC;
  }
  packet[1] = output >> 4;        // Upper data bits (D11.D10.D9.D8.D7.D6.D5.D4)
  packet[2] = (output % 16) << 4; // Lower data bits (D3.D2.D1.D0.x.x.x.x)

  if (!i2c_dev->write(packet, 3)) {
    return false;
  }

 // i2c_dev->setSpeed(100000); // reset to arduino default
  return true;
}

uint16_t Adafruit_MCP4725::getRegValue(int reg2Read)
{
	uint8_t  inbuf[5], stat, statE;
	uint16_t dac, dacE;

	i2c_dev->read(inbuf, 5);   
	stat = inbuf[0];	
	dac = ((uint16_t)inbuf[1] << 4 ) | (inbuf[2] >> 4); // top 8 then bottom 4
	statE = inbuf[3] >> 4;					 // shift down to match status byte PD bits
	dacE = (((uint16_t)inbuf[3] << 4 ) | inbuf[4]) & 0x0FFF; // top 8 then bottom 4
	//Serial.printf("DAC: stat 0x%2x, dac %i, statE 0x%2x, dacE %i\n",stat, dac, statE, dacE);
	switch (reg2Read)
	{
		case MCP4725_READ_STAT :
			return stat;	
		case MCP4725_READ_DAC:
			return dac;
		case MCP4725_READ_EEPROM_STAT :
			return statE;	
		case MCP4725_READ_EEPROM :
			return dacE; 
		default :
			return 0;
	}
}
