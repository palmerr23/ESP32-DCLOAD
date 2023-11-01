/*
SCPI definitions and service routines
Command tree is below
System errors
-100	Syntax error (could not parse command)
-200	Execution error (e.g. parameter outside range)
-300	Device error (e.g. overrange error state: short circuit protection, overvoltage on SMPS)

Device errors
108		not yet implemented

v2
-203	Command protected (needs password)
-210	Can't trigger
-310	System error
-400	Query error (e.g. no data to provide)

105		data not found
106 	measurement not found
122		Channel not found
*/
#ifndef  MYLSCPI_H
#define  MYLSCPI_H
#include "lscpiparser.h"
short SCPIsendGrpMessage(uint8_t tgrp, char *buf);
void SCPItrans(char * response);
//uint8_t streamIn[currentStreamIn].channel = 0;  // current channel set by :INST:CHAN (value is: CHAN -1)

bool trackOn = false;	// old control values
float eTrack, trackSv, trackSa, localLimitRatio = 1.0;
int trackGrp = 0;


struct scpi_parser_context ctx;
bool SCPIheartbeatSent;
char myError[64];
char valBuf[64];
float lastVset = 9999, lastCset = 9999;


bool directCommand = false; // do TRAC commands need to be re-broadcast?

// buffer status
#define SCPIBUF_FREE   (-1) // not active stream , may be re-used
#define SCPIBUF_IDLE    0   // ready for a new cycle
#define SCPIBUF_INPUT   1   // set with first char received
#define SCPIBUF_PROCESS 2   // EOL received, passed to processor, reset to free when response has been sent.

// command stream status/type - only one needed per type of connection as each command is 
#define SCPISTREAM_FREE (-1)	// duplicates function of SCPIBUF_FREE
#define SCPISTREAM_SER (-2)
#define SCPISTREAM_TEL (-3)

short serialBufID = SCPISTREAM_FREE;
short telnetBufID = SCPISTREAM_FREE;
short UDPBufID = SCPISTREAM_FREE;
//#define SCPISTREAM_UDP  [0..255]

#define SCPI_INBUFS 6
short SCPIsource = SCPISTREAM_FREE;   // which type of stream did the last command come from?
short SCPIstreamID = SCPISTREAM_FREE; // stream within each source type. ip[3] for UDP
uint8_t currentStreamIn;		      // current input stream (index)   

char SCPIoutBuf[SCPI_BUFLEN]; 		// only one needed, as we only process one SCPI command at a time
struct scpiStrm {	
	char buf[SCPI_BUFLEN];  		// one per input stream - 1 x Serial; 1 x Telnet, n x UDP
	uint8_t len;					// content length
	unsigned short indx;    		// current buffer pointer (for serial - character stream, rest are block) == len????
	int16_t streamID;         		// type of stream - see defines above [0.255] for UDP, etc
	uint16_t remotePort;			// for UDP packet responses
	uint8_t channel = 0;		    // instrument channel last addressed
	int8_t status;					// status of this buffer/connection
	float eTrack;					// last tracking value sent on this stream (active value will be min of active UDP stream values and local value)
	long lastMessage;				// (Timer Seconds) last message received (will time out after different periods for different stream types)
} streamIn[SCPI_INBUFS];  

//function prototypes & service routines
#include "myLSCPIcmds.h"

