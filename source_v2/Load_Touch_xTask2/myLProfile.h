/* Profile storage 

EEPROM VERSION

myID comms params overwritten from profile.json 

*/
#ifndef MYLPROFILE_H
#define MYLPROFILE_H
//#define PROF_TEST	// comment out for production - writes then reads back into dummy structures, compares

//#define EE_VER_L 6  // SIZE OF THE BLOB
//#define EE_VER_MAJOR 2 // number of digits at the start defining a major version (incompatible data structures if different)
//#define EE_MODEL 12
//#define EE_CRC_L 1   
bool checkProfileHdr(void);
int factory_reset(void);
int writeEE(void);
int readEE(void);
bool setComms(bool create);

struct profile {
  uint8_t crc;           // must be first in struct
  uint8_t softVer; // always after crc, length does not vary between versions
  char model[EE_MODEL];
  uint8_t hardVer; // always after crc, length does not vary between versions
  // pad out to ??? so the sizeof() is correct.
} ;
profile myProfile = {0, SOFT_VERSION, MODEL, HARD_VERSION};
struct profile eeProfile;	// read EE into here to check against software version, etc

// store CRC, HAL, CAL, IS, Settings in that order)
#define MPSIZE (sizeof(myProfile))
#define ADSSIZE (sizeof(halCal))
//#define ADCSIZE (sizeof(myADC))
#define IDSIZE 		(sizeof(myID))
#define SCALSIZE 	(sizeof(sc))
#define SETSIZE 	(sizeof(pSet))
#define WIFISIZE	(sizeof(myWiFi))
#define EESIZE (MPSIZE + ADSSIZE + IDSIZE + SCALSIZE + SETSIZE + WIFISIZE)

bool SPIFFSstart()
{
  if(SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    //Serial.println("SPIFFS Mounted"); 
    // listDir(SPIFFS, "/", 4);
     return true;
  } 
  else
    Serial.println("SPIFFS Mount Failed");
  return false;
}  

int EEcntr = 0;
bool saveSoon = false;
// called every VL from main loop
void saveSettings(void)
{	
	if(saveSoon)
			EEcntr++;
		
	if(changedVal_EE) // detect and reset change
	{
		saveSoon = true;
		//Serial.println("*** EE Changed");
		EEcntr = 0;	// new change? reset counter
		changedVal_EE = false;
	}

	if(EEcntr > SAVE_EE_AFTER)
	{
		EEcntr = 0;
		//Serial.print("*** Saving settings\n");
		setComms(false);
		writeEE();
		changedVal_EE = false;
		saveSoon = false;
	}		
}
bool beginEE() {
  int addr = 0, rb;

  //Serial.printf("Reading EEPROM Profile, %i bytes\n", EESIZE);  
  
  EEPROM.begin(EESIZE + 5 ); // maybe a few additional bytes will stop the occasional kernel panic restart
  
  addr = 0;
  // read Profile into a temp structure, and compare with software defaults
  rb = EEPROM.readBytes(addr, (void *)&eeProfile, sizeof(eeProfile));
    if (checkProfileHdr() == false) 
    {
		Serial.printf("Read %i EE bytes. Incompatible stored profile - factory reset triggered.\n", rb);
		
		factory_reset();
		EEPROM.readBytes(0, (void *)&eeProfile, sizeof(eeProfile));
		if (checkProfileHdr() == false) 
		{
			 Serial.println("Factory reset failed; aborting");
			 return false; // should not allow instrument to start.
		}
	}
  rb = readEE();  
  //Serial.printf("EE read OK [%i]\n", rb);
  dirtyScreen = true;
  return true;
}

int factory_reset(void){
	int bw;
   //  write to EEPROM: Profile, HAL, calibration and settings - already intitalised to defaults   
	   bw = writeEE();
   printf("Written %i bytes to EEPROM\n",bw );
   return bw;
}

