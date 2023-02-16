/* UDP packet Input and output
 * generic packet processing in myMpkt.h
 *
 * ESP32 only
 */
#ifndef MYMUDPPKT_H
#define MYMUDPPKT_H
int8_t getSerialBlock(int8_t pktType);
void unitHeartbeat(idPacket inID);
int8_t getSerialPreamble(void);
bool registerUDPslave(void);
void fixInpad(void);

// send complete fader and gain packet 
// used to update a newly signed on slave
// usually only sent by Master
bool sendAllFaders()
{
  Serial.print("All_Fader_send ");    
  // build a packet
 // faderInfo.pktType = PT_FADERBLOCK;
  //faderInfo.groupID = myID.groupID;
  //Serial.println(" - doing");
  doneChange(VAL_CHGD_BCAST, SLAVE_UDP);
  doneChange(VAL_FADER_BLOCK_MASK, SLAVE_UDP);
  return  true;//putUDP((char *)&faderInfo, sizeof(faderInfo), myBroadcastIP, UDP_PORT);
}

// broadcast an OUTPUT fader level packet (usually a Slave sends this)
bool sendOneFaderPkt(int8_t chan)
{  int in;
  Serial.printf("Single fader send %i\n", chan);
  //inFade.chanID = chan;
  //inFade.groupID = myID.groupID;
  //inFade.chOn = faderInfo.chOn[chan];    
  //inFade.pfl = faderInfo.pfl[chan] ; 
  //inFade.mFader = faderInfo.mFader[chan];
  for(in = 0; in < INCHANS; in++)
  {
    //inFade.fader[in] = faderInfo.fader[in][chan];
  }  
  doneChange(VAL_CHGD_BCAST, SLAVE_UDP);
  doneChange(VAL_FADER_MASK, SLAVE_UDP);
  return true;//putUDP((char *)&inFade, sizeof(inFade), myBroadcastIP, UDP_PORT);
}

// transmit an Input Gain packet to Slaves
bool sendGainPkt()
{
 // LevelPacket levp;
  //Serial.println("Gain_send ");
  gains.pktType = PT_GAINS;
  gains.groupID = myID.groupID;
  // copy from faderInfo
 // memcpy((void*)&gains.gain[0],&faderInfo.gain[0], sizeof(float) * INCHANS);
   //DumpHex((void *)&gains, 16);		
  doneChange(VAL_CHGD_BCAST, SLAVE_UDP);
  doneChange(VAL_GAIN_MASK, SLAVE_UDP);
  return putUDP((char *)&gains, sizeof(gains), myBroadcastIP, UDP_PORT);
}

// transmit an input levels packet - master to slaves only
bool sendLevelPkt()
{
    inLevels.pktType = PT_LEVELS; // to be sure
	inLevels.groupID = myID.groupID;
	// no doneChange here as this is sent each time the data is received from Teensy
    return putUDP((char *)&inLevels, sizeof(inLevels), myBroadcastIP, UDP_PORT);
}

void updateLevels(LevelPacket vp)
{
   // update masterIP as this packet only comes from the Master
   masterIP = UDPsourceIP;
   /* this is already done.
   for(int i = 0; i < INCHANS; i++)
      inLevels.level[i] = vp.level[i]; 
	inLevels.inpadT =   vp.inpadT;
	*/
}

bool sendCnames(void) // always broadcast
{
  // broadcast
 //Serial.print("CN_send ");
  // Serial.printf("inChan.channelName[0] '%s'\n",inChan.channelName[0]);
  doneChange(VAL_CHGD_BCAST, SLAVE_UDP);
  doneChange(VAL_CNAME_MASK, SLAVE_UDP);
  inChan.groupID = myID.groupID;
  return putUDP((char *)&inChan, sizeof(inChan), myBroadcastIP, UDP_PORT);
}

void updateCnames(void)
{
	// just update screen, values used directly
	drawButtons(highButton); // buttons need redrawing as cnames are displayed

  //Serial.printf("inChan.channelName[0] '%s'\n",inChan.channelName[0]);
  // nothing else to do as we use the incoming packet values directly.

}

void updateOnames(void)
{
	//valChanged(VAL_CHGD_MASK | VAL_ONAME_MASK  | VAL_EE_MASK);
	drawLegends(currentMenu); // Onames are displayed, so redraw them
	//Serial.println("ON");
    // nothing else to do as we use the incoming packet values directly.
}

bool sendOnames()
{
  // broadcast
  outChan.pktType = PT_ONAMES;
//Serial.printf("Oname_send ");
  doneChange(VAL_CHGD_BCAST, SLAVE_UDP);
  doneChange(VAL_ONAME_MASK, SLAVE_UDP);
  outChan.groupID = myID.groupID;
  return putUDP((char *)&outChan, sizeof(outChan), myBroadcastIP, UDP_PORT);
}