void SCPI_setup(){	
	struct scpi_command* source;
	struct scpi_command* measure;
	struct scpi_command* instrument;
	struct scpi_command* batt;
	struct scpi_command* step;
	struct scpi_command* func;
	struct scpi_command* system;
	struct scpi_command* error;
#ifndef PROD
	struct scpi_command* test;
#endif
//  struct scpi_command* comms;

  /* First, initialise the parser. */
  scpi_init(&ctx);
 
   	// mandatory registrations
	system = scpi_register_command(ctx.command_tree, SCPI_CL_CHILD, "SYSTEM", 6,  "SYST", 4, NULL);		
	error = scpi_register_command(system, SCPI_CL_CHILD, "ERROR", 5,"ERR", 3, NULL);									   
	scpi_register_command(system, SCPI_CL_CHILD, "ERROR?", 6, "ERR?", 4, system_error);	
	scpi_register_command(error, SCPI_CL_CHILD, "NEXT?", 5, "NEXT?", 5, system_error);	

 //scpi_register_command(parent, location, long_name, long_name_length, short_name, short_name_length, callback);  
  scpi_register_command(ctx.command_tree, SCPI_CL_SAMELEVEL, "*IDN?", 5, "*IDN?", 5, identify);
  scpi_register_command(ctx.command_tree, SCPI_CL_SAMELEVEL, "*TST?", 5, "*TST?", 5,get_post);
  // tree roots
  source =     scpi_register_command(ctx.command_tree, SCPI_CL_CHILD, "SOURce",      6, "SOUR", 4, NULL);
  measure =    scpi_register_command(ctx.command_tree, SCPI_CL_CHILD, "MEASure",     7, "MEAS", 4, NULL);
  instrument = scpi_register_command(ctx.command_tree, SCPI_CL_CHILD, "INSTrument", 10, "INST", 4, NULL);
  func =       scpi_register_command(ctx.command_tree, SCPI_CL_CHILD, "FUNCtion",       8, "FUNC", 4, NULL);
  // submenus
  batt =       scpi_register_command(func, SCPI_CL_CHILD, "BATTery",       7, "BATT", 4, NULL);
  step =       scpi_register_command(func, SCPI_CL_CHILD, "STEP",        4, "STEP", 4, NULL);
#ifndef PROD
  test = 	   scpi_register_command(ctx.command_tree, SCPI_CL_CHILD, "TEST", 4, "TEST", 4, NULL);	
#endif
  // SOURCE 
  scpi_register_command(source, SCPI_CL_CHILD, "MODE", 4,        "MODE", 4, set_mode);
  scpi_register_command(source, SCPI_CL_CHILD, "VOLTage", 7,     "VOLT", 4, set_voltage);
   scpi_register_command(source, SCPI_CL_CHILD, "VMODe", 5,      "VMOD", 4, set_vMode);
  scpi_register_command(source, SCPI_CL_CHILD, "CURRent", 7,     "CURR", 4, set_current);
  scpi_register_command(source, SCPI_CL_CHILD, "POWEr",   5, 	   "POWE", 4, set_power);
  scpi_register_command(source, SCPI_CL_CHILD, "RESIstance", 9,  "RESI", 4, set_resistance);
  scpi_register_command(source, SCPI_CL_CHILD, "STATe", 	5,     "STAT", 4, set_state); // on-off
  scpi_register_command(source, SCPI_CL_CHILD, "LOGInterval",11, "LOGI", 4, set_log_interval); // log interval
  // query setpoints
  scpi_register_command(source, SCPI_CL_CHILD, "MODE?", 5,        "MODE?", 5, get_set_mode);
  scpi_register_command(source, SCPI_CL_CHILD, "VOLTage?", 8,     "VOLT?", 5, get_set_voltage);
    scpi_register_command(source, SCPI_CL_CHILD, "VMODe?", 6,     "VMOD?", 5, get_set_vmode);
    scpi_register_command(source, SCPI_CL_CHILD, "VMACtual?", 9,  "VMAC?", 5, get_act_vmode);
  scpi_register_command(source, SCPI_CL_CHILD, "CURRent?", 8,     "CURR?", 5, get_set_current);
  scpi_register_command(source, SCPI_CL_CHILD, "RESIstance?", 10, "RESI?", 5, get_set_resistance);
  scpi_register_command(source, SCPI_CL_CHILD, "POWEr?", 	6,      "POWE?", 5, get_set_power);
  scpi_register_command(source, SCPI_CL_CHILD, "STATe?", 	6,      "STAT?", 5, get_set_state);
  scpi_register_command(source, SCPI_CL_CHILD, "LOGInterval?",12, "LOGI?", 5, get_log_interval); // log interval

  // MEASURE - read actual values 
  scpi_register_command(measure, SCPI_CL_CHILD, "VOLTage?", 8,      "VOLT?", 5, get_voltage);
  scpi_register_command(measure, SCPI_CL_CHILD, "CURRent?", 8,      "CURR?", 5, get_current);
  scpi_register_command(measure, SCPI_CL_CHILD, "RESIstance?", 10,  "RESI?", 5, get_resistance);
  scpi_register_command(measure, SCPI_CL_CHILD, "POWEr?", 	6,      "POWE?", 5, get_power);
  scpi_register_command(measure, SCPI_CL_CHILD, "TEMPerature?", 12, "TEMP?", 5, get_temp);

  // STEP 
  scpi_register_command(step, SCPI_CL_CHILD, "STATe", 5,       	  "STAT",  4, set_step_state);
  scpi_register_command(step, SCPI_CL_CHILD, "STATe?", 6,         "STAT?", 5, get_step_state);
  scpi_register_command(step, SCPI_CL_CHILD, "ACURrent", 9,       "ACUR",  4, set_current_A);
  scpi_register_command(step, SCPI_CL_CHILD, "ACURrent?", 10,     "ACUR?", 5, get_current_A);
  scpi_register_command(step, SCPI_CL_CHILD, "BCURrent", 9,       "BCUR",  4, set_current_B);
  scpi_register_command(step, SCPI_CL_CHILD, "BCURrent?", 10,     "BCUR?", 5, get_current_B);
  
  scpi_register_command(step, SCPI_CL_CHILD, "AVOLtage", 8,       "AVOL",  4, set_voltage_A);
  scpi_register_command(step, SCPI_CL_CHILD, "AVOLtage?", 8,      "AVOL?", 5, get_voltage_A);
  scpi_register_command(step, SCPI_CL_CHILD, "BVOLtage", 8,       "BVOL",  4, set_voltage_B);
  scpi_register_command(step, SCPI_CL_CHILD, "BVOLtage?", 8,      "BVOL?", 5, get_voltage_B); 
  
  scpi_register_command(step, SCPI_CL_CHILD, "ATIMe", 5,       "ATIM",  4, set_time_A);
  scpi_register_command(step, SCPI_CL_CHILD, "ATIMe?", 6,      "ATIM?", 5, get_time_A);  
  scpi_register_command(step, SCPI_CL_CHILD, "BTIMe", 5,       "BTIM",  4, set_time_B);
  scpi_register_command(step, SCPI_CL_CHILD, "BTIMe?", 6,      "BTIM?", 5, get_time_B);    

  scpi_register_command(step, SCPI_CL_CHILD, "ASTEps",  5,       "ASTE",  4, set_steps_A); 
  scpi_register_command(step, SCPI_CL_CHILD, "ASTEps?", 6,       "ASTE?", 5, get_steps_A);   
  scpi_register_command(step, SCPI_CL_CHILD, "BSTEps",  5,       "BSTE",  4, set_steps_B); 
  scpi_register_command(step, SCPI_CL_CHILD, "BSTEps?", 6,       "BSTE?", 5, get_steps_B); 
  
  scpi_register_command(step, SCPI_CL_CHILD, "SLEW",  4,       "SLEW",  4, set_slew);   
  scpi_register_command(step, SCPI_CL_CHILD, "SLEw?", 5,       "SLEW?", 5, get_slew);    
  
  scpi_register_command(step, SCPI_CL_CHILD, "CYCLes",  6,       "CYCL",  4, set_cycles);   
  scpi_register_command(step, SCPI_CL_CHILD, "CYCLes?", 7,       "CYCL?", 5, get_cycles);    
  
  //BATTERY
  scpi_register_command(batt, SCPI_CL_CHILD, "STATe", 5,       	  "STAT",  4, set_batt_state);
  scpi_register_command(batt, SCPI_CL_CHILD, "STATe?", 6,         "STAT?", 5, get_batt_state);
  scpi_register_command(batt, SCPI_CL_CHILD, "ESR?", 4,           "ESR?",  4, get_batt_esr);
  scpi_register_command(batt, SCPI_CL_CHILD, "CAPAcity?", 9,      "CAPA?", 5, get_batt_capacity);
    /*
  BATStartr
  BATEndR
  WAVEdata
  */
  // SYSTEM
	scpi_register_command(system, SCPI_CL_CHILD, "PASSword", 8,    "PASS", 4, set_pass);   
	scpi_register_command(system, SCPI_CL_CHILD, "PASSword?", 9,   "PASS?", 5, get_pass);  
	scpi_register_command(system, SCPI_CL_CHILD, "SSID", 4,        "SSID", 4, set_ssid);  
	scpi_register_command(system, SCPI_CL_CHILD, "SSID?", 5,       "SSID?", 5, get_ssid);  
	scpi_register_command(system, SCPI_CL_CHILD, "CONNect", 7,     "CONN", 4, set_connect);  
	scpi_register_command(system, SCPI_CL_CHILD, "CONNect?", 8,    "CONN?", 5, get_connect);  
	scpi_register_command(system, SCPI_CL_CHILD, "HOSTname", 8,    "HOST", 4, set_hostName);  
	scpi_register_command(system, SCPI_CL_CHILD, "HOSTname?", 9,   "HOST?", 5, get_hostname);  
#ifndef PROD
	scpi_register_command(test, SCPI_CL_CHILD, "ADSV?", 5,    "ADSV?",  5, get_ADSv);  
	scpi_register_command(test, SCPI_CL_CHILD, "ADSVC?", 6,   "ADSVC?", 6, get_ADSvC);  
	scpi_register_command(test, SCPI_CL_CHILD, "ESPV?", 5,    "ESPV?",  5, get_ESPv);  
	scpi_register_command(test, SCPI_CL_CHILD, "ESPVC?", 6,   "ESPVC?", 6, get_ESPvC); 
	
	scpi_register_command(test, SCPI_CL_CHILD, "ADSA?", 5,    "ADSA?",  5, get_ADSa);  
	scpi_register_command(test, SCPI_CL_CHILD, "ADSAC?", 6,   "ADSAC?", 6, get_ADSaC);  
	scpi_register_command(test, SCPI_CL_CHILD, "ESPA?", 5,    "ESPA?",  5, get_ESPa);  
	scpi_register_command(test, SCPI_CL_CHILD, "ESPAC?", 6,   "ESPAC?", 6, get_ESPaC);  
	scpi_register_command(test, SCPI_CL_CHILD, "DAC?", 4,     "DAC?",   4, get_DAC);  	
#endif
	
	for (int i = 0; i < SCPI_INBUFS; i++)
	{
		streamIn[i].indx = 0; // empty buffer
		streamIn[i].status = SCPIBUF_FREE;
		streamIn[i].streamID = SCPISTREAM_FREE;
	}
}
short getSCPIbuf(short stream)
{
	short i;
	short freeBuf = -1;
//Serial.printf("Getbuf: seeking %i. ", stream);
	// look for existing buffer, tag free ones along the way
	for (i = 0; i < SCPI_INBUFS; i++)
	{
		if (streamIn[i].streamID == stream)
		{
			//Serial.printf("Found existing %i ", i);
			streamIn[i].streamID = stream;
			streamIn[i].lastMessage = millis() / 1000;
			return i;		
		}

		if (streamIn[i].status == SCPIBUF_FREE) // will return the last free buf id
			freeBuf = i;
	}
	//Serial.printf("New %i ", freeBuf);
	if(freeBuf > 0)
	{
		streamIn[freeBuf].streamID = stream;
		streamIn[i].lastMessage = millis() / 1000;
	}
	return freeBuf;
}
// handle one message each cycle, save who it's from
// returns buffer ID when a complete message has been received.
// ***************************** Needs to be non blocking code ******************