int writeEE(void){
	int bytesWrit = 0, addr = 0;

	myProfile.crc = crc8((uint8_t *)&myProfile.softVer, sizeof(myProfile) - 1);
	//Serial.printf("Writing Profile, crc = %i\n", myProfile.crc);
	
	addr = 0;
 	bytesWrit =  EEPROM.writeBytes(addr, (void *)&myProfile, sizeof(myProfile));
	//Serial.printf("  Profile [%i]\n", sizeof(myProfile));		
	addr += MPSIZE;	
	
	bytesWrit += EEPROM.writeBytes(addr, (void *)&halCal, sizeof(halCal));
	//Serial.printf("  HAL [%i]\n", sizeof(halCal));	
	addr += ADSSIZE;
/*
	bytesWrit += EEPROM.writeBytes(addr, (void *)&myADC, sizeof(myADC));
	//Serial.printf("  ADC [%i]\n", sizeof(myADC));	
	addr += ADCSIZE;
		*/
	bytesWrit += EEPROM.writeBytes(addr, (void *)&myID, sizeof(myID));
	//Serial.printf("  ID [%i]\n", sizeof(myID));		
	addr += IDSIZE;

	bytesWrit += EEPROM.writeBytes(addr, (void *)&sc, sizeof(sc));
	//Serial.printf("  Screen cal [%i]\n", sizeof(sc));		
	addr += SCALSIZE;
	
	bytesWrit += EEPROM.writeBytes(addr, (void *)&pSet, sizeof(pSet));
	addr += SETSIZE;
	
	bytesWrit += EEPROM.writeBytes(addr, (void *)&myWiFi, sizeof(myWiFi));
	//Serial.printf("  Settings [%i]\n", sizeof(pSet));	
	//Serial.printf("Total bytes to be written = %i\n",addr + sizeof(pSet) );
	
	// try to avoid watchdog timer kernel panic and reboot - none of these seem to work!
	yield(); 
	long gap;
	gap = millis();
	disableLoopWDT(); //ESP.wdtDisable(); 
	//delay(1);
	//portENTER_CRITICAL(&mux); 
		EEPROM.commit();
	//portEXIT_CRITICAL(&mux);
	feedLoopWDT();	// Just in case
	enableLoopWDT(); // ESP.wdtEnable(WDTO_8S); 
	gap -= millis();
	//Serial.printf("EEPROM save took %ld mS\n", gap);
	yield(); 	

	//Serial.println("Written EEPROM");

	return  bytesWrit;
}
int readEE(void){
	int bytesRead = 0, addr;
	
	addr = 0;
	bytesRead = EEPROM.readBytes(addr, (void *)&myProfile, sizeof(myProfile));
	Serial.printf("Profile: model [%s], Software version %i\n", myProfile.model, myProfile.softVer);	
	
	addr += MPSIZE;
	bytesRead += EEPROM.readBytes(addr, (void *)&halCal, sizeof(halCal));
	//Serial.printf("HAL ADCbits = %i\n", halCal.ADCbits);
	
	addr += ADSSIZE;
	/*
	bytesRead += EEPROM.readBytes(addr, (void *)&myADC, sizeof(myADC));
	//Serial.printf("ADC[3] name = [%s]\n", myADC[3].iname);
	
	addr += ADCSIZE;
		*/
	bytesRead += EEPROM.readBytes(addr, (void *)&myID, sizeof(myID));
	
	addr += IDSIZE;
	bytesRead += EEPROM.readBytes(addr, (void *)&sc, sizeof(sc));
	//Serial.printf("Screen Cal Hmin = %i\n",sc.thmin);
	
	addr +=  SCALSIZE;
	bytesRead += EEPROM.readBytes(addr, (void *)&pSet, sizeof(pSet));  	
	
	addr += SETSIZE;
	bytesRead += EEPROM.readBytes(addr, (void *)&myWiFi, sizeof(myWiFi));
	//Serial.printf("Set voltage = %5.2f\n",pSet.voltage);
	
	//Serial.printf("Total bytes to be read = %i\n",addr + sizeof(pSet) );

	return bytesRead;
}

bool checkProfileHdr(void){
  short i, crc;
  // Major software version, controlled hardware (MODEL) and CRC must be correct.

   if(eeProfile.softVer != SOFT_VERSION)   
    {
      Serial.println("Profile check: Major software version difference.");
      Serial.print("Stored:  "); Serial.println(eeProfile.softVer);    
      Serial.print("Current: "); Serial.println(myProfile.softVer);
      return false;
    }
	
    if(eeProfile.hardVer != HARD_VERSION)   
    {
      Serial.println("Profile check: Major hardware version difference.");
      Serial.print("Stored:  "); Serial.println(eeProfile.softVer);    
      Serial.print("Current: "); Serial.println(myProfile.softVer);
      return false;
    }
    
  if (strcmp(eeProfile.model, MODEL) != 0)
  {
    Serial.println("Profile check: Wrong hardware model.");
    Serial.print("Stored:  "); Serial.println(eeProfile.model);    
    Serial.print("Current: "); Serial.println(MODEL);
    return false;
  }
  /*  crc needs to be calculated over the entire profile  */
  crc = crc8((uint8_t *)&eeProfile.softVer, sizeof(eeProfile) - 1);
  if (eeProfile.crc != crc)
  {
    Serial.println("CRC error.");
    Serial.print("Stored CRC:  "); Serial.println(eeProfile.crc);
    Serial.print("Calculated CRC: "); Serial.println(crc);
    return false;
  }

  return true;
}
// called every "VL" period
// set flag if profile value changed. Only saves after SAVE_EE_AFTER loops.
uint8_t saveClockEE = 0;
void saveEE(void)
{
	if (changedVal_EE) // restart the save clock each time a value is changed 
	{
		saveClockEE = 0;
		needToSaveEE = true;
		changedVal_EE = false;
		//Serial.println("Changed setting - need to save to EE");
	}

	if (needToSaveEE && saveClockEE >= SAVE_EE_AFTER) // save to EE after SAVE_EE_AFTER loops from the *last* time a value was changed
	{
		int wb = 0;
#if (EEAUTOSAVE == 1)
		//Serial.printf("About to save to EE\n");
//delay(10);	
		wb = writeEE();
#else
		//Serial.print("EE Save disabled: ");
#endif
		saveClockEE = 0;
		needToSaveEE = false;
		//Serial.printf("Saved %i bytes to EE\n", wb);		
	}	
	saveClockEE++;  
}

