#ifndef  MYLLOG_H
#define  MYLLOG_H
/* Logging functions */
void addLogItem(float setting, char mode[], char func[], bool tsec = false);
//void addLogItem(float setting, char mode[], char func[], bool tsec = false);
// circular buffer of log entries

// regular timed log call 
bool addLog(bool mustLog)
{
	float setting;
	// time-driven -  exit if not a forced log entry, in auto mode, and not yet time 
	if(!mustLog && pSet.logTime == 0)
		return false;	// didn't log
		
	if(!mustLog && pSet.logTime > 0 &&(millis() - _lastLogged <  pSet.logTime)) // .logTime could be zero == auto
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
void addLogItem(float setting, char mode[], char func[], bool tsec)
//void addLogItem(float setting, char * mode, char * func)
{
	char modebuf[8];
	strcpy(modebuf, (_curState == BMODE_RUN)? modeNamesB[pSet.modeB] : "NOR" );  // is one of the special modes running?
	if (_logCount == MAXLOG)
	{
		// log is full so don't increment counter, just move head
		_logHead = ++_logHead % MAXLOG;
	}
	else
	{
		_logCount++; // fill next spot
	}
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
}
void restartLog(void)
{
	_tLogStarted = millis();
	_logCount = 0;
}
#endif