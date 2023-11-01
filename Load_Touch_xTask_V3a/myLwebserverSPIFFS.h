// web server routines 
// SPIFFS now in myLprofile.h
#ifndef MYLWSPIFFS_H
#define MYLWSPIFFS_H

#include "myLInst.h"
#include "myLoad.h"
#include "FS.h"
#include "SPIFFS.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include "my_mx_defs.h"
#define FORMAT_SPIFFS_IF_FAILED true

void slashRequest(AsyncWebServerRequest *request, int client);
void webCommands(AsyncWebServerRequest *request);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void makeJSONreads(char screen, int client);
void makeJSONsets(char screen, int client);
void printParamsX(AsyncWebServerRequest *request);
void dumpHex(const void* data, size_t size) ;
void makeJSONpreset(int preNum);
void printTail(char * str, int tailLen);
int getClient(AsyncWebServerRequest *request);
char getScreen(AsyncWebServerRequest *request);
int registerHTTPslave(AsyncWebServerRequest *request, int session);
void printParams(AsyncWebServerRequest *request);
void printOutlevels(void) ;
void makeLog(void);
void makeLogJSON(int);
bool readLogBegin(void);
size_t readLog(char* buffer, size_t maxLen);
size_t readBatLog(char* buffer, size_t maxLen);
uint8_t setModeB(uint8_t bMode);
uint8_t setMode(uint8_t mode);
void onOff(int8_t channel, bool status);
int startBAT(int x);
int startST(int x);

size_t readLogCount = 0;   // stores log count between HTTP chunk callbacks
bool screenChanged = true; // not valid for multiple HTTP sessions - so ignore

const String PARAM_INPUT_CMD = "cmd";
const String PARAM_SCREEN_CMD = "screen";
const String PARAM_CLIENT_CMD = "clientID";
const String PARAM_SESSION_CMD = "sessionID";
const String PARAM_INPUT_1 = "value_1";
const String PARAM_INPUT_2 = "value_2";
#define CMD_TOGGLE 1
#define CMD_SCREEN 2
#define CMD_PAD 3
#define CMD_FADER 4
#define CMD_INAME 5
#define CMD_ONAME 6
#define OUTLEN 16

// cut down web traffic by only sending faders and names occasionally with level packets (unless a chanage has been flagged)
// this needs to be significantly 

#define UPDATESEVERY
#define SENDNAMESEVERY 4
#define SENDFADERSEVERY 3

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
int levelLoops = 0;

void webServerStart()
{      
  // Servicing for input and Output page requests (new screen)
  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request)
  {
    // screen changes
    screenChanged = true;	
	//Serial.print("/ ");
	_thisClient = getClient(request); // used in processor()
	_thisScreen = getScreen(request);
	slave[_thisClient].vChanged = VAL_ALL_CHGD; // need to update all parameters on the new screen on next /levels request
	//Serial.printf("/ from %i for screen %c\n", _thisClient, _thisScreen);
	//displayActiveSlaves();
	//printParams(request);
	//Serial.printf("Sending / to client %i\n", _thisClient);	
    slashRequest(request, _thisClient);  
	
	//Serial.print("Request from ");	
	//Serial.print(request->client()->remoteIP());
	//	Serial.printf(":R%i ",request->client()->remotePort());
	//	Serial.printf(":L%i\n",request->client()->localPort());
	//	Serial.printf(":L%i\n",request->client()->localPort());
    request->send_P(200, "text/html", index_html, processor);
  });
  
  // settings screen page 
  server.on("/settings", HTTP_ANY, [](AsyncWebServerRequest *request)
  {
    // screen changes
	 //Serial.print("/settings ");
	_thisClient = getClient(request);
	_thisScreen = getScreen(request);
	//printParams(request);
    slashRequest(request, _thisClient);     
    request->send_P(200, "text/html", settings_html, processor);
  });

  //fast GET from web client: send readings screenID and other data as needed 
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request)
  { 
    char scrn[2] = "M";      // default to main screen
  bool faderCh, padCh, namesCh;

  //Serial.print("/readings ");
 //   	printParams(request);
	_thisClient = getClient(request);
	_thisScreen = getScreen(request);
	//Serial.printf("Get Screen %c, client %i\n",_thisScreen, _thisClient);
	//printParams(request);
    //scrn[0]= currentWebScreen;
	strcpy((char *)&JSONstring, "{\n");
  makeJSONreads(_thisScreen, _thisClient); // may only have "Screen"
	//String temp = JSONstring;
 /*	Serial.print("JSON R|");
	Serial.println(JSONstring); 
	//Serial.print("|\n");
 */		

	strcat((char *)&JSONstring, "\n");
	makeJSONsets(_thisScreen, _thisClient); 
	strcat((char *)&JSONstring, "\n}");

	//Serial.print("JSON |");
	//Serial.println(JSONstring);
	//Serial.print("|\n");
	//dumpHex((void *)JSONstring,  strlen(JSONstring));
/*   */
    request->send(200, "text/plain", JSONstring);
  });

  // input controls from Web page
  // Process a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  // JS GET. process commands from web page - update sliders, buttons, change screen
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request)
  {
	  // Serial.print("/update ");
	  //printParams(request);
	 _thisClient = getClient(request);
	 _thisScreen = getScreen(request);
     webCommands(request);
     request->send(200, "text/plain", "OK");
  });
  
  // CSV data request
  server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request)
  {     
  // Serial.print("/log ");
   _thisClient = getClient(request);
    if (!readLogBegin()) 
    {
      request->send(200,"text/plain","No log data available");
      return;
    }
    
    AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
    //Write up to "maxLen" bytes into "buffer" and return the amount written.
    //index equals the amount of bytes that have been already sent
    //You will be asked for more data until 0 is returned. 
    //Keep in mind that you can not delay or yield waiting for more data!
    size_t r = 0;
    //Serial.printf("Chunk callback: maxLen=%d index=%d\n", maxLen, index );
#ifdef SAVEBATLOG
      if (_batLogIsMostRecent) {
        r = readBatLog((char *)buffer, maxLen);
      } else {
#endif
        r = readLog((char *)buffer, maxLen);
#ifdef SAVEBATLOG        
      }
#endif
      /*
      if (r == 0) {
        Serial.printf("Chunk read completed, read returned 0\n", r, maxLen);
      } else {
        Serial.printf("Chunk read returned %d bytes, buffer=%d bytes\n", r, maxLen);
      }
      */
      return r;
    });
    request->send(response);
    //Serial.printf("request->send executed\n");
  });
  
  // plot request
  server.on("/logJSON", HTTP_GET, [](AsyncWebServerRequest *request)
  {     
   // Serial.print("/logJSON ");
    _thisClient = getClient(request);
	int plotVar = 1;
	if (request->hasParam("plot"))     
       plotVar = (request->getParam("plot")->value()).toInt();
	makeLogJSON(plotVar); // 
	//Serial.println(JSONstring);
    request->send(200, "text/plain", JSONstring);
  });
  