// transmit an ID packet - used as heartbeat and, initally, a prompt for the master to send out updated settings.
bool sendIDpkt(void)
{
  bool res = false;
  //Serial.print("ID_send ");
  idPacket idp;
  idp.pktType = PT_SLAVER;
  idp.groupID = myID.groupID;
  //idp.chanID = myID.prefOut;
 // idp.isMaster = isMaster;
  //int stx = strlen(myID.instName);
  //Serial.printf("A %i '%s' ", stx, myID.instName); delay(1000);
  strcpy((char *)&idp.name, myID.instName);//stx+1

  //Serial.printf("B '%s' ",idp.name); delay(1000);
  res = putUDP((char *)&idp, sizeof(idp), masterIP, UDP_PORT);
  //Serial.print("C "); delay(1000);

  return res;
}

// update the LAN SSID/Password
bool  sendSSIDpacket(void)
{
	bool res = false;	
	lanPkt.groupID = myID.groupID;
	strcpy(lanPkt.ssid, myID.local_ssid);
	strcpy(lanPkt.pass, myID.local_pass);
//Serial.printf("sendSSID: SS |%s|, PW |%s|. ", lanPkt.ssid, lanPkt.pass);
	res = putUDP((char *)&lanPkt, sizeof(lanPkt), masterIP, UDP_PORT);
	//Serial.println("Send done.");
	return res;
}

void updateGainIn()
{ 
// transmitted directly to Teensy if Master
}

// master keeping track of active slaves
void updateSlaveStatus()
{
  int i;
  for (i = 0; i < OUTCHANS; i++)
  {
    if(slave[i].status == SLAVE_MT) 
        break; // reached the end of the active list
    if(slave[i].lastMessage + SLAVETIMEOUT * 1000 <  millis())
      slave[i].status = SLAVE_SLEEP;    
   }    
}
// CHANNEL FADERS
// send this when a setting for a particular mon mix (OUTCHAN) is changed

// update the master fader matrix (faderInfo) for this channel
void updateFadersIn(faderPkt fp)
{
    faderInfo.mFader[fp.chanID] = fp.mFader;
  for (int i = 0; i < INCHANS; i++)
  {
    faderInfo.fader[i][fp.chanID] = fp.fader[i];     
    
  } 
  faderInfo.chOn[fp.chanID] = fp.chOn; 
  faderInfo.pfl[fp.chanID]  = fp.pfl;
  //gains.inpad = fp.inpad;
   // if (teensyConn) teensySendSlave(fp);
}
void updateFaderBlockIn(faderInfoPkt fb)
{

  //Serial.printf("FB packet\n");

}
void updateNet()
{
	//Serial.print("Copy new net credentials\n");	
	//Serial.printf("Copying SSID: SS |%s|, PW |%s|\n", lanPkt.ssid, lanPkt.pass);
	//delay(100);
	strcpy(myID.local_ssid, lanPkt.ssid);
	strcpy(myID.local_pass, lanPkt.pass);
	//Serial.printf("to SSID: SS |%s|, PW |%s|\n", myID.local_ssid, myID.local_pass);
	delay(100);
	//	feedLoopWDT();
}
		 // show message on 
void beginUDPPktHdrs()
{
	memcpy(&UDPoutBuf.preamble, PREAMBLE, PRELEN);
	//memcpy(&serialOutBuf.preamble, PREAMBLE, PRELEN);
	//memcpy(&shd.sPreamble, SERIALPREAMBLE, PRELEN);
	// no need to set preambles for incoming packets.
	//memcpy(&UDPinBuf.preamble, PREAMBLE, PRELEN);
	//memcpy(&serialIntBuf.preamble, PREAMBLE, PRELEN);
}
// no longer used
/*
void heartBeat(void)
{
  Serial.print("Heart: ");
  //Serial.println(myBroadcastIP);
  //sendCnames(); // anything that's broadcast will do
}


// register the slave if first packet, heartbeat otherwise
void unitHeartbeat(idPacket inID)
{   

    // live update to screen if we're displaying the same one
 
  return;

}
*/

