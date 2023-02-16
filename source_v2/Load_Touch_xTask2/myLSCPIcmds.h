/*
SCPI service routines

*/
#ifndef  MYLSCPICMD_H
#define  MYLSCPICMD_H
void printSCPIerrQ(struct scpi_parser_context* context);
// ALL SCPI responses (SCPIoutbuf) must be \n terminated 
// error lines not \n terminated
// SCPI error.length must be set.

void printNumArgs(char *tbuf, scpi_numeric output_numeric)
{
	Serial.printf("Num Args error: ");
	Serial.printf("%s. Numeric args: ", tbuf);
	Serial.printf("length %zd, ", output_numeric.length);
	if(output_numeric.unit == NULL)
		Serial.print("No unit, ");
	else
		Serial.printf("unit |%s|, ", output_numeric.unit);
	Serial.printf("value %5.3f\n", output_numeric.value);
}

// Special error function - used by SCPI system
// sends all errors (clears queue)
scpi_error_t system_error(struct scpi_parser_context* ctx, struct scpi_token* command)
{
	struct scpi_error* error;
	while(ctx->error_queue_head != NULL)
	{
		error = scpi_pop_error(ctx);
		sprintf(SCPIoutBuf,"%i,\"%s\"\n",error->id, error->description);
		/*
		sprintf(SCPIoutBuf,"%i,\"",error->id);
		strncat(SCPIoutBuf, error->description, error->length); // description stored with no trailing \0.
		strcat(SCPIoutBuf, "\"\n");
		*/
		//Serial.printf("Sys Err: |%s|\n",SCPIoutBuf);
		SCPItrans(SCPIoutBuf);
	}
	scpi_free_tokens(command);
	return SCPI_SUCCESS;
}

/*
 * Respond to *IDN?
 */
scpi_error_t identify(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  scpi_free_tokens(command);
  sprintf(SCPIoutBuf,"%s,%s,%s,%i\n", IDN_MANUF, IDN_MODEL, myID.instName, IDN_FIRMWARE);
  SCPItrans(SCPIoutBuf);
 // Serial.println("OIC,Embedded SCPI Example,1,10");
  return SCPI_SUCCESS;
}