// generic calls, no need to know which client requests.
  // load CSS, JS and icon files
  server.on("/myLoad.css", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(SPIFFS, "/myLoad.css", "text/css");
  });
  server.on("/myLoad.js", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(SPIFFS, "/myLoad.js", "text/javascript");
  });
    server.on("/jogDial.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(SPIFFS, "/jogDial.js", "text/javascript"); // "/jogDial.min.js"
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.ico", "text/plain");
  });
    server.on("/FileSaver.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/FileSaver.js", "text/javascript");
  });

  server.on("/base_bg.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/base_bg.png", "image/png");
  });
  server.on("/base_knob.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/base_knob.png", "image/png");
  });

  // Start server
  server.begin();
}

// getClient() and getScreen() may be from GET or POST requests. 
// All other variables are GET only

int getClient(AsyncWebServerRequest *request)
{
	String bufS, bufC;
	bool gotClient = false, gotSession = false;
	int session = -1, bufI;
	// find clientID parameter in the request
	// none? flag error
	// if the clientID == -1, register new client
	// return the clientID
	
	if(request->hasParam(PARAM_SESSION_CMD, true)) // POST 
	{
		//Serial.print(" SE_POST ");
		bufS = request->getParam(PARAM_SESSION_CMD, true)->value();	
		//bufI = request->getParam(PARAM_SESSION_CMD, true)->value();	
		gotSession = true;
		session = bufS.toInt();
	}
	
	if(request->hasParam(PARAM_SESSION_CMD)) // GET 
	{
		//Serial.print(" SE_GET ");
		bufS = request->getParam(PARAM_SESSION_CMD)->value();	
		//bufI = request->getParam(PARAM_SESSION_CMD)->value();	
		gotSession = true;
		session = bufS.toInt();
	}
	//Serial.printf("SessionID = |%s| %i\n", bufS.c_str(), session);
	_thisSession = session;
	
	/*
	if(request->hasParam(PARAM_CLIENT_CMD, true)) // POST 
	{
		//Serial.print(" CL_POST ");
		bufC = request->getParam(PARAM_CLIENT_CMD, true)->value();	
		gotClient = true;
	}
	
	if(request->hasParam(PARAM_CLIENT_CMD)) // GET 
	{
		//Serial.print(" CL_GET ");
		bufC = request->getParam(PARAM_CLIENT_CMD)->value();	
		gotClient = true;
	}
	*/
	
	 if (gotSession && session >= 0) // ignore requests with no legitimate sessionID attached
     {       
		 int i; // = bufC.toInt();
		//Serial.printf("Request from session [%i]\n", session);
		for(i = 0; i < MAXSLAVES; i++)
			if (slave[i].sessionID == session)
			{
				slave[i].status = SLAVE_ACTIVE;   // see ageSlaves()
				slave[i].lastMessage =  millis();
				// if master re-starts, re-enter parameters from live HTTP sessions.
				//slave[i].type = SLAVE_HTTP;
				//slave[i].slaveIP = UDPsourceIP = request->client()->remoteIP(); 
				return i; // client ID
			}
	 
			 // register as a new client	 
			 Serial.println("Registering new web client");		
			 return registerHTTPslave(request, session);		
	 }	 
	 return -1;
}

char getScreen(AsyncWebServerRequest *request)
{
	String ss;
	char screen;
	bool gotScreen = false;
	if (request->hasParam(PARAM_SCREEN_CMD)) // GET
    {
		 //Serial.print(" SC_GET ");
		 ss = request->getParam(PARAM_SCREEN_CMD)->value();	
		 gotScreen = true;
	}
		// convert string to character
	if(request->hasParam(PARAM_SCREEN_CMD, true)) // POST 
	{
		//Serial.print(" SC_POST ");
		ss = request->getParam(PARAM_SCREEN_CMD, true)->value();	
		gotScreen = true;
	}
	if(gotScreen)
	{
		screen = (ss.c_str())[0];	
		return screen;
	}		
	
	Serial.printf("Missing screen ID in request\n");
	return 'M';	// default to Input
}
// look for a spare slot, otherwise kick out first sleepy one
int registerHTTPslave(AsyncWebServerRequest *request, int session)
{ 
	int i;	
	// don't use IP address for HTTP slaves, as multiple browser tabs may be open
	for (i = 0; i < MAXSLAVES; i++) // try for empty slot
		if(slave[i].status == SLAVE_MT)
		{
			slave[i].type = SLAVE_HTTP;
			slave[i].sessionID = session;
			slave[i].slaveIP = UDPsourceIP = request->client()->remoteIP(); 
			slave[i].status = SLAVE_ACTIVE;   // see ageSlaves()
			slave[i].lastMessage =  millis();
			return i;
		}
/*		
	for (i = 0; i < MAXSLAVES; i++) // a dud one with 0.0.0.0 IP?
		if(slave[i].slaveIP[0] == 0)
		{
			slave[i].type = SLAVE_HTTP;
			slave[i].slaveIP = UDPsourceIP = request->client()->remoteIP(); 
			slave[i].status = SLAVE_ACTIVE;   // see ageSlaves()
			slave[i].lastMessage =  millis();
			return i;
		}
*/
	for (i = 0; i < MAXSLAVES; i++) // sleepy slave to use?
		if(slave[i].status == SLAVE_SLEEP)
		{
			slave[i].type = SLAVE_HTTP;
			slave[i].sessionID = session;
			slave[i].slaveIP = UDPsourceIP = request->client()->remoteIP(); 
			slave[i].status = SLAVE_ACTIVE;   // see ageSlaves()
			slave[i].lastMessage =  millis();
			return i;
		}			
    
	return -1; // failed, all slots full.
  
}
// service "/" request
// just upload the page (done in server.on() above)
// make sure the next /update includes screen info
void slashRequest(AsyncWebServerRequest *request, int client)
{
    char  cc;
    String bufS;
    //Serial.printf("Request / has %i params:\n",request->params());
    //printParams(request);
    // screen change - will be send with next /update JSON
    screenChanged = true; 
	  cc = getScreen(request);
    if (cc != 'X')
    { 
        currentWebScreen = cc;    
		  if (client >= 0)		
		    slave[client].vChanged = VAL_ALL_CHGD | VAL_CHGD_MASK; // send everything on next /update request
      //  Serial.printf("http / client %i requests screen |%c|\n", client, currentWebScreen);		
    }     
	else
		currentWebScreen = 'M';	// default for new page requests
}