short SCPIread(void){
	// no delayed commands allowed
	// read available chars from each stream, process first one to complete a line 
	// read available serial into buffer
	//short bufID;
	//Serial.print(":");
	if (Serial.available() > 0) 
	{
		//Serial.print("*");
		if(serialBufID == SCPISTREAM_FREE) // won't release this, if Serial is being used
		{
			serialBufID = getSCPIbuf(SCPISTREAM_SER); 
			streamIn[serialBufID].status = SCPIBUF_INPUT;
			streamIn[serialBufID].indx = 0;			
		}		
		
		while (Serial.available())
		{
			streamIn[serialBufID].buf[streamIn[serialBufID].indx] = Serial.read();
			//Serial.print(streamIn[serialBufID].buf[streamIn[serialBufID].indx]); //echo to serial
			// if EOL return the bufferID
			if(streamIn[serialBufID].buf[streamIn[serialBufID].indx++] =='\n')
			{
				streamIn[serialBufID].indx--;
				streamIn[serialBufID].buf[streamIn[serialBufID].indx] = '\0'; // kill the '\n'
				streamIn[serialBufID].status = SCPIBUF_PROCESS;
				streamIn[serialBufID].len = strlen(streamIn[serialBufID].buf);
				SCPIsource = SCPISTREAM_SER;
				currentStreamIn = serialBufID;
				directCommand = true;
				//Serial.printf("SCPI Serial [%s]\n", streamIn[serialBufID].buf);
				return currentStreamIn;
			}				
		}	
		
		streamIn[serialBufID].lastMessage = millis()/1000; // any character == heartbeat
	}
	// process telnet
	char *iptr;
	iptr = getTelnet(); // '\n' already dealt with in here
	if (iptr)
	{	

		if(telnetBufID == SCPISTREAM_FREE) // won't release this, if Telnet is being used
		{
			telnetBufID = getSCPIbuf(SCPISTREAM_TEL); 
			streamIn[telnetBufID].status = SCPIBUF_PROCESS;					
		}
		streamIn[telnetBufID].lastMessage = millis()/1000;		
		strcpy(streamIn[telnetBufID].buf, iptr);
		streamIn[telnetBufID].len = strlen(iptr);
		SCPIsource = SCPISTREAM_TEL;
		currentStreamIn = telnetBufID;
		directCommand = true;
		//Serial.printf("SCPI Telnet [%s]\n",streamIn[telnetBufID].buf);
		return currentStreamIn;		
	}
	// process UDP packets
	// search through buffer pile for active UDP buffers
	iptr = getUDP();
	if (iptr)
	{
		// separate streams for each remote host
		UDPBufID = getSCPIbuf(UDPsourceIP[3]); // may release this, if exceeds SCPIUDPTIMEOUT
		//Serial.printf("UDP pkt in [%s] to buffer %i from IP[%i]\n", iptr, UDPBufID,UDPsourceIP[3]);
		streamIn[UDPBufID].status = SCPIBUF_PROCESS;	
		streamIn[UDPBufID].remotePort =	UDPsourcePort;	
		strcpy(streamIn[UDPBufID].buf, iptr);
		streamIn[UDPBufID].len = strlen(iptr);
		SCPIsource = UDPsourceIP[3];
		currentStreamIn = UDPBufID;
		streamIn[UDPBufID].lastMessage = millis()/1000;
		directCommand = false;
		//Serial.printf("SCPI UDP [%s]\n", streamIn[UDPBufID].buf);
		return currentStreamIn;	
	}
	
	// nothing waiting
	directCommand = false; // never used????
	return SCPISTREAM_FREE;
}

