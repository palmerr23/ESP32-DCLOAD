#ifndef  MYMESP32_H
#define  MYMESP32_H
/* HTTP 
*/
#define INCHANS 8 // kill me!


// UDP or web client slave status - see myMESP32.h
#define SLAVETIMEOUT	15		// seconds - no packets received from slave will change status to SLAVE_SLEEP (needs to be longer than heartbeats)
#define SLAVE_MT	 0			// empty slot (status and type)
#define SLAVE_ACTIVE 1			// slave is alive
#define SLAVE_SLEEP  2			// slave was alive (not currently used for processing)

// slave types - these are sometimes used alongside clientID numbers [0..7]
#define SLAVE_UDP	 0x10
#define SLAVE_HTTP	 0x20
#define SLAVE_USB    0x40		// not implemented
#define SLAVE_ALL	 0xF0		// not sure if this is used

// diagnostics
//#define CDEBUG2
#define MAXSLAVES 12				// no point in having more controllers than outputs!?
// Master only
// VAL_CHGD_MASK,VAL_BCAST_MASK, VAL_EE_MASK in vChanged are ignored here
struct unitRegister{
	IPAddress slaveIP;
	uint32_t  lastMessage; 		// in mS
	uint16_t  vChanged;			// valChanged for this client (global for UDP - broadcast update; individual for HTTP clients)
	int32_t   sessionID;
	uint8_t   type = SLAVE_MT;
	uint8_t	  status = SLAVE_MT;	
} slave[MAXSLAVES]; 

// valChanged masks - bitmap[16]
// what needs to be done?
#define VAL_ALL_BITS		0xffff
#define VAL_CHGD_MASK		0x01 
#define VAL_BCAST_MASK		0x02 	// broadcast local change 
#define VAL_EE_MASK			0x04 	// General save to EE - may be ignored, may happen regularly
#define VAL_ALL_CHGD	(0xfff0)



#define VAL_AUDIO_MASK		0x08    // send to audio processor - may be ignored
// group of items changed
#define VAL_CNAME_MASK		0x10 
#define VAL_ONAME_MASK		0x20
#define VAL_FADER_MASK		0x40	// output fader, pfl, chOn
#define VAL_FADER_BLOCK_MASK 0x80
#define VAL_GAIN_MASK		0x100	// gain + inPad
#define VAL_CAL_MASK		0x200	// screen Cal
#define VAL_ID_PROF_MASK	0x400	// ID or profile
/*
// test mask for any bot set


// common cmd combinations
#define VAL_CHGD_BCAST 	  	(VAL_CHGD_MASK | VAL_BCAST_MASK)		// a value has changed that needs broadcast + screen update
#define VAL_CHGD_NO_BCAST 	VAL_CHGD_MASK 							// only a local value has changed, screen updated only, no need to send an update packet
#define VAL_CHGD 			VAL_CHGD_NO_BCAST						// catch the generic calls, assume they're local changes only
#define VAL_ALL_CHGD	(VAL_GAIN_MASK | VAL_FADER_MASK | VAL_CNAME_MASK | VAL_ONAME_MASK | VAL_FADER_BLOCK_MASK)
*/
uint16_t x_changed = 0;

// changedVal_EE = immediate flag to start the save count, set by valChanged().  EEflag just sets the screen indicator flag on
//volatile bool changedVal_scrn = false, changedVal_EE = false, changedVal_UDP = false; // changed settings values need to be addressed by several modules
// bool EEflag = false

// turn on flags for eventual updates, each is reset by the servicing routine
bool EEflag = false; // if need to save to EE. Reset when EE is saved.