// service "/update" request
// send JSON of required data
// always readings, screen info and other data as required.
void webCommands(AsyncWebServerRequest *request)
{
    int cs = currentWebScreen - '0'; // char to int conversion
    cs = constrain(cs, 0,7);
    int btn;
    char val1[4];
	/*
	bool pads = false, faders = false, names = false;
	Serial.printf("Request /update has %i params:\n",request->params());
    // printParams(request);
	if(slave[_thisClient].vChanged & (VAL_CNAME_MASK | VAL_ONAME_MASK))
		names = true;
	// this could be improved to reduce unnecessary updates
	if(slave[_thisClient].vChanged & (VAL_FADER_MASK | VAL_FADER_BLOCK_MASK | VAL_GAIN_MASK))
		pads = faders = true;
*/
	screenChanged = true; 
    String inputCmd = "No cmd.";
    String inputParam1 = "No param_1.";
    String inputParam2 = "No param_2.";
    char val_1[128] ="No val_1.";
    char val_2[128] ="No val_2.";
    char val_s[16] ="X";
    char scrnChar = 'X';
	int scrn;	//[0..7 or 29 for input]

   // all requests should have cmd and scn; may have value-1 and value_2 parameters.
    int cmd = -1;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
     if (!request->hasParam(PARAM_INPUT_CMD)) // every request should have this argument
     {
      Serial.println("Error: request missing CMD parameter");
      return;
     }
    
    inputCmd = request->getParam(PARAM_INPUT_CMD)->value();
    cmd = inputCmd.toInt();
    // all requests must have cmd and scrn parameters.
   // Serial.printf("/update: cmd=|%s|",inputCmd);

    if (request->hasParam(PARAM_INPUT_1)) 
    {
       inputParam1 = request->getParam(PARAM_INPUT_1)->value();
       inputParam1.toCharArray(val_1, 128);
       
       // val_1 format: "pad_X" 
       strncpy((char *)&val1, val_1, 3);  // first three chars tell object type
       btn = val_1[4] - '0' ;  // index to button array  
       //Serial.printf(" value_1=|%s|, btn %i; ",val_1, btn);
    }
    if (request->hasParam(PARAM_INPUT_2)) 
    {
      inputParam2 = request->getParam(PARAM_INPUT_2)->value();
      inputParam2.toCharArray(val_2, 128);
      //Serial.printf(" value_2=|%s|; ",val_2);
    }
	  char ssc = getScreen(request);
    if (ssc != 'X') // every request should have this argument
    {
      //inputParam2 = request->getParam(PARAM_SCREEN_CMD)->value();
      //inputParam2.toCharArray(val_s, 16);
      scrnChar = ssc;
	    scrn = scrnChar - '0';
     // Serial.printf(" screen=|%s|%c| \n",val_s, scrnChar);
    }
    else
    {
      Serial.println("Error: request missing SCREEN parameter");
      return;
    }
	float tempX =  atof(val_2);

	// process the commands 
    uint8_t dummy, * vp = &dummy;
	// main screen
	if(strcmp(val_1, "ampsSID") == 0)
		pSet.current = constrain(tempX, IDROPOUT, iMaxOp);
	if(strcmp(val_1, "voltsSID") == 0)
		pSet.voltage = constrain(tempX, VDROPOUT, VMAXOP);	
	if(strcmp(val_1, "powerSID") == 0)
		pSet.power = constrain(tempX, PDROPOUT, PMAXOP);
	if(strcmp(val_1, "resistSID") == 0)
		pSet.resistance = constrain(tempX, RDROPOUT, RMAXOP);

	if(strcmp(val_1, "logtSID") == 0)
		pSet.logTime = constrain(tempX, 0, MAXLOGTIME);

	// Step screen - S1
	if(strcmp(val_1, "volts1SID") == 0)	
		pSet.s[0].volts = constrain(tempX, VDROPOUT, VMAXOP);
	if(strcmp(val_1, "amps1SID") == 0)	
		pSet.s[0].amps = constrain(tempX, IDROPOUT, iMaxOp);
	if(strcmp(val_1, "steps1SID") == 0)	
		pSet.s[0].steps = constrain(tempX, IDROPOUT, iMaxOp);
	if(strcmp(val_1, "time1SID") == 0)	
		pSet.s[0].time = constrain(tempX, 0, MAXUINTVAL);	
	// Step screen - S2
	if(strcmp(val_1, "volts2SID") == 0)	
		pSet.s[1].volts = constrain(tempX, VDROPOUT, VMAXOP);
	if(strcmp(val_1, "amps2SID") == 0)	
		pSet.s[1].amps = constrain(tempX, IDROPOUT, iMaxOp);
	if(strcmp(val_1, "steps2SID") == 0)	
		pSet.s[1].steps = constrain(tempX, IDROPOUT, iMaxOp);
	if(strcmp(val_1, "time2SID") == 0)	
		pSet.s[1].time = constrain(tempX, 0, MAXUINTVAL);
	
	if(strcmp(val_1, "riseSID") == 0)	
		pSet.slew = constrain(tempX, 0, MAXUINTVAL);
	if(strcmp(val_1, "cycSID") == 0)	
		pSet.cycles = constrain(tempX, 0, MAXUINTVAL);
	
	// mode buttons
	if(strcmp(val_1, "CC") == 0)	
		setMode(MODE_CC);
	if(strcmp(val_1, "CV") == 0)	
		setMode(MODE_CV);
	if(strcmp(val_1, "CP") == 0)	
		setMode(MODE_CP);
	if(strcmp(val_1, "CR") == 0)	
		setMode(MODE_CR);
	
	// bmode buttons
	if(strcmp(val_1, "BT") == 0)	
	{
		startBAT(1);
		//setModeB(MODE_BAT);
	}
	if(strcmp(val_1, "ST") == 0)	
	{
		startST(1);
		//setModeB(MODE_ST);
	}
	if(strcmp(val_1, "NOR") == 0)	
		setModeB(MODE_NOR);
	
	// on/off buttons
	if(strcmp(val_1, "ON") == 0)	
		onOff(0, true);
	if(strcmp(val_1, "OFF") == 0)	
		onOff(0, false);

   // KMOde
   if(strcmp(val_1, "KA") == 0)  
    pSet.vMode = 'A';
   if(strcmp(val_1, "KM") == 0)  
    pSet.vMode = 'M';
   if(strcmp(val_1, "KK") == 0)  
    pSet.vMode = 'K';
	
//Serial.println("]");	
	valChanged(VAL_CHGD_REMOTE);	// update on the console screen
    
}
	/*
    switch (cmd)    
    {
      case CMD_TOGGLE : // No longer used.        
        Serial.print(" General toggle change.");  
        break;
      case CMD_SCREEN : // 2 - do not use for screen change - now handled in slashRequest()       
        break;
      case CMD_PAD : // 3  - decode value_1 for onb, sol or pad 
		//valChanged(VAL_CHGD_BCAST | VAL_GAIN_MASK | VAL_FADER_BLOCK_MASK | VAL_EE_MASK, _thisClient);
		//Serial.print(" Pad change. ");
		// which variable to change?		
        if(val_1[0] == 'p' && btn > 3) // Pad: no settable pads on the first 4 channels
		{
			;//Serial.print(" PAD ");
			//gains.inpad = (atoi(val_2) == 1) ? bitSet(gains.inpad , btn) : bitClear(gains.inpad, btn);          
		}
        if(val_1[0] == 's') // SOLO
		{
			;//Serial.print(" SOL ");
			//faderInfo.pfl[scrn] = (atoi(val_2) == 1) ? bitSet(faderInfo.pfl[scrn], btn) : bitClear(faderInfo.pfl[scrn], btn);          
		}
        if(val_1[0] == 'm') 
		{
			//Serial.print(" MUT ");
			// MUTE: channel OFF = mute ON - inverted setting logic
			if(scrnChar > '8') // Input screen
				;//gains.IchOn = (atoi(val_2) == 1) ? bitClear(gains.IchOn , btn) : bitSet(gains.IchOn, btn);					
			else
				;//faderInfo.chOn[scrn] = (atoi(val_2) == 1) ? bitClear(faderInfo.chOn[scrn] , btn) : bitSet(faderInfo.chOn[scrn], btn);				
		}

       // Serial.printf(" Scrn %i, Channel %i, Inpad 0x%2x, IchOn 0x%2x | scrn %c, pfl 0x%2x, OchOn 0x%2x\n", 
	   //				scrn, btn, gains.inpad, gains.IchOn, scrnChar, faderInfo.pfl[scrn], faderInfo.chOn[scrn] );         
        break;
      case CMD_FADER : //4
        //Serial.printf("Fader change. scrn [%c,%i], fader %i,  val %2.3f\n",  scrnChar, cs, btn, atof(val_2));  
        if(scrnChar == 'N') // change gain        
        {  
		  //Serial.print("-G-");
		  //gains.gain[btn] = atof(val_2); 
		 // valChanged(VAL_CHGD_BCAST | VAL_GAIN_MASK | VAL_EE_MASK, _thisClient);
		}
        else // 0..7
        {
		  //Serial.print("-F-");
		 // valChanged(VAL_CHGD_BCAST | VAL_FADER_BLOCK_MASK | VAL_EE_MASK, _thisClient);
          if(btn < INCHANS)
           ;//  faderInfo.fader[btn][cs] = atof(val_2);
           else // will be the Master fader
           ; // faderInfo.mFader[cs]= atof(val_2);
        }
        break;
      case CMD_INAME : // 5
		val_2[NAMELEN-1] ='\0'; //truncate to legal length
       // Serial.print("Input Name change."); 
		//valChanged(VAL_CHGD_BCAST | VAL_CNAME_MASK | VAL_EE_MASK, _thisClient);
        if(btn < INCHANS) // don't change Master
          //strcpy(inChan.channelName[btn], val_2);
        break;
      case CMD_ONAME : // 6
	   // val_2[NAMELEN-1] ='\0'; //truncate to legal length
	  //  valChanged(VAL_CHGD_BCAST | VAL_ONAME_MASK| VAL_EE_MASK, _thisClient);
        //Serial.print("Output Name change.");          
      //  strcpy(outChan.name[cs], val_2);
        break;
      default:
        Serial.printf("Unknown command %i\n",cmd);
    }
*/

