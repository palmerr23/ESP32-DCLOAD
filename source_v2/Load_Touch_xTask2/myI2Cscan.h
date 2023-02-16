// myI2Cscan.h
#ifndef MYI2CSCAN_H
#define MYI2CSCAN_H
int i2cScan() {
  int count = 0;        
  Serial.print ("I2C scanner:\n");
 
  for (uint8_t i = 1; i < 127; i++) // 8 to 120 are legal
  {
    Wire.beginTransmission (i);
   //Wire.write(0x10);
    if (Wire.endTransmission () == 0)
      {
      Serial.print ("- Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
          count++; 
      } // end of good response  
      //delayMicroseconds(10);      // nice and slow
  } 
  Serial.print ("Found ");
  Serial.print (count, DEC);
  Serial.println (" device(s)\n");
  return count;
}
#endif