// UDP (display control) slave uses VAL_BCAST_MASK to determine whether a local change should be broadcast (= all actions, other than turn wireless on).
// x_changed is used by both master and slave for local processing. 
// Master processing also sets/unsets slave[i].vChanged flags, based on VAL_BCAST_MASK. 
// Master also uses slave[i]. to ensure the various client type are updated as well. 
// Each change is "done" when the new value has been sent to that client (HTTP - don't send back to the one that sent the change) or type (broadcast UDP).
// ************* 
// clientType (source of change) is a compound flag: [0..15] == a specific client. [16-255] is a client type (UDP, HTTP or USB serial).
// clientType must be one or the other (type or clientID) or any TYPE flag present will prevent processing for clientID == 0
// Processing is not yet fully coded
/*
void valChanged(uint16_t cmd, uint8_t clientType = SLAVE_ALL)
{
	bool UDPsource = false;
	if(clientType & SLAVE_UDP)
		UDPsource = true;
	//Serial.printf("vC: cmd 0x%04x, UDP %i: ",cmd, UDPsource);	
	x_changed |= VAL_CHGD_MASK; 	// always set the basic flag (just in case)
	x_changed |= cmd;				// set all the appropriate bits on, each stream of activity resets them as they are processed	
	EEflag |= (VAL_EE_MASK & cmd);	
	saveEEtime  = millis() + EE_SAVE_AFTER;		// put off an EESAVE until EE_SAVE_AFTER last change 
	
	if (isMaster)
	{	
		if(cmd & VAL_BCAST_MASK)
			for(int i = 0; i < MAXSLAVES; i++)
			{
				if(slave[i].status != SLAVE_MT) // All types get set
				{
					// if change came from a UDP client broadcast, then no need to re-broadcast to other UDP, just HTTP and USB.					
					// other sources, still need to broadcast ot UDP
					if(UDPsource && (slave[i].type != SLAVE_UDP) || !UDPsource)
					{
						//Serial.printf("%i [0x%04x], ", i, slave[i].vChanged);
						slave[i].vChanged |= cmd;
						//slaves[i].vChanged|= VAL_CHGD_MASK;
					}
				}		
			}			
	}
	else
	{
		// update screen
		changedVal_scrn = true; 		// update the screen		
	}
	//Serial.println();
}
*/
// clientType is the stream that has processed the change
// default to SLAVE_UDP is generally non-destructive as UDP changes are broadcast, no matter from which client/master.
/*
void doneChange(uint16_t flags = VAL_ALL_BITS, uint8_t clientType = SLAVE_UDP)
{
	uint8_t isType;
	//Serial.printf("dC: cmd 0x%04x, CT 0x%04x: ",flags, clientType);	
	isType = clientType & SLAVE_ALL; // this is a TYPE call, not a CLIENT one		
	x_changed &= ~flags;	// set all the appropriate bits off for local change
	if(x_changed == VAL_CHGD_MASK) 	// if only the basic bit is set, turn it off
		x_changed = 0x00;
	// all UDP flags cleared at once (Type). HTTP/USB cleared when data sent to that client (clientID).
	if(isMaster)
	{
		if(isType)				// if called with a type, process all slaves of this type
			for(int i = 0; i < MAXSLAVES; i++)
			{
				// if type == UDP, clear all UDP 
				// for others, just clear this one applies to this slave = same type?	
				if((slave[i].status != SLAVE_MT) && (slave[i].type & isType)) 
				{
					slave[i].vChanged &= ~flags;
					if(!slave[i].vChanged & 0xf8) 	// if only the basic bits are set, turn them off
						slave[i].vChanged = 0x00;
					//Serial.printf("%i [0x%04x], ", i, slave[i].vChanged);						
				}	
			}
		else // just process this slave
		{
			slave[clientType].vChanged &= ~flags;
			if(slave[clientType].vChanged == VAL_CHGD_MASK) 	// if only the basic bit is set, turn it off
				slave[clientType].vChanged = 0x00;	
		}		
	}
	//Serial.println();
}
*/
// hasChanged() and changes() do no slave[x] processing 
// is this flag set? 
bool hasChanged(uint16_t cmd = VAL_ALL_BITS)	
{
	return (x_changed & cmd);
}
/*
bool hasChangedC(uint16_t cmd = VAL_ALL_BITS, uint8_t type = SLAVE_UDP)	
{ 
bool hc = false;
	for(int i = 0; i < MAXSLAVES; i++)
		if(slave[i].status != SLAVE_MT && slave[i].type == type && slave[i].vChanged & cmd)
			hc = true;
	return hc;
}
*/
// what changes are pending?
uint16_t changes(uint16_t flags = VAL_ALL_BITS)
{ 
	return x_changed & flags;
}
void printActiveChanges(uint16_t x_changed){
  Serial.print("Active change flags: ");
  if(!x_changed)
    Serial.print("No active changes ");
  if(x_changed & VAL_CHGD_MASK)
    Serial.print("VAL_CHGD_MASK ");
  if(x_changed & VAL_EE_MASK)
    Serial.print("VAL_EE_MASK ");
  if(x_changed & VAL_BCAST_MASK)
    Serial.print("VAL_BCAST_MASK ");
/*
  if(x_changed & VAL_AUDIO_MASK)
    Serial.print("VAL_AUDIO_MASK ");
  if(x_changed & VAL_CNAME_MASK)
    Serial.print("VAL_CNAME_MASK ");
  if(x_changed & VAL_ONAME_MASK)
    Serial.print("VAL_ONAME_MASK ");
  if(x_changed & VAL_FADER_MASK)
    Serial.print("VAL_FADER_MASK ");
  if(x_changed & VAL_FADER_BLOCK_MASK)
    Serial.print("VAL_FADER_BLOCK_MASK ");    
  if(x_changed & VAL_GAIN_MASK)
    Serial.print("VAL_GAIN_MASK ");
*/
  Serial.println();
}
void printClientChanges(int clientX)
{
  if(clientX >= 0)
  {
    Serial.printf("Client %i: ",clientX);   
    printActiveChanges(slave[clientX].vChanged);
  }
  else
    for(int i = 0; i < MAXSLAVES; i++)
      if(slave[i].status == SLAVE_ACTIVE)
      {
        Serial.printf("Client %i: ",i);   
        printActiveChanges(slave[i].vChanged);
      }
}