/* JSON
 * Readings only
 * All variable names are of the form "ccccIJ". J is context optional. cccc is generally the name of the HTML id of the control, I is the control number usually [0..7]
 */
 // just the core JSON lines. Calling routine needs to add leading { and trailing , or }
void makeJSONreads(char screen, int client)
{
  char nums[16];  
  char scrStr[20] = "0";
 // bool pads = false, faderGains = false, names = false;
	//Serial.printf("Request /update has %i params:\n",request->params());
	// printParams(request);
	//Serial.printf("/levels to %i: flags 0x%4x: ", client, slave[client].vChanged);
	//printClientChanges(client);
/*
	if(slave[client].vChanged & (VAL_CNAME_MASK | VAL_ONAME_MASK))
	{
		//names = true;
		//doneChange(VAL_CNAME_MASK | VAL_ONAME_MASK, client);
		Serial.printf("/readings NAMES to %i: flags 0x%4x\n", client, slave[client].vChanged);
	}
	// this could be improved to reduce unnecessary updates
	if(slave[client].vChanged & (VAL_FADER_MASK | VAL_FADER_BLOCK_MASK | VAL_GAIN_MASK))
	{
		//pads = faderGains = true;
		//doneChange(VAL_FADER_MASK | VAL_FADER_BLOCK_MASK | VAL_GAIN_MASK, client);
		Serial.printf("/readings FADER GAIN PADS to %i: flags 0x%4x\n", client, slave[client].vChanged);
	}
*/
	//Serial.printf("faders & gains %i, pads %i, names %i\n", faderGains, pads, names);
  // screen first
  // only add these immediately after a screen change (or name update?)
  // or pending name changes on web page will be overwritten
  //char sc = screen[0];
  
  int scn = screen - '0'; // integer version of screen 
  if(scn < 0 || scn > INCHANS) 
    scn = -1;
  scrStr[0] = screen;
	
  //strcpy((char *)&JSONstring, "{\n");
  /*
  strcat((char *)&JSONstring, "\"clientID\":");
  strcat((char *)&JSONstring,itoa(client, (char *)&nums, 10));
  strcat((char *)&JSONstring, ",\n");
  */
  
  //calcOutLevels(scn); // value will be > INCHANS for iNput screen
  //printOutlevels();
 
	strcat((char *)&JSONstring, "\"screen\":\"");
	strcat((char *)&JSONstring, scrStr);
	strcat((char *)&JSONstring, "\",\n");
	
	strcat((char *)&JSONstring, "\"device\":\"");
	strcat((char *)&JSONstring, myID.instName);
	strcat((char *)&JSONstring, "\"");
	
	if(screen =='M')
	{
		strcat((char *)&JSONstring, ",\n\"voltsR\" : ");
		sprintf((char *)&nums, "%3.3f",  meas.ADSvolts); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"ampsR\" : ");
		sprintf((char *)&nums, "%3.3f",  meas.ADSamps); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"powerR\" : ");
		sprintf((char *)&nums, "%3.3f",  meas.ADSamps * meas.ADSvolts); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"resistR\" : ");
		float res; 
		res = (meas.ADSamps < 0.01) ? 0.0 : meas.ADSvolts/meas.ADSamps;
		res = constrain(res, 0.0, RMAXOP);
		sprintf((char *)&nums, "%3.3f",  res ); 
		strcat((char *)&JSONstring, nums);
		//strcat((char *)&JSONstring,"");
	}
	if(screen =='S')
	{
		; // nothing to read
	}
	// closing brace
	//strcat((char *)&JSONstring, "\n}");
    
  // Serial.printf("JSON-reads: scrn %c |%s| %i\n", screen, JSONstring, strlen(JSONstring));  
  // dumpHex((void *)JSONstring,  strlen(JSONstring));
    screenChanged = false;
}

