#ifndef  LCVTHAL_H
#define  LCVTHAL_H


// Input readings are converted in two stages: counts to Volts; then Volts to real world values
// HAL (conversion device) counts to Volts 
float convertHAL(int reading, hal_cal * hp){
// convert reading from HAL to volts
// assumes minimum reading is zero counts
float volts;
int maxCount;
maxCount = (1 << hp->ADCbits) -1;
  
  if (hp->threePoint){ // dual slope approximation
    if (reading <= hp->midCount)
      volts = hp->minVolts + (hp->midVolts - hp->minVolts) * reading / hp->midCount;
    else
      volts = hp->midVolts + (hp->maxVolts - hp->midVolts) * (reading - hp->midCount) / (maxCount - hp->midCount);
  }
  else {
    volts = hp->minVolts + (hp->maxVolts - hp->minVolts) * reading / maxCount;
  }
  return volts;  
}

// convert Volts read to the real world value that represents
/////////////// should be in myHelpers.h????
float convertADC(float reading, inADC * ap){
// convert reading from volts to real world value
float val;
  //Serial.print("CvtADC mV = "); Serial.print(ap->maxVolts);
  if (ap->threePoint) // dual slope approximation
  { 
    if (reading <= ap->maxVolts)
      val = ap->minVal + (ap->midVal - ap->minVal) * (reading - ap->minVolts) / (ap->midVolts - ap->minVolts);
    else
      val = ap->midVal + (ap->maxVal - ap->midVal) * (reading - ap->midVolts) / (ap->maxVolts - ap->midVolts);
  }
  else { // two points
    val = ap->minVal + (ap->maxVal - ap->minVal) * (reading - ap->minVolts) / (ap->maxVolts - ap->minVolts);
  }
  //Serial.print("CvtADC RW = "); Serial.print(val); Serial.println(ap->units);
 // Serial.printf("CVT [%s]: %f - %f\n", ap->iname, reading, ap->minVolts);
  return val;  
}

#endif // CVTHAL_H
