/*  basic DC Load test
 *  I2C scan
 *  DAC steps
 *  ESP32 and ADS1115 ADC readings
*/
#define DACADDR 0x60
#define ADSADDR 0x48
#define I2CSPEED   1000000 // 1MHz 
#define DACMAX 4095
#define ADSMAX 32767
#define ONPIN 4
#define VPIN  34
#define IPIN  35
#define TPIN  32

#include <Wire.h>
#include "Adafruit_MCP4725.h"
Adafruit_MCP4725 dac;

#include"myLADS1115.h"
Adafruit_ADS1115 ads(ADSADDR);

void setup() {      
  Serial.begin (115200);
  while (!Serial) 
    ;
  delay(2000);
  Serial.println("DAC test - DC load");
  pinMode(ONPIN, OUTPUT);
  Wire.begin(); 
  Wire.setClock(I2CSPEED);
  I2Cscan();
  dac.begin(DACADDR); // use standard Wire
  dac.setVoltage(0, false, I2CSPEED); // safe start current
  ads.begin(I2CSPEED);
  digitalWrite(ONPIN, HIGH);
  Serial.println ("Setup complete");
}  

void loop() 
{
  DACtest(0, 170, 5, 2000);  // 170 ~= 1A with both modules connected (0.5A without daughter board)
  delay(5000);
}

// there will be no current sunk at very low DAC counts(by design)
void DACtest(int minS, int maxS, int steps, int delayMs)
{  
  int   stepS = (maxS - minS)/steps;
  for(int i = 0; i <= steps; i++)
  {
    dac.setVoltage(minS + i * stepS, false, I2CSPEED);
    Serial.printf("%i: DAC %i\n", i, minS + i * stepS);
    delay(100); // allow some settling time
    readADC();
    delay(delayMs);
  }
  dac.setVoltage(0, false, I2CSPEED); // back to a safe output value between tests 
  Serial.println("Test complete\n");
}

int I2Cscan(void)
{     
  Serial.println("I2C scanner: ");
  int count = 0;
  for (uint8_t i = 1; i < 127; i++) // 8 to 120 are legal
  {       
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
      {
      Serial.print ("- Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");      
      count++; 
      }  
      delayMicroseconds(50);     
  } 
  Serial.print ("Found ");
  Serial.print (count, DEC);
  Serial.println (" device(s)\n");  
return count;
}

// Read ESP32 and ADS1115 ADCs and print results
// These are raw counts, not real-world values
void readADC(void)
{
  int16_t adsV, adsI, espV, espI, espT;
  adsV = ads.readADC_Differential_0_1();
  adsI = ads.readADC_Differential_2_3();
  espV = analogRead(VPIN); // hardware offset by 0.6V
  espI = analogRead(IPIN); // hardware offset by 0.6V
  espT = analogRead(TPIN);
  Serial.printf("ADS: V = %5i, I = %5i\nESP: V = %5i, I = %5i, T = %5i\n\n", adsV, adsI, espV, espI, espT);  
}
  
