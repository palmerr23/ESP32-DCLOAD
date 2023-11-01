#ifndef  MYLLOG_H
#define  MYLLOG_H
/* Logging functions */
void addLogItem(float setting, const char mode[], const char func[], bool tsec = false);
//void addLogItem(float setting, char mode[], char func[], bool tsec = false);
// circular buffer of log entries

// regular timed log call 
bool addLog(bool mustLog)
{
	float setting;
	// time-driven -  exit if not a forced log entry, in auto mode, and not yet time 
	if(!mustLog && pSet.logTime == 0)
		return false;	// didn't log

  int lTime = pSet.logTime;
  if(lTime > 0 && lTime < MINLOG) // don't allow **actual** log interval to go below MINLOG
    lTime = MINLOG;
	if(!mustLog && lTime > 0 &&(millis() - _lastLogged <  lTime)) // .logTime could be zero == auto
		return false;	

	char modebuf[8];
	strcpy(modebuf, (_curState == BMODE_RUN)? modeNamesB[pSet.modeB] : "NOR" );  // is one of the special modes running?
	switch (pSet.mode)
	{
		case MODE_CV :
			setting = dynSet.voltage;
			break;

		case MODE_CC :
			setting = dynSet.current;
			break;
		case MODE_CP :
			setting = dynSet.power;
			break;			
		case MODE_CR :
			setting = dynSet.resistance;
			break;
	}
	bool secFlag = false;
	if(strncmp("BAT", modebuf,3) == 0)
		secFlag = true;
	addLogItem(setting, modeNames[pSet.mode], modebuf, secFlag);

	return true;
}
// time, volts and amps are always logged
// tsec (time is in seconds) default == false
// mode and func are SHORT SZTRINGS.
void addLogItem(float setting, const char mode[], const char func[], bool tsec)
//void addLogItem(float setting, char * mode, char * func)
{
	char modebuf[8];
	strcpy(modebuf, (_curState == BMODE_RUN)? modeNamesB[pSet.modeB] : "NOR" );  // is one of the special modes running?
	
	// tail
	int nextLog = (_logHead + _logCount ) % MAXLOG;

	_lastLogged = millis();

	if (tsec)
		logData[nextLog].time = (millis() - _tLogStarted)/1000;
	else
		logData[nextLog].time = millis() - _tLogStarted;
	
	if(pSet.modeB == MODE_BAT && _curState == BMODE_RUN)
	{
		//Serial.print("^");
		logData[nextLog].amps = batCurrent;
		logData[nextLog].volts = batCCvolts;
	}
	else    
	{	//Serial.print("~");
		logData[nextLog].amps = meas.ADSamps;
		logData[nextLog].volts = meas.ADSvolts;
	}

	logData[nextLog].setting = setting;
  strcpy(logData[nextLog].mode, mode);
  strcpy(logData[nextLog].function, func);	
  
  if (_logCount >= MAXLOG)
  {
    // log is full so don't increment counter, just move head
    _logHead = ++_logHead % MAXLOG;
  }
  else
  {
    _logCount++; // fill next spot
  }
}
void restartLog(void)
{
	_tLogStarted = millis();
	_logCount = 0;
}


/* Add log entry to battery log
 * ocvolts: Battery Open Circuit Voltage
 * amps: Discharge Amps
 * esr: ESR
 * mah: Battery capacity in milliAh
 * mwh: Battery capacity in millWh
 */
void addBatLogItem(float ocvolts, float ccvolts, float amps, float esr, float mah, float mwh) {
#ifdef SAVEBATLOG
  _batLogIsMostRecent = true;     // indicate bat log is latest log

  // tail
  int nextLog = (_batLogHead + _batLogCount ) % MAXBATLOG;
  batLogData[nextLog].time = (millis() - _tBatLogStarted)/1000;
  batLogData[nextLog].amps = amps;
  batLogData[nextLog].ocvolts = ocvolts;
  batLogData[nextLog].ccvolts = ccvolts;
  batLogData[nextLog].esr = esr;
  batLogData[nextLog].mah = mah;
  batLogData[nextLog].mwh = mwh;
  
  if (_batLogCount >= MAXBATLOG) 
  {
    // log is full so don't increment counter, just move head
    _batLogHead = ++_batLogHead % MAXBATLOG;
  } else {
    _batLogCount++; // fill next spot
  }
#endif
}
void printLog()
{
  int i, j;
    Serial.printf("Log [%i]:\n", _logCount);
  for(i =0; i < _logCount; i++)
  {
    j = (_logHead + i) % MAXLOG;
    Serial.printf("  T %i S, ", logData[j].time);
    Serial.printf("%3.3fV, ",logData[j].volts);
    Serial.printf("%3.3fA, ",logData[j].amps);  
    Serial.printf("%3.3f set, ",logData[j].setting);
    Serial.printf("Mode %s, ",logData[j].mode);    
    Serial.printf("Fn %s\n",logData[j].function);
  }  
}
void printBatLog()
{
int i, j;
    Serial.printf("BAT Log [%i]:\n", _batLogCount);
  for(i =0; i < _batLogCount; i++)
  { 
    j = (_batLogHead + i) % MAXBATLOG;    
    Serial.printf("  T %i S, ", batLogData[j].time);
    Serial.printf("%3.3f A, ", batLogData[j].amps);
    Serial.printf("%3.3f OC V, ",batLogData[j].ocvolts);
    Serial.printf("%3.3f CC V, ",batLogData[j].ccvolts);
    Serial.printf("%3.3f ESR, ",batLogData[j].esr);
    Serial.printf("%3.3f mAH, ",batLogData[j].mah);
    Serial.printf("%3.3f mWH\n",batLogData[j].mwh);
  }
}

// restart battery log
void restartBatLog(void) {
#ifdef SAVEBATLOG
  _tBatLogStarted = millis();
  _batLogCount = 0;
  _batLogHead = 0;
#endif
}
#endif
