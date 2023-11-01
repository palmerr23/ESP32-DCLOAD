#ifndef LHELPERS_H
#define LHELPERS_H


// SIGN function
#define SIGN(X) (((X) < 0) ? -1 : 1)
#define bitInvert(value, bit) ((value) ^= (1UL << (bit)))
#define isBitSet(value, bit) ((value) & (1UL << (bit)))

// ---------------------------------------------------------------------------- 
// crc8
//
// Copyright (c) 2002 Colin O'Flynn
// Minor changes by M.Thomas 9/2004 
// ----------------------------------------------------------------------------

#define CRC8INIT    0x00
#define CRC8POLY    0x18              //0X18 = X^8+X^5+X^4+X^0

// ----------------------------------------------------------------------------

uint8_t crc8(uint8_t *data, uint16_t data_length) {
  uint8_t  b;
  uint8_t  bit_counter;
  uint8_t  crc = CRC8INIT;
  uint8_t  feedback_bit;
  uint16_t loop_count;
  
  for (loop_count = 0; loop_count != data_length; loop_count++) {
    b = data[loop_count];
    bit_counter = 8;

    do {
      feedback_bit = (crc ^ b) & 0x01;

      if (feedback_bit == 0x01) {
        crc = crc ^ CRC8POLY;
      }

      crc = (crc >> 1) & 0x7F;

      if (feedback_bit == 0x01) {
        crc = crc | 0x80;
      }

      b = b >> 1;
      bit_counter--;

    } while (bit_counter > 0);
  }
  
  return crc;
}
#define VAL_CHGD -1
#define VAL_CHGD_NOEE 2
#define VAL_CHGD_REMOTE 3
void valChanged(int8_t cmd){
	//Serial.print("V");
	switch (cmd) 
	{
		case VAL_CHGD:
			changedVal_scrn = true;
			changedVal_EE = true;
			//changedVal_remote = true; // not sure if this is needed
			break;
		case VAL_CHGD_NOEE:		// all remote settings need saving
			changedVal_scrn = true;
			//changedVal_remote = false; // not sure if this is needed
			break;
		case VAL_CHGD_REMOTE:
			//changedVal_scrn = true; 
			changedVal_EE = true;
			changedVal_remote = true; // not sure if this is needed
			break;

		default:
		break;
	}	
	dynSet = pSet; // update live control set
}
IPAddress cidr2netmask(uint8_t cidr){
	uint8_t temp;
	IPAddress netmask = (IPAddress) 0xffffffff << (32 - cidr); // in reverse byte order
	// reverse byte order
	temp = netmask[3];
	netmask[3] = netmask[0];
	netmask[0] = temp;
	
	temp = netmask[2];
	netmask[2] = netmask[1];
	netmask[1] = temp;
	
	return netmask;
}
float myRound(float num, int digits)
{	
	return round(num * pow(10, digits)) / pow(10, digits);
}

#endif // HELPERS_H