// just the core JSON lines. Calling routine needs to add leading { and trailing , or }
void makeJSONsets(char screen, int client)
{
  char nums[16];  
  char scrStr[20] = "0";
//  bool pads = false, faderGains = false, names = false;
	//Serial.printf("Request /update has %i params:\n",request->params());
	// printParams(request);
	//Serial.printf("/levels to %i: flags 0x%4x: ", client, slave[client].vChanged);
	//printClientChanges(client);
	if(slave[client].vChanged & (VAL_CNAME_MASK | VAL_ONAME_MASK))
	{
		//names = true;
		//doneChange(VAL_CNAME_MASK | VAL_ONAME_MASK, client);
		;//Serial.printf("/readings NAMES to %i: flags 0x%4x\n", client, slave[client].vChanged);
	}
	// this could be improved to reduce unnecessary updates
	if(slave[client].vChanged & (VAL_FADER_MASK | VAL_FADER_BLOCK_MASK | VAL_GAIN_MASK))
	{
		//pads = faderGains = true;
		//doneChange(VAL_FADER_MASK | VAL_FADER_BLOCK_MASK | VAL_GAIN_MASK, client);
		;//Serial.printf("/readings FADER GAIN PADS to %i: flags 0x%4x\n", client, slave[client].vChanged);
	}
	//Serial.printf("faders & gains %i, pads %i, names %i\n", faderGains, pads, names);
  // screen first
  // only add these immediately after a screen change (or name update?)
  // or pending name changes on web page will be overwritten
  //char sc = screen[0];
  
  int scn = screen - '0'; // integer version of screen 
  if(scn < 0 || scn > INCHANS) 
    scn = -1;
  scrStr[0] = screen;
	
  //strcpy((char *)&JSONstring, "{\n");
  /*  
  strcat((char *)&JSONstring, "\"clientID\":");
  strcat((char *)&JSONstring,itoa(client, (char *)&nums, 10));
  strcat((char *)&JSONstring, ",\n");
*/
  
  //calcOutLevels(scn); // value will be > INCHANS for iNput screen
  //printOutlevels();
	
	/* // makeJSONreads always used with makeJSONsets.
	 strcat((char *)&JSONstring, "\"screen\":\"");
	 strcat((char *)&JSONstring, scrStr);
	 strcat((char *)&JSONstring, "\",\n");
	 
	 strcat((char *)&JSONstring, "\"device\":\"");
	 strcat((char *)&JSONstring, myID.instName);
	 strcat((char *)&JSONstring, "\"\n");
	 */
	 if(screen =='M')
	 {
		 // value settings
		strcat((char *)&JSONstring, ",\"voltsS\" : ");
		sprintf((char *)&nums, "%3.3f",  pSet.voltage); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"ampsS\" : ");
		sprintf((char *)&nums, "%3.3f",  pSet.current); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"powerS\" : ");
		sprintf((char *)&nums, "%3.3f",  pSet.power ); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"resistS\" : ");
		sprintf((char *)&nums, "%3.3f",  pSet.resistance); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"logtS\" : ");
		sprintf((char *)&nums, "%i",  pSet.logTime ); 
		strcat((char *)&JSONstring, nums);
		
		// mode buttons
		strcat((char *)&JSONstring, ",\n\"CV\" : ");
		sprintf((char *)&nums, "%i",  inMode[MODE_CV]); 
		strcat((char *)&JSONstring, nums);
		
		strcat((char *)&JSONstring, ",\n\"CC\" : ");
		sprintf((char *)&nums, "%i",  inMode[MODE_CC]); 
		strcat((char *)&JSONstring, nums);
		
		strcat((char *)&JSONstring, ",\n\"CR\" : ");
		sprintf((char *)&nums, "%i",  inMode[MODE_CR]); 
		strcat((char *)&JSONstring, nums);
		
		strcat((char *)&JSONstring, ",\n\"CP\" : ");
		sprintf((char *)&nums, "%i",  inMode[MODE_CP]); 
		strcat((char *)&JSONstring, nums);

		// modeB buttons
		strcat((char *)&JSONstring, ",\n\"ST\" : ");
		sprintf((char *)&nums, "%i",  isModeB[MODE_ST]); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"BT\" : ");
		sprintf((char *)&nums, "%i",  isModeB[MODE_BAT]); 
		strcat((char *)&JSONstring, nums);
		
		strcat((char *)&JSONstring, ",\n\"NOR\" : ");
		sprintf((char *)&nums, "%i",  isModeB[MODE_NOR]); 
		strcat((char *)&JSONstring, nums);
		// On off buttons

		strcat((char *)&JSONstring, ",\n\"ON\" : ");
		sprintf((char *)&nums, "%i",  _outOn); 
		strcat((char *)&JSONstring, nums);
		
		strcat((char *)&JSONstring, ",\n\"OFF\" : ");
		sprintf((char *)&nums, "%i",  !_outOn); 
		strcat((char *)&JSONstring, nums);
    
    strcat((char *)&JSONstring, ",\n\"KEL\":");
    sprintf((char *)&nums, "\"%c\"", _cur_vMode ); 
    strcat((char *)&JSONstring, nums);
    
    strcat((char *)&JSONstring, ",\n\"KMODE\":");
    sprintf((char *)&nums, "\"%c\"", pSet.vMode ); 
    strcat((char *)&JSONstring, nums);
	 }
	 if(screen =='S')
	 {
		//s1
		strcat((char *)&JSONstring, ",\"volts1S\" : ");
		sprintf((char *)&nums, "%3.3f",  pSet.s[0].volts); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"amps1S\" : ");
		sprintf((char *)&nums, "%3.3f",  pSet.s[0].amps); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"time1S\" : ");
		sprintf((char *)&nums, "%i",  pSet.s[0].time ); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"steps1S\" : ");
		sprintf((char *)&nums, "%i",  pSet.s[0].steps); 
		strcat((char *)&JSONstring, nums);

		//s2
		strcat((char *)&JSONstring, ",\n\"volts2S\" : ");
		sprintf((char *)&nums, "%3.3f",  pSet.s[1].volts); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"amps2S\" : ");
		sprintf((char *)&nums, "%3.3f",  pSet.s[1].amps); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"time2S\" : ");
		sprintf((char *)&nums, "%i",  pSet.s[1].time ); 
		strcat((char *)&JSONstring, nums);

		strcat((char *)&JSONstring, ",\n\"steps2S\" : ");
		sprintf((char *)&nums, "%i",  pSet.s[1].steps); 
		strcat((char *)&JSONstring, nums);
		
		// other
		strcat((char *)&JSONstring, ",\n\"cycS\" : ");
		sprintf((char *)&nums, "%i",  pSet.cycles); 
		strcat((char *)&JSONstring, nums);	

		strcat((char *)&JSONstring, ",\n\"riseS\" : ");
		sprintf((char *)&nums, "%i",  pSet.slew); 
		strcat((char *)&JSONstring, nums);	
		//strcat((char *)&JSONstring,"");
	 }	 
	// ADD more here for MODE and MODEB
	
	// closing brace
	//strcat((char *)&JSONstring, "\n}");
    
   //Serial.printf("JSON-sets: scrn %c |%s| %i\n", screen, JSONstring, strlen(JSONstring));  
  //  dumpHex((void *)JSONstring,  strlen(JSONstring));
    screenChanged = false;
}
 