// call from main loop
short SCPIreadBufID; // used to echo group commands from Serial or Telnet
void  process_SCPI(void)
{
  short stat;  
  SCPIreadBufID = SCPIread(); // we only process one command from one stream each time around
  
  if(SCPIreadBufID != SCPISTREAM_FREE)
  {
//Serial.printf("SCPI_proc: buf [%s], len = %i\n", streamIn[SCPIreadBufID].buf, strlen(streamIn[SCPIreadBufID].buf));
    stat = scpi_execute_command(&ctx, streamIn[SCPIreadBufID].buf, strlen(streamIn[SCPIreadBufID].buf)); 
	if (stat < 0 && strcmp(streamIn[SCPIreadBufID].buf, HEART_MSG))
		Serial.printf("SCPI_proc: unknown cmd [%s]\n", streamIn[SCPIreadBufID].buf);
	streamIn[SCPIreadBufID].indx = 0;
	streamIn[SCPIreadBufID].status = SCPIBUF_IDLE;
  }
}

// send response to requestor
void SCPItrans(char * response)
{
	bool sentOK;
	// which is the current stream?
	if(SCPIsource == SCPISTREAM_SER)
	{
		Serial.print(response);
		return;
	}
	
	if (SCPIsource == SCPISTREAM_TEL)
	{
		//Serial.printf("SCPIOUT Telnet: %s\n", response); // for now!
		sentOK = putTelnet(response);
		if(!sentOK)
			Serial.printf("SCPIOUT Telnet: unable to send\n");
		return;
	}
		
	if (SCPIsource >= 0) // UDP streams
	{
		IPAddress sendIP = myIP;
		sendIP[3] = streamIn[currentStreamIn].streamID;
		//Serial.printf("SCPI UDP OUT [%i]: %s\n", SCPIsource, response); // for now!
		// not broadcast - point to point response ********* do we ignore all messages outside our track grp, or just broadcast ones??
		putUDP(streamIn[currentStreamIn].buf, sendIP, streamIn[currentStreamIn].remotePort, trackGrp, 0);
		//streamIn[currentStreamIn].status = SCPIBUF_IDLE;
		return;
	}
}