/******************** READ LIVE VALUES ***********************/
/* output voltage */
scpi_error_t get_voltage(struct scpi_parser_context* context, struct scpi_token* command)
{
  float voltage;
  voltage = meas.ADSvolts;
  sprintf(SCPIoutBuf,"%5.3f\n", voltage);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/* output current */
scpi_error_t get_current(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = meas.ADSamps;
  //Serial.printf("CCC %5.3f\n",valX);
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t get_power(struct scpi_parser_context* context, struct scpi_token* command)
{
  float watts;
  watts = meas.ADSamps * meas.ADSvolts;
  //Serial.printf("CCC %5.3f\n",watts);
  sprintf(SCPIoutBuf,"%8.4f\n", watts);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_resistance(struct scpi_parser_context* context, struct scpi_token* command)
{
  float ohms;
  if(meas.ADSamps < SMALL_DIFF)
		ohms = -1;
	else
  ohms =  meas.ADSvolts / meas.ADSamps;
  //Serial.printf("CCC %5.3f\n",ohms);
  sprintf(SCPIoutBuf,"%8.4f\n", ohms);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/* heatsink temperature */
scpi_error_t get_temp(struct scpi_parser_context* context, struct scpi_token* command)
{
  float temp;
  temp = meas.ESPtemp;
  sprintf(SCPIoutBuf,"%5.1f\n", temp);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_batt_esr(struct scpi_parser_context* context, struct scpi_token* command)
{
  float temp;
  temp = meas.ESPtemp;
  sprintf(SCPIoutBuf,"%5.2f\n", esr);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_batt_capacity(struct scpi_parser_context* context, struct scpi_token* command)
{
  float temp;
  temp = meas.ESPtemp;
  sprintf(SCPIoutBuf,"%5.4f\n", batAH);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/******************** GET INSTRUMENT SETTINGS ***********************/
// return the mode
scpi_error_t get_set_mode(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  Serial.print("SCPI get mode\n");
  sprintf(SCPIoutBuf,"%s\n", modeNames[pSet.mode]);
  Serial.print(SCPIoutBuf);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

// return the voltage setpoint
scpi_error_t get_set_voltage(struct scpi_parser_context* context, struct scpi_token* command)
{ 
//Serial.print("SCPI get volt\n");
  sprintf(SCPIoutBuf,"%5.3f\n", pSet.voltage);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t get_set_current(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%5.3f\n", pSet.current);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t get_set_power(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%5.3f\n", pSet.power);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_set_resistance(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%5.3f\n", pSet.resistance);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_set_state(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%s\n", (_outOn)? "ON" : "OFF");
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t get_log_interval(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%5.3f\n", pSet.logTime/1000.0);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

// READ STEP SETTINGS  
scpi_error_t get_step_state(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%s\n", (isModeB[MODE_ST] == BMODE_RUN)? "ON" : "OFF");
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_voltage_A(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = pSet.s[0].volts;
  //Serial.printf("CCC %5.2f\n",valX);
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_voltage_B(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = pSet.s[1].volts;
  //Serial.printf("CCC %5.3f\n",valX);
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_current_A(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = pSet.s[0].amps;
  //Serial.printf("CCC %5.3f\n",valX);
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_current_B(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = pSet.s[1].amps;
  //Serial.printf("CCC %5.3f\n",valX);
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_time_A(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = pSet.s[0].time/1000.0;
  //Serial.printf("CCC %5.2f\n",valX);
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_time_B(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = pSet.s[1].time/1000.0;
  //Serial.printf("CCC %5.2f\n",valX);
  sprintf(SCPIoutBuf,"%5.3\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_steps_A(struct scpi_parser_context* context, struct scpi_token* command)
{
  int valX;
  valX = pSet.s[0].steps;
  //Serial.printf("CCC %5.2f\n",valX);
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_steps_B(struct scpi_parser_context* context, struct scpi_token* command)
{
  int valX;
  valX = pSet.s[1].steps;
  //Serial.printf("CCC %5.0f\n",valX);
  sprintf(SCPIoutBuf,"%i\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_slew(struct scpi_parser_context* context, struct scpi_token* command)
{
  int valX;
  valX = pSet.slew;
  //Serial.printf("CCC %5.2f\n",valX);
  sprintf(SCPIoutBuf,"%5.3f\n", valX/1000.0);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_cycles(struct scpi_parser_context* context, struct scpi_token* command)
{
  int valX;
  valX = pSet.cycles;
  sprintf(SCPIoutBuf,"%i\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

// BATT
scpi_error_t get_batt_state(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%s\n", (isModeB[MODE_BAT] == BMODE_RUN)? "ON" : "OFF");
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/***************************** CHANGE SETTINGS ***********************************************/
//scpi_parse_numeric(const char* str, size_t length, float default_value, float min_value, float max_value);
// Current
scpi_error_t setC(int step, struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, ov, temp;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  output_numeric = scpi_parse_numeric(args->value, args->length, 1, IDROPOUT, iMaxOp);
  //Serial.printf("SC: [%s] %5.3f, length %i\n", args->value, output_numeric.value, args->length);
  if((output_numeric.length == 0 ||
    (output_numeric.length == 1 && output_numeric.unit[0] == 'A')) &&
	(output_numeric.value >= IDROPOUT && output_numeric.value <= iMaxOp))
  {

Serial.printf("*");
    ov = constrain(output_numeric.value, 0, iMaxOp);
	if(step < 0)
		pSet.current = ov;
	else
		pSet.s[step].amps = ov;
    //iSetting = output_value;
	//Serial.print("S");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid set current %5.2f", output_numeric.value) ;
	error.description = myError;	
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);
	error.length = strlen(myError) +1;
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t set_current(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return setC(-1, context, command); // main
}
scpi_error_t set_current_A(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return setC(0, context, command);  // step 0
}
scpi_error_t set_current_B(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return setC(1, context, command); // step 1
}

/** Set output voltage  */
scpi_error_t setV(int step, struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, output_valueS;
  //float maxV = 25;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  output_numeric = scpi_parse_numeric(args->value, args->length, 12, VDROPOUT, VMAXOP);
  if((output_numeric.length == 0 ||
    (output_numeric.length == 1 && output_numeric.unit[0] == 'V')) &&
	(output_numeric.value >= VDROPOUT && output_numeric.value <= VMAXOP)
	)
  {
    output_value = constrain(output_numeric.value, 0, VMAXOP);
	if(step < 0)
		pSet.voltage = output_value;
	else
		pSet.s[step].volts = output_value;
	dirtyScreen = true;
	valChanged(VAL_CHGD);
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid set voltage  %5.2f", output_numeric.value) ;
	error.description = myError;
	Serial.printf("SetV ERR: |%s|  |%s| \n",myError,error.description);
    error.length = strlen(myError) +1;
    sprintf(SCPIoutBuf,"%i,\"%s\"\n", error.id, error.description);
	//Serial.printf("Errline: |%s|\n", SCPIoutBuf);
    SCPItrans(SCPIoutBuf);    
    scpi_queue_error(&ctx, error);
	//printSCPIerrQ(&ctx);
  }
  scpi_free_tokens(command);
 // Serial.printf("Debug - set volts to %6.3f V\n", vSetting);
  return SCPI_SUCCESS;
}
scpi_error_t set_voltage(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return setV(-1, context, command);
}
scpi_error_t set_voltage_A(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return setV(0, context, command);
}
scpi_error_t set_voltage_B(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return setV(1, context, command);
}

scpi_error_t set_power(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, ov, temp;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  output_numeric = scpi_parse_numeric(args->value, args->length, 1, PDROPOUT, PMAXOP);
  if((output_numeric.length == 0 ||
    (output_numeric.length == 1 && output_numeric.unit[0] == 'W')) &&
	(output_numeric.value >= PDROPOUT && output_numeric.value <= PMAXOP))
  {
//temp = (chDef[streamIn[currentStreamIn].channel].ap + IOUT)->maxVal; // maximum allowed current (off
//Serial.printf("SC: %5.2f\n", temp);
    ov = constrain(output_numeric.value, PDROPOUT, PMAXOP);
    pSet.power = ov;
    //iSetting = output_value;
	//Serial.print("S");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid set power %5.2f", output_numeric.value) ;
	error.description = myError;
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t set_resistance(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, ov, temp;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }
Serial.println(":RESI");
  output_numeric = scpi_parse_numeric(args->value, args->length, 1, RDROPOUT, RMAXOP);
printNumArgs("RESI", output_numeric);
  if((output_numeric.length == 0 ||
    (output_numeric.length == 1 && output_numeric.unit[0] == 'R')) &&
	(output_numeric.value >= RDROPOUT && output_numeric.value <= RMAXOP))
  {
//temp = (chDef[streamIn[currentStreamIn].channel].ap + IOUT)->maxVal; // maximum allowed current (off
//Serial.printf("SC: %5.2f\n", temp);
    ov = constrain(output_numeric.value, RDROPOUT, RMAXOP);
    pSet.resistance = ov;
    //iSetting = output_value;
	//Serial.print("S");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid set resistance %5.2f\n", output_numeric.value) ;
	error.description = myError;
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t set_log_interval(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, ov, temp;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }
//Serial.println(":LOGI");
  output_numeric = scpi_parse_numeric(args->value, args->length, 1, 0, MAXUINTVAL);
// printNumArgs("LOGI", output_numeric);
  if((output_numeric.length == 0 || 
	(output_numeric.length == 1 && output_numeric.unit[0] == 'S'))	  
	&& output_numeric.value >= 0 && output_numeric.value <= MAXUINTVAL/1000.0) 
  {
//temp = (chDef[streamIn[currentStreamIn].channel].ap + IOUT)->maxVal; // maximum allowed current (off
//Serial.printf("SC: %5.2f\n", temp);
    ov = constrain(output_numeric.value*1000, 0, MAXUINTVAL); // already done in checks, above
    pSet.logTime = ov;
    //iSetting = output_value;
	//Serial.print("S");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid log interval %5.2f", output_numeric.value) ;
	error.description = myError;
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
//Serial.println(SCPIoutBuf);
    SCPItrans(SCPIoutBuf);
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t set_steps(int step, struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, ov, temp;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  output_numeric = scpi_parse_numeric(args->value, args->length, 1, 0, MAXUINTVAL);
  if(output_numeric.length == 0 &&
	(output_numeric.value >= 0 && output_numeric.value <= MAXUINTVAL)) 
  {
//temp = (chDef[streamIn[currentStreamIn].channel].ap + IOUT)->maxVal; // maximum allowed current (off
//Serial.printf("SC: %5.2f\n", temp);
    ov = constrain(output_numeric.value, 0, MAXUINTVAL);
	pSet.s[step].steps = ov;
    //iSetting = output_value;
	//Serial.print("S");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid set steps %5.2f", output_numeric.value) ;
	error.description = myError;
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t set_steps_A(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return set_steps(0, context, command);
}
scpi_error_t set_steps_B(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return set_steps(1, context, command);
}

scpi_error_t set_Stime(int step, struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, ov, temp;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  output_numeric = scpi_parse_numeric(args->value, args->length, 1, 0, MAXUINTVAL);
  if((output_numeric.length == 0 ||
    (output_numeric.length == 1 && output_numeric.unit[0] == 'S')) &&
	(output_numeric.value >= 0 && output_numeric.value <= MAXUINTVAL/1000.0)) // output_numeric.unit[0] ~~ 'mS')
  {
//temp = (chDef[streamIn[currentStreamIn].channel].ap + IOUT)->maxVal; // maximum allowed current (off
//Serial.printf("SC: %5.2f\n", temp);
    ov = constrain(output_numeric.value * 1000, 0, MAXUINTVAL); // mS
	pSet.s[step].time = ov;
    //iSetting = output_value;
	//Serial.print("S");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid set step time %5.2f", output_numeric.value) ;
	error.description = myError;
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t set_time_A(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return set_Stime(0, context, command);
}
scpi_error_t set_time_B(struct scpi_parser_context* context, struct scpi_token* command)
{	
	return set_Stime(1, context, command);
}

scpi_error_t set_slew(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, ov, temp;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  output_numeric = scpi_parse_numeric(args->value, args->length, 1, 0, MAXUINTVAL);
  if((output_numeric.length == 0 ||
    (output_numeric.length == 1 && output_numeric.unit[0] == 'S')) &&
	(output_numeric.value >= 0 && output_numeric.value <= MAXUINTVAL/1000.0)) // output_numeric.unit[0] ~~ 'mS')
  {
//temp = (chDef[streamIn[currentStreamIn].channel].ap + IOUT)->maxVal; // maximum allowed current (off
//Serial.printf("SC: %5.2f\n", temp);
    ov = constrain(output_numeric.value * 1000, 0, MAXUINTVAL);
	pSet.slew = ov;
    //iSetting = output_value;
	//Serial.print("S");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid set rise/fall %5.2f", output_numeric.value) ;
	error.description = myError;
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t set_cycles(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, ov, temp;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  output_numeric = scpi_parse_numeric(args->value, args->length, 1, 0, MAXUINTVAL);
  if(output_numeric.length == 0  &&
	(output_numeric.value >= 0 && output_numeric.value <= MAXUINTVAL)) // output_numeric.unit[0] ~~ 'mS')
  {
//temp = (chDef[streamIn[currentStreamIn].channel].ap + IOUT)->maxVal; // maximum allowed current (off
//Serial.printf("SC: %5.2f\n", temp);
    ov = constrain(output_numeric.value, 0, MAXUINTVAL);
	pSet.cycles = ov;
    //iSetting = output_value;
	//Serial.print("S");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid set step cycles %5.2f", output_numeric.value) ;
	error.description = myError;	
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/*** Set the output on or off */
scpi_error_t set_state(struct scpi_parser_context* context, struct scpi_token* command)
{

  struct scpi_token* args;
  //struct scpi_numeric output_numeric;
  //unsigned char output_value;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  //Serial.printf("SET IPROT: [%s]\n", args->value);
  if (!strncmp(args->value,"ON",2))
  {
     onOff(0, true);  
	 //Serial.print("S");
	 valChanged(VAL_CHGD);
	 dirtyScreen = true;
  }
  else 
    if(!strncmp(args->value,"OFF", 3))
	{		
       onOff(0, false);  ;  
	  //Serial.print("S");
	  valChanged(VAL_CHGD); 
	  dirtyScreen = true;
	}
    else
    {
      scpi_error error;
      error.id = -200;
	  strncpy(valBuf, args->value, args->length);
	  valBuf[args->length] = '\0';
      sprintf(myError, "Command error; Invalid set state (on/off) '%s'", valBuf);
	  error.description = myError;
      sprintf(SCPIoutBuf,"%i,\"%s\"", error.id,error.description);
      SCPItrans(SCPIoutBuf);
      scpi_queue_error(&ctx, error);
    }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
/*** Set the mode */
scpi_error_t set_mode(struct scpi_parser_context* context, struct scpi_token* command)
{
  int i;
  struct scpi_token* args;
  //struct scpi_numeric output_numeric;
  //unsigned char output_value;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  //Serial.printf("SET MODE: [%s]\n", args->value);
  for(i = 1; i < C_MODES; i++)
	  if (!strncmp(args->value,modeNames[i],2))
	  {
		 setMode(i);  
		 //Serial.print("S");
		 valChanged(VAL_CHGD);
		 dirtyScreen = true;
		 break;
	  } 
  if(i == C_MODES)
  {
	  scpi_error error;
	  error.id = -200;
	  strncpy(valBuf, args->value, args->length);
	  valBuf[args->length] = '\0';
	  sprintf(myError, "Command error; Invalid set mode '%s'", valBuf);
	  error.description = myError;
	  sprintf(SCPIoutBuf,"%i,\"%s\"", error.id,error.description);
	  SCPItrans(SCPIoutBuf);
	  scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/*** Set STEP function on (start) or off (stop) */
scpi_error_t set_step_state(struct scpi_parser_context* context, struct scpi_token* command)
{

  struct scpi_token* args;
  //struct scpi_numeric output_numeric;
  //unsigned char output_value;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  if (!strncmp(args->value,"ON",2))
  {
     startST(0);  
	//Serial.print("S: VE Y");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else 
    if(!strncmp(args->value,"OFF", 3))
	{		
      setNORb(0);
	//Serial.print("S: VE N");
	valChanged(VAL_CHGD);
	dirtyScreen = true;
	}
    else
    {
      scpi_error error;
      error.id = -200;
	  strncpy(valBuf, args->value, args->length);
	  valBuf[args->length] = '\0';
      sprintf(myError, "Command error; Invalid set STEP mode on call '%s'", valBuf) ;
	  error.description = myError;
      sprintf(SCPIoutBuf,"%i,\"%s\"", error.id,error.description);
      SCPItrans(SCPIoutBuf);
      scpi_queue_error(&ctx, error);
    }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/*** Set Battery test on (start) or off (stop)*/
scpi_error_t set_batt_state(struct scpi_parser_context* context, struct scpi_token* command)
{

  struct scpi_token* args;
  //struct scpi_numeric output_numeric;
  //unsigned char output_value;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  if (!strncmp(args->value,"ON",2))
  {
     startBAT(0);
	//Serial.print("S:CE Y");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else 
    if(!strncmp(args->value,"OFF", 3))
	{		
      setNORb(0);
	//Serial.print("S:CE N");
	valChanged(VAL_CHGD);
	dirtyScreen = true;
	}
    else
    {
      scpi_error error;
      error.id = -200;
	  strncpy(valBuf, args->value, args->length);
	  valBuf[args->length] = '\0';
      sprintf(myError, "Command error; Invalid set BAT mode on '%s'", valBuf) ;
	  error.description = myError;
      sprintf(SCPIoutBuf,"%i,\"%s\"", error.id,error.description);
      SCPItrans(SCPIoutBuf);
      scpi_queue_error(&ctx, error);
    }
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/*** COMMUNCIATION  (INST) Set the SSID*/
scpi_error_t set_ssid(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, output_valueS;
  //float maxV = 25;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  if(strlen(args->value) >= 2  && strlen(args->value) <= 32)
  {
		//Serial.printf("Change SSID to [%s]\n", args->value);
		strcpy(myID.local_ssid,args->value);
		valChanged(VAL_CHGD);
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid set SSID '%s'", args->value) ;
	error.description = myError;
    //error.length = 26;
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);    
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
 // Serial.printf("Debug - set volts to %6.3f V\n", vSetting);
  return SCPI_SUCCESS;
}
/*** Set the WiFi PASSphrase*/
scpi_error_t set_pass(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  float output_value, output_valueS;
  //float maxV = 25;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }
  if(strlen(args->value) >= 8  && strlen(args->value) <= 63)
  {
		//Serial.printf("Change PASS to [%s]\n", args->value);
		strcpy(myID.local_pass,args->value);
		valChanged(VAL_CHGD);
		dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid SSID password '%s'", args->value) ;
	error.description = myError;
    //error.length = 26;
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);    
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
 // Serial.printf("Debug - set volts to %6.3f V\n", vSetting);
  return SCPI_SUCCESS;
}

/*** Set the WiFi autoConnect function on or off */
scpi_error_t set_connect(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  //struct scpi_numeric output_numeric;
  unsigned char output_value;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }
  //Serial.printf("SET CONN: [%s]\n", args->value);
  if (!strncmp(args->value,"ON",2))
  {
	myID.autoConnect = true;    
	//Serial.print("AC+");
	valChanged(VAL_CHGD);
	dirtyScreen = true;
  }
   else 
    if(!strncmp(args->value,"OFF",3))
	{		
	  myID.autoConnect = false;    
	 // Serial.print("AC-");
	  valChanged(VAL_CHGD) ;
	  dirtyScreen = true;
	} 
    else
    {
      scpi_error error;
      error.id = -200;
	  strncpy(valBuf, args->value, args->length);
	  valBuf[args->length] = '\0';
      sprintf(myError, "Command error; Invalid auto-connect command '%s'", valBuf) ;
	  error.description = myError;
      sprintf(SCPIoutBuf,"%i,\"%s\"", error.id,error.description);
      SCPItrans(SCPIoutBuf);  
      scpi_queue_error(&ctx, error);
    }

  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t set_hostName(struct scpi_parser_context* context, struct scpi_token* command){
  struct scpi_token* args;
  struct scpi_numeric output_numeric;
  //float output_value, output_valueS;
  //float maxV = 25;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  if(strlen(args->value) >= 2  && strlen(args->value) <= 16)
  {
		//Serial.printf("Change HOST NAME to [%s]\n", args->value);
		strcpy(myID.instName, args->value);
		valChanged(VAL_CHGD);
		dirtyScreen = true;
  }
  else
  {
    scpi_error error;
    error.id = -200;
	sprintf(myError, "Command error; Invalid hostname '%s'", args->value);
	error.description = myError;
    //error.length = 26;
    sprintf(SCPIoutBuf,"%i,\"%s\"", error.id, error.description);
    SCPItrans(SCPIoutBuf);    
    scpi_queue_error(&ctx, error);
  }
  scpi_free_tokens(command);
 // Serial.printf("Debug - set volts to %6.3f V\n", vSetting);
  return SCPI_SUCCESS;
}

// return the WiFi pass phrase
scpi_error_t get_pass(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%s\n", "WiFi password can only be SET remotely."); //myID.local_pass
//Serial.printf("SCPI PASS: %s\n", myID.local_pass);
  //sprintf(SCPIoutBuf,"%s\n", "SCPI PASS: RUBBISH");
// Serial.printf(""%s\n",SCPI PASS: RUBBISH\n");

  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
// return the SSID
scpi_error_t get_ssid(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%s\n",myID.local_ssid);
  //Serial.printf("SCPI SSID: %s\n", myID.local_ssid);
  //sprintf(SCPIoutBuf,"%s\n", "SCPI SSID: RUBBISH");
//Serial.printf("%s\n","SCPI SSID: RUBBISH\n");
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t get_hostname(struct scpi_parser_context* context, struct scpi_token* command){
  sprintf(SCPIoutBuf,"%s\n",myID.instName);
  //Serial.printf("SCPI HOST NAME: [%s]\n", myID.instName);
  //sprintf(SCPIoutBuf,"%s\n", "SCPI SSID: RUBBISH");
//Serial.printf("%s\n","SCPI SSID: RUBBISH\n");
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

// return the WiFi auto connect setting
scpi_error_t get_connect(struct scpi_parser_context* context, struct scpi_token* command)
{ 
  sprintf(SCPIoutBuf,"%s\n", (myID.autoConnect)?"ON":"OFF");
//Serial.printf("SCPI AUTO CONN: %s\n", (myID.autoConnect)?"ON":"OFF");
 // sprintf(SCPIoutBuf,"%s\n", "AUTO CONN: RUBBISH");
//Serial.printf("SCPI AUTO: %s\n", "RUBBISH");
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
#ifndef PROD
scpi_error_t get_ADSv(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = meas.ADSvolts;
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_ADSvC(struct scpi_parser_context* context, struct scpi_token* command)
{
  int valX;
  valX = ADCcount.ADSv;
  sprintf(SCPIoutBuf,"%i\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_ESPv(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = meas.ESPvolts;
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_ESPvC(struct scpi_parser_context* context, struct scpi_token* command)
{
  int valX;
  valX = ADCcount.ESPv;
  sprintf(SCPIoutBuf,"%i\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_ADSa(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = meas.ADSamps;
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_ADSaC(struct scpi_parser_context* context, struct scpi_token* command)
{
  int valX;
  valX = ADCcount.ADSa;
  sprintf(SCPIoutBuf,"%i\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_ESPa(struct scpi_parser_context* context, struct scpi_token* command)
{
  float valX;
  valX = meas.ESPamps;
  sprintf(SCPIoutBuf,"%5.3f\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_ESPaC(struct scpi_parser_context* context, struct scpi_token* command)
{
  int valX;
  valX = ADCcount.ESPa;
  sprintf(SCPIoutBuf,"%i\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
scpi_error_t get_DAC(struct scpi_parser_context* context, struct scpi_token* command)
{
  int valX;
  valX = DACis;
  sprintf(SCPIoutBuf,"%i\n", valX);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
#endif

scpi_error_t get_post(struct scpi_parser_context* context, struct scpi_token* command)
{
  sprintf(SCPIoutBuf,"%s\n", postBuf);
  SCPItrans(SCPIoutBuf);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}


// broadcast TRACKING messages to the group if tracking
// - Tracking reduction value
// - voltage set
// - Current set
// incoming messages handled via SCPI handler

//bool oneSent = false;

//#define SEND_NOT_TRACK	1000 // send heartbeat occasionally if not Tracking

/* //** Set the Tracking on or off 
scpi_error_t set_track_enable(struct scpi_parser_context* context, struct scpi_token* command)
{

  struct scpi_token* args;
  //struct scpi_numeric output_numeric;
  //unsigned char output_value;

  args = command;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  if (!strncmp(args->value,"ON",2))
  {
     chDef[streamIn[currentStreamIn].channel].xp->trackOn = true;  
	//Serial.print("S:TE Y");
	valChanged(VAL_CHGD); 
	dirtyScreen = true;
  }
  else 
    if(!strncmp(args->value,"OFF", 3))
	{		
      chDef[streamIn[currentStreamIn].channel].xp->trackOn = false;  
	//Serial.print("S:TE N");
	valChanged(VAL_CHGD);
	dirtyScreen = true;
	}
    else
    {
      scpi_error error;
      error.id = -200;
	  strncpy(valBuf, args->value, args->length);
	  valBuf[args->length] = '\0';
      sprintf(myError, "Command error; Invalid value '%s'", valBuf) ;
	  error.description = myError;
      sprintf(SCPIoutBuf,"%i,\"%s\"", error.id,error.description);
      SCPItrans(SCPIoutBuf);
      scpi_queue_error(&ctx, error);
      scpi_free_tokens(command);
      return SCPI_SUCCESS;
    }

  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
*/
#endif