// calculated display levels for the HTTP interface
// these are calculated on the fly every time a level packet is requested.
// they will relate to the current screen of the requesting Host.
/*struct outLevels {
  float outLevel[INCHANS];
  float outlevelM;
  int scrn;  // may not be used
} oLevels;
*/
//struct faderPkt mySets;
//bool pfl[INCHANS];
//bool chOn[INCHANS];
//short  highButton = -1;	// currently selected screen button
//instID myID = {{192,168,50,1}, {192,168,50,1},{192,168,50,0}, LOCAL_SS, LOCAL_PASS, ESPNETNAME, ESPNETPW, true, 1, 7, ISSLAVE, "MONI"};
//settings pSetA ; // do we still use this????
// are we using the encoder to change values or change screens?
bool changeScreenMode = true; // this starts with a false 'press' so intialise to ON
long lastScreenChange = 0;

//#define WIFITYPE SLAVE
/*
uint16_t byteTime;
HardwareSerial mySerial(2); // UART2 - only one that's accessible while using SPI ***WILL NOT WORK ON WROVERS as IO 16/17 are NC ******
							//Pins 16 & 17
bool mySerialBegin(uint32_t speed)
{
  //Serial.printf("Starting UART 2 on TXD = %i, RXD = %i\n",TXD2,RXD2); 
  mySerial.begin(speed);
  mySerial.setTimeout(SERIALTIMEOUT);
  //Serial.println("Serial Txd is on pin: "+String(TX));
  //Serial.println("Serial Rxd is on pin: "+String(RX));
  byteTime = (11 * 1000000L)/speed;	// microseconds per byte, assuming 11 bit times per byte
  // audio processor should start sending level packets within SERIALTIMEOUT seconds if we're the master unit
 int byteCount = 0; 
 Serial.println("Checking for Teensy serial pkts");
 for (int i = 0; i < SERIALWAIT; i++)
  {
	while (mySerial.available())  
	{ 	
		uint8_t dummy = mySerial.read();	// phantom first byte?
		byteCount++;
		//Serial.printf("Master mode: Read from Teensy %i 0x%2x\n", i, dummy);
		if(byteCount > 5)
			return ISMASTER;
	}
	delay(1000);
  }
  return ISSLAVE;
}
// Reconcile different gains.inpad contributions
// Part comes from Teensy and part from slaves.
uint8_t lastInpadT = 0;
void fixInpad(void)
{		
	if(inLevels.inpadT != lastInpadT) // broadcast change of line/level
	{
		Serial.printf("Fix Inpad: gains 0x%x, levels 0x%x, ", gains.inpad, inLevels.inpadT);
		valChanged(VAL_GAIN_MASK); // don't broadcast the change
		lastInpadT = inLevels.inpadT;
	}
	gains.inpad = (gains.inpad & PADCHANS) | (inLevels.inpadT & MLCHANS);
	//Serial.printf("result 0x%x\n", gains.inpad);	
}
*/
#endif
