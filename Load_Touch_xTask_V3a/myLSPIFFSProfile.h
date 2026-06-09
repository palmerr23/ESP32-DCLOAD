/*

Only comms parameters stored in profile.json
All other seetings are in EEPROM

*/

#ifndef MYSPIFFSPROF_H
#define MYSPIFFSPROF_H
#include "FS.h"
#include "SPIFFS.h"
#define FORMAT_SPIFFS_IF_FAILED true
#include <ArduinoJson.h>
#define BLKSIZE 1024	// assumes whole file is < this
char proFile[] = "/profile.json";
//StaticJsonDocument<BLKSIZE> jDoc;
DynamicJsonDocument jDoc(BLKSIZE);
uint8_t fileBuf[BLKSIZE];	
struct version {  
  uint8_t softVer; // always after crc, length does not vary between versions
  char model[EE_MODEL];
  uint8_t hardVer; // always after crc, length does not vary between versions
} myVersion = {SOFT_VERSION, MODEL, HARD_VERSION};

bool setComms(bool create);
void printProfile(void);
void printCal(void);
bool setProfile(bool create =  false);
void listDir();
void errorMsg(String error, bool restart);
/*
void JsetVersion();
void JsetHalCal();
void JsetIDblock();
void JsetScreenCal();
void JsetpSet();
bool JgetVersion();
bool JgetHalCal();
bool JgetIDblock();
bool JgetScreenCal();
bool JgetpSet();
*/
// profile.json is SSID, Password and Hostname 
// a false return will write default parameters from myID to be written
bool getComms(void)
{
	bool success = true;
  jDoc.clear();
	File file = SPIFFS.open(proFile);

	if(!file)
	{
		 Serial.printf("%s − failed to open for reading\n", proFile);
		 //setProfile(true);	// create a fresh profile
		// Serial.println("Saving a fresh profile");
		 setComms(true);
		 return false;
	}
	int len = file.size();
	file.read(fileBuf, len);
#ifdef PRINT_DIAGNOSTICS
	Serial.printf("read [%s]\n", fileBuf);
#endif

	DeserializationError error = deserializeJson(jDoc, fileBuf);
	if(error) 
	{
	  Serial.println("Parsing JSON failed");
    //setProfile(true);  // create a fresh profile
	  return false;
	}
	file.close();
	strcpy(myID.local_ssid, jDoc["ssid"]);
	strcpy(myID.local_pass, jDoc["pass"]);
	strcpy(myID.instName, jDoc["hostname"]);
//Serial.printf("Profile.json: ssid=%s, pass=%s\n",myID.local_ssid,myID.local_pass);
  // IDblock should be always present

#ifdef PRINT_DIAGNOSTICS	
	printProfile(); // can only be called after deserialization 
#endif
	return true;
}
// create is ignored.
bool setComms(bool create)
{
  char * bp = (char *)fileBuf; // serializeJson() needs char, file.write needs uint8_t
	fileBuf[0] = 0; // empty the buffer (char NULL)
  jDoc.clear();
	jDoc["ssid"] = myID.local_ssid;
	jDoc["pass"] = myID.local_pass;
	jDoc["hostname"] = myID.instName;     

	//Serial.println("Setting comms profile:");
	//printProfile();
   /* */
  //  Serial.println("JSON: "); 
  //  serializeJsonPretty(jDoc, Serial);
  //return false; // serialize clears the jDoc?
 
  serializeJsonPretty(jDoc, bp, BLKSIZE);
	int len = strlen((char *)fileBuf);
//  Serial.printf("Saving JSON. len = %i [%s]\n", len, fileBuf);

  File file = SPIFFS.open(proFile, FILE_WRITE);
  if(!file)
  {
     Serial.printf("%s − failed to open for writing\n", proFile);
     return false;
  }
	int savd = file.write(fileBuf, len+1); // write the closing \0 as well.
	file.close();
 // Serial.printf("Saved %i bytes\n", savd);
	return true;
}

/************** profile utilities *********************/
void printProfile(void)
{
	Serial.println("JSON profile:");
	
serializeJsonPretty(jDoc, Serial);
}
// assumes no subdirectories
void listDir()
{
    Serial.printf("Listing SPIFFS root:\r\n");
    File root = SPIFFS.open("/");
    if(!root){
        Serial.println("- failed to open root directory");
        return;
    }
    File file = root.openNextFile();
		if(!file)
        Serial.println("- SPIFFS FS is empty"); 
    while(file){        
				Serial.print("  FILE: ");
				Serial.print(file.name());
				Serial.print("\tSIZE: ");
				Serial.println(file.size());        
        file = root.openNextFile();
    }		
}
#endif