#ifdef  PROF_TEST
// ***** Needs updating with latest EEPROM profile structs *****
// EEPROM write-read test - reads back into separate structures and compares.
// not for production use
bool testEEprofile(){
	int addr = 0;
	profile testProf;
	hal_cal testHAL;
	//inADC testADC[ADC_CHANNELS];
	instID testID;
	screenCals testSC;
	settings testSet;
	uint8_t crcx;
	size_t bw, pr, hal, adc, id, scx, set;
	
  //  EEPROM.begin(EESIZE);
	
	Serial.printf("------------- EESIZE is %i bytes -------------\n", EESIZE);
	
	bw = factory_reset(); // write profile from defaults
	Serial.printf("%i bytes written\n", bw);
	
	//read profile into temporary structures	
	addr = 0;
	pr =  EEPROM.readBytes(addr, (void *)&testProf, sizeof(testProf));
	
	addr += MPSIZE;
	hal = EEPROM.readBytes(addr, (void *)&testHAL, sizeof(testHAL));
		
	addr += ADSSIZE;
	/*
	adc = EEPROM.readBytes(addr, (void *)&testADC, sizeof(testADC));
	
	addr += ADCSIZE;

	id =  EEPROM.readBytes(addr, (void *)&testID, sizeof(testID));
	
	addr += IDSIZE;
	scx =  EEPROM.readBytes(addr, (void *)&testSC, sizeof(testSC));
		*/
	addr += SCALSIZE;
	set = EEPROM.readBytes(addr, (void *)&testSet, sizeof(testSet));
	
	Serial.printf("Read %i of %i\n", pr+hal+adc+id+scx+set, EESIZE);

 /*  crc needs to be calculated over the entire profile  */
  crcx = crc8((uint8_t *)&testProf.softVer, sizeof(testProf) - 1);
  if (testProf.crc != crcx)
  {
    Serial.println("CRC error.");
    Serial.print("Stored CRC:  "); Serial.println(testProf.crc);
    Serial.print("Calculated CRC: "); Serial.println(crcx);   
  }	
  else
		Serial.println("CRC OK");
	//Compare written and read versions
	Serial.printf("Profile: Read %i of %i\n", pr, sizeof(myProfile));
	if (memcmp(&testProf, &myProfile, sizeof(testProf)) != 0)
	{
		Serial.println("  Broken Profile");		
	} 
	else
		Serial.println("  Profile OK");
	Serial.printf("  Profile model [%s]\n", testProf.model);	
	
	Serial.printf("HAL: Read %i of %i\n", hal, sizeof(testHAL));
	if (memcmp(&testHAL, &halCal, sizeof(testHAL)) != 0)
		Serial.println("  Broken HAL");
	else
		Serial.println("  HAL OK");
	Serial.printf("  HAL ADCbits = %i\n", testHAL.ADCbits);
	
	/*
	Serial.printf("ADC: Read %i of %i\n", adc, sizeof(testADC));
	if (memcmp(&testADC, &myADC, sizeof(testADC)) != 0)
		Serial.println("  Broken ADC");
	else
		Serial.println("  ADC OK");
	Serial.printf("  ADC[3] name = [%s]\n", testADC[3].iname);
	*/
	Serial.printf("ID: Read %i of %i\n", id, sizeof(testID));
	if (memcmp(&testID, &myID, sizeof(testID)) != 0)
		Serial.println("  Broken ID");
	else
		Serial.println("  ID OK");
	
	Serial.printf("Screen Cal: Read %i of %i\n", scx, sizeof(testSC));
	if (memcmp(&testSC, &sc, sizeof(testSC)) != 0)
	{
		Serial.println("  Broken SC");
		Serial.printf("  SC: Thmin = %i [%i]\n", testSC.thmin, sc.thmin);
	}
	else
		Serial.println("  SC OK");	
	Serial.printf("  Screen Cal Hmin = %i\n",testSC.thmin);
	
	Serial.printf("Settings: Read %i of %i\n", set, sizeof(testSet));
	if (memcmp(&testSet, &pSet, sizeof(testSet)) != 0)
		Serial.println("  Broken Settings");
	else
		Serial.println("  Settings OK");
	Serial.printf("  Set voltage = %5.2f\n",testSet.voltage);
	
	Serial.println ("---------------- End of EE test -------------");
}
#endif

#endif