// send response to a tracking group
// ********** NOT used ****** See SCIPgrpMsg
void SCPItransGrp(char * response, uint8_t groupID){
	// UDP broadcast
  //Serial.println(response);
}


// check UDP streams for recent messages, disable those exceeding SCPIUDPTIMEOUT
// disp == false for production
/*
uint8_t countSCPIstreams(bool disp)
{
	short i, numStreams = 0;
	//if(disp) Serial.printf("\nSTRM upd @ %i: ", (millis()/1000));
	for(i = 0; i < SCPI_INBUFS; i++)
	{		
		if(streamIn[i].streamID >= 0 && streamIn[i].status >= 0) // UDP stream and not FREE
		{	
			if((streamIn[i].lastMessage + SCPIUDPTIMEOUT) > (millis() / 1000 ))
			{
				//if(disp) Serial.print("*");
				numStreams++;
			}
			else
			{
				//if(disp) Serial.print("Freeing");
				streamIn[i].streamID = SCPISTREAM_FREE; // free up the stream
				streamIn[i].status = SCPIBUF_FREE; // free up the buffer
			}
		}
	}
	return numStreams;
}
*/
// broadcast a heartbeat message to the rest of the tracking group
// return number of active UDP streams 
/*
short eTcntr = 99;
uint8_t SCPIgroupHeartbeat(void)
{	
	
	// send our heartbeat message if nothing's been sent since last time
	if(SCPIheartbeatSent == false) // nothing else sent since we last sent a message
	{
		sprintf(valBuf,HEART_MSG); // body length needs to be >= "*IDN?" or it will be discarded
		SCPIsendGrpMessage(trackGrp, valBuf);
		eTcntr = 0;
	}
	
	SCPIheartbeatSent = false; 
	
	// return number of active UDP streams
	return countSCPIstreams(true); // false for prod
}

// send message to group
// return number of active UDP streams 
short SCPIsendGrpMessage(uint8_t tgrp, char *buf)
{
short i, UDPcount = 0;
	// send message to my group
	putUDP(buf, myBroadcastIP, UDP_PORT,  tgrp, 0);
//Serial.printf("SCPI Grp [%i] Msg: %s\n", tgrp, buf);
	// reset heartbeat clock - any outgoing message will do
	SCPIheartbeatSent = true;	
		eTcntr = 0;
	return countSCPIstreams(false);;
}
*/
///////////////////// UNUSED //////////////////////////
// group commands passed through to slave
// local control settings passed through to slave
void SCPIpassThru(char * command){
  // transmit to my slave channel
}
// responses to commands and SCPI messages from slave
void SCPIpassBack(char * command){
  // transmit to my master channel
  // process locally if required
}
void printSCPIerrQ(struct scpi_parser_context* context)
{
	struct scpi_error* err;
	if(context->error_queue_head != NULL)
	{
		err = context->error_queue_head;
		Serial.printf("Err Q head |%s|\n", err->description);
	}
	else 
		Serial.println("Err Q empty.");

}

#endif