// take action on an incoming packet 
// data is already in a structure of the correct type for processing
// do not re-transmit, just UPDATE local variables
uint8_t UDPpktActionIn(int8_t pType)
{
  char buf[128] = "Cycle power on unit to use\nnew WiFi settings";
  if(pType > 0)
	pSetA.WifiConnected = true; // any good packet == connected
  switch (pType)
  {
    case PT_FADERS :  // 0x10 set channel mix levels (can also be sent the other way for master controller override)
      //Serial.print("PF ");
	  if(isMaster)
	    valChanged(VAL_CHGD_BCAST | VAL_FADER_MASK  | VAL_EE_MASK, SLAVE_UDP);
      updateFadersIn(inFade);
      masterFaderUpdate = true;
      break;
    case PT_FADERBLOCK :  //0x11  update entire fader and gain array.
     //Serial.print("PFB ");
	if(isMaster)
	  valChanged(VAL_CHGD_BCAST | VAL_FADER_BLOCK_MASK| VAL_EE_MASK, SLAVE_UDP);
	else
	  dirtyScreen = true;
      // no need to do anything as block is copied directly over existing values
      break;
    case PT_LEVELS :  // 0x20 master to slaves - channel signal levels
      //Serial.print("PL:\n");
      updateLevels(inLevels);  
      gotLevels = true;
	  lastLevels = millis();
      //Serial.printf("level 0: %3.2f [%3.1fdB] 1: %3.2f [%3.1fdB], oload 0x%2x\n", inLevels.level[0], vol2DBv(inLevels.level[0]), inLevels.level[1], vol2DBv(inLevels.level[1]), inLevels.overload);     
      break;
    
    case PT_GAINS :  // 0x21 input gain settings 
       //Serial.print("PG "); 
	   if(isMaster)
	     valChanged(VAL_CHGD_BCAST | VAL_GAIN_MASK | VAL_EE_MASK, SLAVE_UDP);
	   updateGainIn();	  // fix slave/Teensy inpads
	   //slaveInpad = gains.inpad;	  
       masterGainUpdate = true;   
       break;
      
    case PT_CNAMES :  // 0x30 Input Channel names  
       //Serial.print("PCN ");
       // just update the screen
	   if(isMaster)
	     valChanged(VAL_CHGD_BCAST | VAL_CNAME_MASK | VAL_ONAME_MASK | VAL_EE_MASK, SLAVE_UDP);
       updateCnames(); // use the input packet data directly       
      break;
      
     case PT_ONAMES :  // 0x31 Output Channel names  
       //Serial.print("PON ");
	   if(isMaster)
	     valChanged(VAL_CHGD_BCAST | VAL_CNAME_MASK | VAL_ONAME_MASK | VAL_EE_MASK, SLAVE_UDP);
       updateOnames(); // use the input packet data directly       
       break; 
       
      case PT_SLAVER :  // 0x01 slave to master - register slave or update last heartbeat 
      if(isMaster == ISMASTER)
        {
          // if a new registration, send Cnames, Onames and full fader packet to all via UDP
          //Serial.print("PT_SLAVER ");
          registerUDPslave();          
        }       
       break;
	   
	  case  PT_NET :	// update network credentials
		// Serial.print("PT_NET ");
		 // update net credentials
		 updateNet();
		 // show message on screen
		 screenError(buf, ERR_BG_A, 5, false);	   
	     break;
    
    //PT_MFADERS, PT_FADERBLOCK - only transmitted by master to audio processor
    default: 
      Serial.printf("Bad UDP pkt being parsed 0x%x\n", pType);
      return PT_ERROR;
  }  
  return pType;
}

// regsiter or update slave status on heartbeat (PT_SLAVER message)
// if a new registration
bool registerUDPslave()
{
  int i;
  bool newSlave = false;
  for (i = 0; i < MAXSLAVES; i++)
  {
    // find an existing slave or the first free block
    // we never set status back to SLAVE_MT once registered, so the first SLAVE_MT is the end of the list
    if(UDPsourceIP == slave[i].slaveIP || slave[i].status == SLAVE_MT)
    {
      if(slave[i].status == SLAVE_MT) // must be a new slave 
      {    
          newSlave = true; // trigger sending of settings packets
          slave[i].slaveIP = UDPsourceIP; 
		  slave[i].type = SLAVE_UDP; 
		  //Serial.print("SLAVE new. ");
      }
      else 
        if(slave[i].status == SLAVE_SLEEP)
		{
           newSlave = true; // also want to send update packets if slave has come back online.
		  // Serial.print("SLAVE wakes. ");
		}

      // new slave or update existing the lastMessage and status    
      slave[i].status = SLAVE_ACTIVE;   // see ageSlaves()
      slave[i].lastMessage =  millis();
      break;      
    }
  }

  if(newSlave)
  {
    sendOnames(); // broadcasts
    sendCnames();
	sendGainPkt();
    sendAllFaders();
    return true;
  }
  if (i >= MAXSLAVES) // got to the end without finding this slave or a free slot
    return false;
}
void ageSlaves()
{
  int i;
  for (i = 0; i < MAXSLAVES; i++)
  {
    if((slave[i].status == SLAVE_ACTIVE) && ((slave[i].lastMessage + SLAVETIMEOUT * 1000) <=  millis()))  
    {  
      //Serial.print(" SS ");
      slave[i].status = SLAVE_SLEEP;   
    } 
	// HTTP duds may occur with IP = 0.0.0.0 and no type if master re-starts
	if((slave[i].status == SLAVE_SLEEP) && slave[i].slaveIP[0] == 0) 
		slave[i].status = SLAVE_MT;   
  }	
}
void displayActiveSlaves()
{
  int i;
  Serial.println("Slaves"); 
  for (i = 0; i < MAXSLAVES; i++)
  {
    if(slave[i].status != SLAVE_MT)
	{
		Serial.printf("  %i: session %i, type = 0x%2x, status %s, now %i, last message %i, flags 0x%04x, IP ", 
					i, slave[i].sessionID, slave[i].type, (slave[i].status == SLAVE_ACTIVE)? "ACTIVE":"SLEEP", millis()/1000, slave[i].lastMessage/1000,slave[i].vChanged );   
		Serial.println(slave[i].slaveIP);
	}
  }  
}

#endif