void printTail(char * str, int tailLen)
{
  int sl = strlen(str);
  if(sl  <  tailLen)
    tailLen = sl;
  str += (sl - tailLen);
  Serial.printf("Tail: ...|%s|\n",str);
}
void dumpHex(const void* data, size_t size) {
  char ascii[17];
  size_t i = 0, j;
  ascii[16] = '\0';
 Serial.printf("\n%3i: ", i);
  for (i = 0; i < size; ++i) {
    Serial.printf("%02X ", ((unsigned char*)data)[i]);
    if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
      ascii[i % 16] = ((unsigned char*)data)[i];
    } else {
      ascii[i % 16] = '_';
    }
    if ((i+1) % 8 == 0 || i+1 == size) {
      Serial.printf(" ");
      if ((i+1) % 16 == 0) {
        Serial.printf("|  %s \n%3i: ", ascii, i+1);
      } else if (i+1 == size) {
        ascii[(i+1) % 16] = '\0';
        if ((i+1) % 16 <= 8) {
          Serial.printf(" ");
        }
        for (j = (i+1) % 16; j < 16; ++j) {
          Serial.printf("   ");
        }
        Serial.printf("|  %s \n", ascii);
      }
    }
  }
}
void printParams(AsyncWebServerRequest *request)
{
	int params = request->params();
	Serial.printf(" Request has %i params\n", params);
	for(int i=0;i<params;i++)
	{
	  AsyncWebParameter* p = request->getParam(i);
	  if(p->isFile()) //p->isPost() is also true
	  {
		Serial.printf(" FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
	  } else 
		  if(p->isPost())
		  {
			Serial.printf(" POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
		  } else 
		  {
			Serial.printf(" GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
		  }
	}
}
void printParamsX(AsyncWebServerRequest *request)
{
  int paramsNr = request->params();

  Serial.printf("Request has %i params:\n",paramsNr);
  for(int i=0;i<paramsNr;i++)
  { 
     AsyncWebParameter* p = request->getParam(i);
 
     Serial.print("Name: |");
     Serial.print(p->name());
 
     Serial.print("| Value: |");
     Serial.print(p->value());    
     Serial.println("|");
  }
   Serial.println("------");
}
// levels = recursion depth
void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
              Serial.printf("Recursive to %s\n",file.name());
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}
// for PLOT only.
#define LOGSHORT 10 // short log view has this many points
#define DECIMATE 20 // maximum points to send - select periodic points to include
void makeLogJSON(int whichPlot)
{
	String logString;
	char nums[16];
	float min1 =9999 , max1 = 0, min2 = 9999, max2 = 0, min3 = 9999, max3 = 0;
	int i, j, steps, start; 
	start = 1;
	steps = _logCount;
	if (whichPlot == 2 && _logCount > LOGSHORT)
	{
	   start = _logCount - LOGSHORT;	
	   steps = LOGSHORT;
	}
   
	//Serial.println("makelogJSON");
	// adjust this ******* setting on same scale as volts,
	for(i = start; i <= _logCount; i++)
	{
		int j = (_logHead + i) % MAXLOG;
		min1 = min(min1, logData[j].volts);
		max1 = max(max1, logData[j].volts);
		
		min2 = min(min2, logData[j].amps);
		max2 = max(max2, logData[j].amps);
		
		min3 = min(min3, logData[j].setting); 
		max3 = max(max3, logData[j].setting);
	}
	logString = "{ ";
	logString += "\"logLength\" : ";
	logString += itoa(_logCount, nums,10);
	logString += ",\n";
	logString += "\"points\" : ";
	logString += itoa(steps, nums,10);
	logString += ",\n";
//		Serial.print(" 1 ");
	logString += "\"label1\" : \"Volts\",\n";
	logString += "\"label2\" : \"Amps\",\n";
	logString += "\"label3\" : \"Setting\",\n";
	logString += "\"min1\" : ";
	logString += min1;
	logString += ",\n\"max1\" : ";
	logString += max1;
	logString += ",\n\"min2\" : ";
	logString += min2;
	logString += ",\n\"max2\" : ";
	logString += max2;
	logString += ",\n\"min3\" : ";
	logString += min3;
	logString += ",\n\"max3\" : ";
	logString += max3;
	logString += ",\n\"data1\" : [";
//	Serial.print(" 2 ");
  vTaskDelay(1);  // hand control to FreeRTOS to avoid watchdog trigger
	for(i = start; i < _logCount; i++)
	{
		j = (_logHead + i) % MAXLOG;
		logString += logData[j].volts;
		if (i < _logCount -1)
			logString += ", ";
	}
  vTaskDelay(1);  // hand control to FreeRTOS to avoid watchdog trigger
	//	Serial.print(" 3 ");
	logString += "],\n\"data2\" : [";
	for(i = start; i < _logCount; i++)
	{
		j = (_logHead + i) % MAXLOG;
		logString += logData[j].amps;
		if (i < _logCount -1)
			logString += ", ";
	}
	logString += "],\n\"data3\" : [";
	for(i = start; i < _logCount; i++)
	{
		j = (_logHead + i) % MAXLOG;
		logString += logData[j].setting;
		if (i < _logCount -1)
			logString += ", ";
	}
	logString += "] }";	
	//	Serial.print(" 4 ");
	strcpy (JSONstring, logString.c_str());
  vTaskDelay(1);  // hand control to FreeRTOS to avoid watchdog trigger
}
/*
// CSV log data
// reuse JSONstring buffer
void makeLog(void)
{
  char nums[32];  
 // Serial.println("Starting makeLog");
  strcpy((char *)&JSONstring, "DC Load Log\nProject:\nDate:\nOperator:\n");
  sprintf((char *)&nums, "Entries, %i\n",_logCount);
  strcat((char *)&JSONstring, nums);
  strcat((char *)&JSONstring, "Time,Voltage,Current,Setting,Mode,Function\n");
  
  for(int i = 1; i <= _logCount; i++)
  {	 
  	int j = (_logHead + i) % MAXLOG;
  	sprintf((char *)&nums, "%i,",logData[j].time);
  	strcat((char *)&JSONstring, nums);
  
  	sprintf((char *)&nums, "%3.2f,",logData[j].volts);
  	strcat((char *)&JSONstring, nums);
  
  	sprintf((char *)&nums, "%3.2f,",logData[j].amps);
  	strcat((char *)&JSONstring, nums);
  	
  	sprintf((char *)&nums, "%3.2f,",logData[j].setting);
  	strcat((char *)&JSONstring, nums);
  	
  	sprintf((char *)&nums, "%s,",   logData[j].mode);
  	strcat((char *)&JSONstring, nums);
  	
  	sprintf((char *)&nums, "%s\n",   logData[j].function);
  	strcat((char *)&JSONstring, nums);	
    
    vTaskDelay(1);  // hand control to FreeRTOS to avoid watchdog trigger
  }
}
*/

// start reading log data
// must be called once before readLog 
// returns false if log data is empty
bool readLogBegin(void) {
  if (_logCount < 1)
    return false;
  readLogCount = 1;
}

/* CSV log data
 * Read log data and write into buffer up to maxLen
 * repeated calls to this function provides
 * log data (CSV format) in chunks suitable for sending to web client
 * Note: Disable all Serial.print instructions for production version
 */
size_t readLog(char* buffer, size_t maxLen) {
  size_t buflen = 0; size_t newBuflen = 0;
  char nums[32];

  //Serial.printf("readLog start: readLogCount=%d maxLen=%d\n", readLogCount, maxLen);
  
  // all log entries have been processed
  if (readLogCount >= _logCount)  // all log entries have been processed
  {
    return 0;
  }

  if (readLogCount==1) // first call -> write header
  {
    strcpy((char *)&JSONstring, "DC Load Log\nProject:\nDate:\nOperator:\n");
    sprintf((char *)&nums, "Entries: %i\n",_logCount);
    strcat((char *)&JSONstring, nums);
    strcat((char *)&JSONstring, "Time,Voltage,Current,Setting,Mode,Function\n");
    //Serial.printf("readLog: add header, %d bytes\n", strlen(JSONstring) );
  } 
  else // subsequent calls -> prepare empty JSONstring
  {
    JSONstring[0] = 0;
  }

  // itereate through log entries
  // starting where we left off previously
  for(int i = readLogCount; i <= _logCount; i++) 
  {
    int j = (_logHead + i) % MAXLOG;
    sprintf((char *)&nums, "%i,",logData[j].time);
    strcat((char *)&JSONstring, nums);

    sprintf((char *)&nums, "%3.2f,",logData[j].volts);
    strcat((char *)&JSONstring, nums);

    sprintf((char *)&nums, "%3.2f,",logData[j].amps);
    strcat((char *)&JSONstring, nums);
  
    sprintf((char *)&nums, "%3.2f,",logData[j].setting);
    strcat((char *)&JSONstring, nums);
  
    sprintf((char *)&nums, "%s,",   logData[j].mode);
    strcat((char *)&JSONstring, nums);
  
    sprintf((char *)&nums, "%s\n",   logData[j].function);
    strcat((char *)&JSONstring, nums);

    newBuflen = buflen + strlen(JSONstring);
    // can the buffer take the new string large?
    if (newBuflen < maxLen)
    { // yes - 
      strcpy(buffer+buflen, JSONstring);    // add new entry to buffer
      //Serial.printf("readLog: [%d] %s", i, JSONstring );
      JSONstring[0] = 0;      // clear string
      buflen = newBuflen;     // update buffer length
      readLogCount++;
    } 
    else 
    { // no space for this entry -> return
      // if maxLen is too small for a single log entry .... (shouldn't happen)
      // I've seen this situation arise with lots of console logging active 
      if (buflen == 0) { // no entries in buffer
        //Serial.printf("readLog: insufficient buffer size\n");
        // return a single byte and hope for bigger buffer on next callback
        // need to return > 0 bytes to get more callbacks for remaining log entries
        strcpy(buffer, " ");
        buflen = 1;
      }
      //Serial.printf("readLog return: readLogCount=%d [%d/%d]\n", readLogCount, buflen, maxLen);
      return buflen;
    }
  }
  // if foor loop ends 
  // all log entries have been processed
  readLogCount = _logCount;
  //Serial.printf("readLog end: [%d/%d]\n", buflen, maxLen);
  return newBuflen;
}

#ifdef SAVEBATLOG
/*
 * Read Battery log data and write into buffer up to maxLen
 * repeated calls to this function provides
 * log data (CSV format) in chunks suitable for sending to web client
 * Note: Disable all Serial.print instructions for production version
 */
size_t readBatLog(char* buffer, size_t maxLen) {
  size_t buflen = 0; size_t newBuflen = 0;
  char nums[32];
  //Serial.printf("readLog start: readLogCount=%d maxLen=%d\n", readLogCount, maxLen);
  
  if (readLogCount >= _batLogCount)  // all log entries have been processed
  {
    return 0;   // signal no further callbacks required 
  }

  if (readLogCount==1) // first call -> write header
  {
    strcpy((char *)&JSONstring, "DC Load,Battery Test\nProject:\nDate:\nOperator:\n");
    sprintf((char *)&nums, "Entries:,%i\n",_batLogCount);
    strcat((char *)&JSONstring, nums);
    strcat((char *)&JSONstring, "Time[s],OC Voltage[V],Current[A],ESR[Ohm],mAh,mWh\n");
    //Serial.printf("readLog: add header, %d bytes\n", strlen(JSONstring) );
  } 
  else // subsequent calls -> prepare empty JSONstring
  {
    JSONstring[0] = 0;
  }

  // itereate through battery log entries
  // starting where we left off previously
  for(int i = readLogCount; i <= _batLogCount; i++) 
  {
    int j = (_logHead + i) % MAXLOG;
    sprintf((char *)&nums, "%i,",batLogData[j].time);
    strcat((char *)&JSONstring, nums);

    sprintf((char *)&nums, "%3.2f,",batLogData[j].ocvolts);
    strcat((char *)&JSONstring, nums);

    sprintf((char *)&nums, "%3.2f,",batLogData[j].amps);
    strcat((char *)&JSONstring, nums);
  
    sprintf((char *)&nums, "%3.2f,",batLogData[j].esr);
    strcat((char *)&JSONstring, nums);
  
    sprintf((char *)&nums, "%.3f,",   batLogData[j].mah);
    strcat((char *)&JSONstring, nums);
  
    sprintf((char *)&nums, "%.3f\n", batLogData[j].mwh);
    strcat((char *)&JSONstring, nums);

    newBuflen = buflen + strlen(JSONstring);
    // sufficient buffer for new string?
    if (newBuflen < maxLen)
    { // yes - 
      strcpy(buffer+buflen, JSONstring);    // add new entry to buffer
      //Serial.printf("readBatLog: [%d] %s", i, JSONstring );
      JSONstring[0] = 0;      // clear string
      buflen = newBuflen;     // update buffer length
      readLogCount++;
    } 
    else 
    { // no space for this log entry -> return
      // if maxLen is too small for a single log entry .... (shouldn't happen)
      // I've seen this situation arise with lots of console logging active 
      if (buflen == 0) { // no entries in buffer
        //Serial.printf("readLog: insufficient buffer size\n");
        // return a single byte and hope for bigger buffer on next callback
        // must return > 0 bytes to get more callbacks for remaining log entries
        strcpy(buffer, " ");
        buflen = 1;
      }
      //Serial.printf("readBatLog return: readLogCount=%d [%d/%d]\n", readLogCount, buflen, maxLen);
      return buflen;
    }
  }
  // if foor loop ends 
  // all log entries have been processed
  readLogCount = _batLogCount;
  //Serial.printf("readBatLog end: [%d/%d]\n", buflen, maxLen);
  return newBuflen;
}
#endif

#endif
