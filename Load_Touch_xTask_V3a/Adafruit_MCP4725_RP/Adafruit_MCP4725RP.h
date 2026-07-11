/**************************************************************************/
/*!
    @file     Adafruit_MCP4725RP.h
		
		RP: 2021
		Added getRegValue()
		Added #defines
*/
/**************************************************************************/

#ifndef _ADAFRUIT_MCP4725RP_H_
#define _ADAFRUIT_MCP4725RP_H_

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Wire.h>

#define MCP4725_DAC_MASK			(0xfff)
#define MCP4725_I2CADDR_DEFAULT 	(0x62) ///< Default i2c address
#define MCP4725_CMD_WRITEDAC 		(0x40) ///< Writes data to the DAC
#define MCP4725_CMD_WRITEDACEEPROM  (0x60) ///< Writes data to the DAC and the EEPROM (persisting the assigned value after reset)
#define MCP4725_STATUS_RDY			(0x80)	// status read masks									    
#define MCP4725_STATUS_POR			(0x40)	
#define MCP4725_STATUS_PD			(0x06)	


#define MCP4725_READ_DAC			0
#define MCP4725_READ_STAT			1
#define MCP4725_READ_EEPROM			2
#define MCP4725_READ_EEPROM_STAT			3
/**************************************************************************/
/*!
    @brief  Class for communicating with an MCP4725 DAC
*/
/**************************************************************************/
class Adafruit_MCP4725 {
public:
  Adafruit_MCP4725();
  bool begin(uint8_t i2c_address = MCP4725_I2CADDR_DEFAULT,
             TwoWire *wire = &Wire);
  bool setVoltage(uint16_t output, bool writeEEPROM);

  uint16_t getRegValue(int reg2read = MCP4725_READ_DAC);
private:
  Adafruit_I2CDevice *i2c_dev = NULL;
};

#endif
