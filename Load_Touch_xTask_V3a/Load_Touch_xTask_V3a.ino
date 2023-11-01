// V1.3
//#define PROD
#define SOFT_VERSION 4
#define KELVIN_MOD // only redefines a few default calibration settings
/* REMEMBER TO SET DISPLAY TYPE BEFORE COMPILING
 * controlTask() is driven by ADS1115 interrupts, watchdog in main code for lost interrupt 
*/
#define  ILI9488 // select one
//#define  ILI9341 //
#define SAVEBATLOG  // after battery test use the battery log when saving log file via http 
//#define BOARD_D 

// diagnostic options
#define WIFI    // faster startup for testing if WiFi is off. Also disbles serial SCPI?
#define REALNET // use network credentials from a file for WiFi testing
//#define NO_ERRORS // don't show screen Error messages - for testing basic TFT functions
//#define C_DEBUG // slow down control execution and enable serial diagnostics, for testing
#define PROCESS_EVERY_CDEBUG 20 //  
//#define PRINT_DIAGNOSTICS
//#define SOATEST // reduce I, V and P max values for testing SOA code. Recompile with different software version when changing this.
bool _printMe = true;   // diagnostic prints during testing 
// end of diagnostic settings

#ifdef PROD
  #define SPLASH    // turn on for production
  #undef C_DEBUG    // ensure all diagnostics are off
  #undef PRINT_DIAGNOSTICS
  #undef SOATEST 
  #undef REALNET
  _printMe = false;
#endif

// ADS watchdog
#define ADS_WATCHDOG 10    // (mS) restart ADS cycle interrupts get lost (ADS interrupt cycle ~ 1.2mS @ 860sps)
volatile long adsLastInt;
volatile long controlTime = 0;
long loops = 0;
int adsCount = 0;
bool startUp = true;
bool _scrTouched = false;  // was the screen touched?
bool newScrError = false; // for non-blocking version of screenError
bool holdScreen = false;  // don't update screen while this is true (screenError)

#include <esp_task_wdt.h>
#include "myLInst.h"
#include "myLoad.h"
#include "myLESP32.h"
#include "myLHelpers.h"
#include "myLScreenDefs.h"
#include "myLSwitches.h"
#include "myLADS1115.h"
#include "myLADS1115bits.h"   
#include "myLPOST.h"

void printfloat(char fmt[], float val, uint16_t highlight, uint16_t x, short y, uint16_t txtcol, uint16_t highcol, uint16_t bgcol);
int readADC(int ADCpin, bool printMe = false);
//long loopcntr;
//#define TESTPIN FANPIN
bool myBlink = false; // set by timer at same time as regular redraw
bool testPin = false;

#include "myLScreenDraw.h"
#include "myLWifi.h"
#include "myLTelnet.h"
#include "myLUDP.h"
//#include "myLHTTP.h"
#include "myLHTML_literal.h"
#include "myLHTML_processor.h"
#include "myLwebserverSPIFFS.h"

#include "myLADC_DAC_PWM.h"

#include "myLControl.h"
#include "lscpiparser.h"
#include "myLSCPI.h"
#include "myLProfile.h"
#include "myLSPIFFSProfile.h"
#include "myLlog.h"
//float vTarget = VSTART;
//float iTarget = 0.08;
//float eTrack = 1.0;
//int setting = WSTART;

long lTimer, mTimer, fTimer, vfTimer, vTimer, cTimer, tTimer, tUsec, cdTimer, lcount = 0; // various processing frequency timers for loop()
#define NBUCKETS 100
//int timeBucket[NBUCKETS];
//long fineTime;

// xtask params
TaskHandle_t controlTaskHandle;
xTimerHandle controlCycleTimer;
#define TIMER_TICK  1     // mS per tick. 
#define TIMER_CYCLE 1     // Control cycle time. Something less than the ADS measurement cycle to allow intermediate ESP readings.

#define C_STACK    12000  // Stack size for control task
#define TASK_CPU  1     // beware of changing this from the same core as Arduino - may cause watchdog errors 
#define TASK_PRI  4   // higher number than loop() == 1

long controlStart;

void setup() {   
  newPosition = 0; 
  Serial.begin(115200);
  delay(1000);
  Serial.println("*********** Starting DC Load Control *************");
  /******** TFT & TOUCH *****************/
  screenBegin();
#ifdef SPLASH
  splashScreen();
#endif
  if (!post())
     (postBuf, MY_RED, 10, false); // 3 x I2C for single (7 for dual);
 /*   EEPROM.begin(EESIZE + 5 ); // maybe a few additional bytes will stop the kernel panic restart on save
  if (!setupEE()) // read values from EEPROM
  {
      screenError("Bad EEPROM read\nFailed factory reset\nAborting", MY_RED, 10, false);  
      // should probably shut things down
  }
  */
  for(int i = 0; i < NUM_CALS; i++)
    hal_defs[i] = halCal[i];  // copy factory defaults in case a reset is needed
  if(!SPIFFSstart())
    screenError("File system failure", MY_RED, -1, true); 
//Serial.println("SPIFFS started"); listDir();

//Serial.println("After getComms"); listDir();
 // settings from EEPROM
  if(!beginEE())
  {
     Serial.println("EEPROM unreadbale after factory reset - should possibly halt!");
     screenError("Unable to read EEPROM after\nFactory reset", MY_RED, -1, true);
  }
  ts.setRotation(sc.tsRot); // some displays have a rotated touch panel (CAL menu)
  tft.setRotation(sc.scrRot); // some displays have a rotated screen (CAL menu)
  
  // set up control pins, ISR and other hardware-related initialisation
  control_setup(); // daughter board sensing
  
  // comms params from SPIFFS profile.json - must run after Settings read from SPIFFS (overwrite comms in myID)
  if(getComms()) 
  {
    updateWiFi(myID.local_ssid, myID.local_pass, true);
    //printStoredWiFi();
  } 
  else
  {
    screenError("Unable to read profile in file system\nFile restore needed", MY_RED, -1, true);
  }

  // xTask for control()
  // 
  //Serial.println("xTask");  delay(1000);
  //pinMode(TESTPIN, OUTPUT);
  /*
  for(int i = 0; i < 5000; i++)
  {   
     testPin = !testPin;
     digitalWrite(TESTPIN, (testPin)? HIGH : LOW);
     delay(1);    
  }
  */
    // start the regular conversion process
  attachInterrupt(digitalPinToInterrupt(ADSRDY), ads_ISR, FALLING);
 // Serial.println("ATTACH int done");
  //  delay(100);  
   controlStart = micros();
   adsReady = false;
   adsReadStart(); // start the ADS (restarted after interrupt in controlTask, watchdog in loop())

  // Once controlTask() is running, no I2C commands should be issued in any other routines to avoid conflicts
  xTaskCreatePinnedToCore(controlTask, "cTask", C_STACK, NULL, (TASK_PRI | portPRIVILEGE_BIT ), &controlTaskHandle, TASK_CPU);
  //controlCycleTimer = xTimerCreate("timerC", pdMS_TO_TICKS(TIMER_TICK), true, (void*)0, controlCallback); // auto reload timer
  //xTimerStart(controlCycleTimer, TIMER_CYCLE);
  // Serial.println("xTask2");delay(1000);

  //ads.adsStartSingleDiffRdy(adsConfig, ADSMUX_START);
  //adsLastInt = millis();
  //Serial.println("xTask3");delay(1000);
  delay(100); // let control stabilise
  currentZeroCal(); // calibrate zero current
#ifdef WIFI
  // remote command handling - Telnet, UDP, SCPI
  //Serial.println("Starting WiFi");
  if(myID.autoConnect)
  {
    wifiBegin();
    setupTelnet();
    //setup_http();     
    webServerStart();  
    UDPstart();// Serial.printf("UDP start %s\n",(UDPstart())?"OK":"BAD");
  }
  SCPI_setup();
#endif
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(PWM_PIN, PWMChannel);  
  // configure  PWM functionalitites
  ledcSetup(PWMChannel, PWM_FREQ, PWM_RESOLUTION);
  setFan(70);
  redrawScreen();
  //printDaughterCal();
  //printHalCal();
 getESPviReadings();
 esp_task_wdt_init(10, true); // change the watchdog to 10 seconds
 esp_task_wdt_add(NULL);
 //Serial.printf("Task subscribed to WDT %s\n", (esp_task_wdt_status(NULL) == ESP_OK)? "OK" : "BAD");
 //Serial.printf("fmap %3.2f\n", fmap(5, 0.0, 10.0, 0, 20));
  Serial.println("******** Done setup *******");
  kelvinTest();
  Serial.println("SCPI Command?");
  delay(50);  
  lTimer =  mTimer = fTimer = vfTimer = vTimer = cTimer = tTimer = millis();
  enableLoopWDT();  // should already be done automatically
}
int count = 0;
#include "myLTouchProcess.h"
/*
static void controlCallback(xTimerHandle pxTimer)
{
    //Serial.print("|");
    vTaskResume(controlTaskHandle);
}
*/
// ADS finished conversion 
// - resumes controlTask()
// - watchdog in loop()
void IRAM_ATTR ads_ISR(){  
  BaseType_t xYield;
  //portENTER_CRITICAL_ISR(&mux);
  adsReady = true;
  adsLastInt = millis();
  adsISRcount++; 
  //portEXIT_CRITICAL_ISR(&mux);
  xYield = xTaskResumeFromISR(controlTaskHandle);
  portYIELD_FROM_ISR(xYield);
}

uint16_t potval = 0;
bool updateZero;
short UDPcnt;
long lastADS = 0;
bool dp;


// all I2C processing called from here to avoid conflicts.


long ADStLast;
long screenHoldStart = 0;
void loop() /**************** LOOP ***************************/
{ 
  if(!(esp_task_wdt_reset() == ESP_OK))
    Serial.println("WDT not fed.");;
 //  fineTime = micros();
   /* very fast stuff */    
#ifndef ALTENCODER 
   enc.loop();
#endif
   // tUsec = micros();
#ifdef C_DEBUG    // slow down control for debug
 if(millis() - cdTimer > PROCESS_EVERY_CDEBUG)
 { 
   cdTimer = millis();
#endif
  // control was here
#ifdef C_DEBUG // slow down control for debug (see myControl.h)
 }
#endif   
 if(millis() > vfTimer + PROCESS_EVERY_VF)
    {   
      // just rise and fall (slew) setting 
      vfTimer = millis(); 
      // update dynamic settings, let control() take care of output management
      if(pSet.modeB == MODE_ST) // mode resets to NOR once cycle(s) are completed???
         dynSet = stepCalc();
      else
        if(pSet.modeB != MODE_BAT)
           dynSet = pSet; // values may have changed - BAT test takes care of this explicitly
    }    
     feedLoopWDT(); yield(); // let ESP32 background tasks run
     
    if(millis() > fTimer + PROCESS_EVERY_F)
    {   
      fTimer = millis();  
      if(pSet.modeB == MODE_BAT) // mode resets to NOR once cycle is completed
      {         
         batTest();
      }
     // Serial.print("F"); delay(1);
      // but.loop(); // encoder switch not used
      butL.loop(); // L & R switches
      butR.loop();
      processOnOffSw();
    }
    feedLoopWDT();
    yield(); 

    long adsL = adsLastInt;   
   
    if(millis() - cTimer > PROCESS_EVERY_CTRL)
    { 
      cTimer = millis();
      
     // detect lost adsReady interrupt (should occur every 1.2mS)   
      if((millis() - adsLastInt) > ADS_WATCHDOG) // && !holdScreen???
      {
#ifndef NO_ERRORS
         Serial.printf("*** Lost ADS interrupt, restart after ISR %i mS, last restart %imS {ISR %i). ", millis() - adsLastInt, millis() - ADStLast, adsISRcount );
#endif
         // kick the ADS conversion process
         portENTER_CRITICAL_ISR(&mux); // avoid interrupts during processing
           adsReady = false;      
           vTaskResume(controlTaskHandle);     
         portEXIT_CRITICAL_ISR(&mux);
         ADStLast = millis();
         Serial.println("Restarted");
      }
#ifdef WIFI
      process_SCPI(); // do this faster than we send out messages - or deadly embrace can occur.
#endif
    // logging
    if(_outOn && (pSet.modeB != MODE_BAT)) // Bat test has its own logging cycles
    {
      addLog(false); // log after mode and timing analysis; don't force log entry 
    }
   }
   feedLoopWDT();
   yield(); 
       
   if(millis() > mTimer + PROCESS_EVERY_M)
   {  
    //Serial.print("-");
   // if(_outOn && meas.ADSvoltsAvg < 19.3)printADSavg();
    mTimer = millis();    
    //if (meas.ADSamps < RESPMIN_I || ((dynSet.mode != MODE_CC) &&  meas.ESPvolts < RESPMIN_V) ) Serial.print(" ESPLO ");

    lastADS = adSProcessCount;   
    
     // control inputs: encoder, buttons and touch    
     _scrTouched = processTouchSwEnc(); 

     if (dirtyScreen && !holdScreen)
       redrawScreen();

     if(changedVal_remote && !holdScreen)  // update all settings, as something has changed remotely
     {
     // Serial.print('#');
       drawSettings(-1, 0, true); // redraw all settings, nothing selected
       changedVal_remote = false; 
     }
     if(!_outOn)
     {
       rReading = 0.0;
       pReading = 0.0;
     }
     else
     {  
       rReading = abs((iReading < SMALL_DIFF) ? 0.0 : vReading / iReading);
       pReading = abs(vReading * iReading);
     }
     /*
     if(pSet.logTime > 0 && pSet.logTime < MINLOG)
     {
       pSet.logTime = MINLOG; 
       valChanged(VAL_CHGD);
     }     
     */
     if(!holdScreen)
        drawReadings(); // only one per cycle
     count++;
  }
  feedLoopWDT();
  yield(); 
  
  
  // lower priority  - screen redraw
  if(millis() > lTimer + PROCESS_EVERY_L)
  {
    lTimer = millis();  
    
    //Serial.printf("ADSV %2.3f\n", meas.ADSvolts);
    
    if(highButton >= 0 && highButton < NUMBUTS) // ensure highlighted set digit is always visible
    {
      //int sv =  but[highButton].sv;  
      butDigit = constrainButDig(butDigit, highButton); 
      //Serial.printf("BD[%i] = %i [%i..%i]\n", highButton, butDigit, (1 - setVals[sv].post),(setVals[sv].post -1) );
    } 
    
    // non-blocking screen error  
    if(newScrError || holdScreen) 
    {
      if(newScrError) 
      {
        screenHoldStart = millis();
        holdScreen = true;
        screenError(scrE.message, scrE.bgcol, scrE.onTime, scrE.logo);   // , blocks initially for 0 seconds
        newScrError = false;
        //Serial.printf("Holding screen %iS\n", scrE.onTime);
      } 
      if(scrE.onTime >= 0)
      {
        if((millis() -  screenHoldStart)/1000 > scrE.onTime)
          holdScreen = false;
          dirtyScreen = true;
      }
      else
      {
        if(_scrTouched)
          holdScreen = false;
          dirtyScreen = true;
      }
    }
    meas.ESPtemp = readESP_T(); // update temp 
    if( meas.ESPtemp > TMAXOP) // slow response
    {
      onOffKill(0, false);
      errorKill(true, true, meas.ESPtemp, "Maximum temperature exceeded", true);      
    }
    //setFan(60);
    fanControl();
   //Serial.printf("Fan %i pwm [%2.1f C]\n", fanControl(), meas.ESPtemp);

#ifdef xxxC_DEBUG // slow down control for debug
      _DACis = control(ADS_CONTROL);
#endif
     startUp = false; // startUp concluded first time this is called
     if(!holdScreen)
     {
       drawIndicators(currentMenu);
       drawLegends(currentMenu);
     }
     myBlink = !myBlink;  
     if(!_outOn && updateZero) // re-calibrate zero current offset
     {   
        currentZeroCal(); 
        updateZero = false;
     }
  } 
  feedLoopWDT();
  yield(); 

  
  if(millis() > vTimer + PROCESS_EVERY_VL)
  {
    // Serial.printf("\nLoop %2.2f mS; ADS %2.2f mS\n", (float)(millis() - vTimer)/lcount, (float)(millis() - vTimer)/adsCount);
    vTimer = millis();          
    adsCount = lcount = 0;
#ifdef WIFI
     //UDPcnt = SCPIgroupHeartbeat();
#endif
    saveSettings();
     //saveEE();  // set flag if Profile value changed. Only saves after SAVE_EE_AFTER loops
     updateZero = true; // Current zero offset calibration at next "L" cycle
     //Serial.printf("Active SCPI streams %i, LLR %5.2f\n",UDPcnt, localLimitRatio);
     //Serial.printf("Limit:  %i [%i], Track: %i [%i], Output: %i\n", pContA.limitInd, pSet.limitOn, pContA.trackInd, pSet.trackOn, pSet._outOn);
    //Serial.printf("Setpoint Nom: %5.2F / %5.2F, act: %5.2F\n", pSet.voltage, pContA.vSetpointR, pContA.vSetpointX);
     //Serial.printf("Amps %3.3f, DAC %i, ampsToDAC %i\n", meas.ADSamps, _lastDAC, ampsToDAC(meas.ADSamps));
     //Serial.printf("saveEE ind %i\n",needToSaveEE);
    
    //printDynSet();
   // printADC(true);
      //Serial.printf("Free heap %i\n",ESP.getFreeHeap());
  // printHalCal();
  //    printDaughterCal();
    //printSTEP();
    //Serial.printf("Heartbeat: Output %s\n", (_outOn) ? "On" : "Off");
    //Serial.printf("Output %s\n", (_outOn) ? "On" : "Off");
   // Serial.printf("pMaxOp %2.2f, dynSet.current %2.2f, meas.ADSvolts %3.2f, calc pwr %3.2f, calc cur %2.2f\n", pMaxOp, dynSet.current, meas.ADSvolts, dynSet.current * meas.ADSvolts, pMaxOp / meas.ADSvolts);
  // Serial.printf("Volts: ADS count = %i, %2.3fV\n", ADCcount.ADSv, meas.ADSvolts);
   //Serial.printf("DAC: %i\n", _lastDAC);
  // Serial.printf("ESPamps %2.2f [%i], ADSamps %2.2f, ", meas.ESPamps, ADCcount.ESPa, meas.ADSamps );
  // Serial.printf("Volts: ADS %2.3fV[%2.3fV](%i)\n",  meas.ADSvolts, meas.ADSvoltsAvg, ADCcount.ADSv);
  // printICal();
  // printVCal();
   //  Serial.printf("pSet.logTime %i\n", pSet.logTime);
  // Serial.printf("dynSet.current %2.3f\n", dynSet.current);
   // Serial.printf("Fan %i - done\n",readFan());
  //  Serial.printf("Control time = %i\n", controlTime/loops); loops = 0; controlTime = 0;
 //   float ctRatio = (1000 * controlTime) / (micros() - controlStart);
  //  Serial.printf("control loop time %5.1f%%\n", ctRatio/10);
  //Serial.printf("BTS %i \n",batTestState);
  //printADSavg();
  }
  lcount++;
  feedLoopWDT();
  yield(); 
  
   /*
  loopTimetest(); // fineTime = micros(); needs to be uncommented at the top of loop()
 */
}

// keep the digit being changed for the selected setting within bounds.
int constrainButDig(int butDig, int hBut)
{
  int svx = but[hBut].sv; 
  int post = setVals[svx].post;
  int pre = setVals[svx].pre;
  // assume  post and pre are not *both* zero
  if(post == 0) // integer
    butDig = constrain(butDig, 0, pre - 1);
  else
    butDig = constrain(butDig, -post, pre - 1);
  if(pre == 0) // no XXX. part
    butDig = constrain(butDig, -post, -1);
  return butDig;
}
/*
void zeroBuckets(void){
  for (int  i = 0; i < NBUCKETS; i++)
    timeBucket[i] = 0;
}
void printBuckets(void){
  for (int  i = 0; i < NBUCKETS; i++)
  {
    Serial.printf(", %i",timeBucket[i] );
    if(i % 10 == 0)
    Serial.printf("\n %2i: ", i);
  }
}
*/
/*
void A2Dtest(void){
  int vall;
  for (float  i = 0; i < RDACHI_I; )
  {
    vall = ampsToDAC(i);
    //Serial.printf("DAC: %2.3f %i\n", i, vall);
    i = i * 1.2 + 0.01; 
  }
}
*/
// test routine
/*
void cvtADSReadings(void)
{
  // convert ADS counts to volts/amps
  //Serial.println("get ADS read");
  meas.ADSvolts = ADStoV(ADCcount.ADSv);
  meas.ADSamps  = ADStoA(ADCcount.ADSa);
  pReading = meas.ADSvolts * meas.ADSamps;
  if(meas.ADSamps > MEDIUM_DIFF)
     rReading = meas.ADSvolts / meas.ADSamps;
}
*/
void printDynSet(void)
{
  Serial.printf("dynSet: %3.2fV  %3.2fA, ", dynSet.voltage, dynSet.current);
  Serial.printf("mode = %i, modeB = %i\n", dynSet.mode, dynSet.modeB);
}
void printADC(bool isLong)
{ 
  getESPviReadings();// OK to update these
  // don't read ADS directly, interrupt driven conversion
  if(isLong)
  {
   
    Serial.printf("ADC: %i Mmeas [ESP]: ", adSProcessCount);
    Serial.printf("A: %2.3f %i [%2.3f %i]",   meas.ADSamps, ADCcount.ADSa, meas.ESPamps, ADCcount.ESPa); 
    Serial.printf(", V: %2.3f %i [%2.3f %i]", meas.ADSvolts, ADCcount.ADSv, meas.ESPvolts, ADCcount.ESPv);      
    Serial.printf(", T: [%2.3f %i]", readESP_T(), ADCcount.ESPt);     
    Serial.printf(", DAC: %i, OutOn %i\n", _DACis, _outOn);  
  } else
  {
    Serial.printf("ADC: %i Meas [ESP]: ", adSProcessCount);
    Serial.printf("A: %2.3f [%2.3f]",   meas.ADSamps, meas.ESPamps); 
    Serial.printf(", V: %2.3f [%2.3f]", meas.ADSvolts, meas.ESPvolts);      
    Serial.printf(", T: [%2.3f]\n", readESP_T());  
  }
}
void printDaughterCal(void)
{
  Serial.printf("dPresent %i [%i], iMaxOp %2.1f\n", myID.daughter_present, (digitalRead(DPRESENT) == LOW), iMaxOp );
  Serial.printf("halCal[ADS_I].maxVal %2.1f, maxErr %2.1f\n", halCal[ADS_I].maxVal, halCal[ADS_I].maxErr);
  Serial.printf("halCal[ESP_I].maxVal %2.1f, maxErr %2.1f\n", halCal[ESP_I].maxVal, halCal[ESP_I].maxErr);
    Serial.printf("halCal[ADS_V].maxVal %2.1f, maxErr %2.1f\n", halCal[ADS_V].maxVal, halCal[ADS_V].maxErr);
   Serial.printf("halCal[ESP_V].maxVal %2.1f, maxErr %2.1f\n", halCal[ESP_V].maxVal, halCal[ESP_V].maxErr);
  Serial.printf("VMAXOP %3.2f, IMAXOP %3.2f[%3.2f], PMAXOP %3.2f{%3.2f]\n",VMAXOP, IMAXOP, iMaxOp, PMAXOP, pMaxOp);
}
void printVCal(void)
{
  Serial.printf("halCal[ADS_V] %2.1f .. %2.1f [%i  .. %i], maxErr %2.1f\n", halCal[ADS_V].lowCalVal, halCal[ADS_V].hiCalVal, halCal[ADS_V].lowCalCount, halCal[ADS_V].hiCalCount, halCal[ADS_V].maxErr);
  Serial.printf("halCal[ESP_V] %2.1f .. %2.1f [%i  .. %i], maxErr %2.1f\n", halCal[ESP_V].lowCalVal, halCal[ESP_V].hiCalVal, halCal[ESP_V].lowCalCount, halCal[ESP_V].hiCalCount, halCal[ESP_V].maxErr);
}
void printICal(void)
{
  Serial.printf("halCal[ADS_I] %2.2f %2.2f %2.2f [%i  %i %i], maxErr %2.1f\n", halCal[ADS_I].lowCalVal, halCal[ADS_I].midCalVal, halCal[ADS_I].hiCalVal, halCal[ADS_I].lowCalCount, halCal[ADS_I].midCalCount, halCal[ADS_I].hiCalCount, halCal[ADS_I].maxErr);
  Serial.printf("halCal[ESP_I] %2.2f %2.2f %2.2f [%i  %i %i], maxErr %2.1f\n", halCal[ESP_I].lowCalVal, halCal[ESP_I].midCalVal, halCal[ESP_I].hiCalVal, halCal[ESP_I].lowCalCount, halCal[ESP_I].midCalCount, halCal[ESP_I].hiCalCount, halCal[ESP_I].maxErr);
   Serial.printf("halCal[DAC] %2.2f %2.2f %2.2f [%i  %i %i], maxErr %2.1f\n", halCal[DAC].lowCalVal, halCal[DAC].midCalVal, halCal[DAC].hiCalVal, halCal[DAC].lowCalCount, halCal[DAC].midCalCount, halCal[DAC].hiCalCount, halCal[DAC].maxErr);
}
void printADSavg(void)
{
    Serial.printf("ADsAvg %3.3f [%3.3f]: ", meas.ADSvoltsAvg, meas.ADSvolts);
    for(int i = 0; i < NUM_AVG_VOLTS; i++)  
        Serial.printf("%2.3f, ",batVolts[i]);
    Serial.println();
}
void kelvinTest()
{
  digitalWrite(KELPIN, LOW); // should already be in MAIN mode
    float mVolts, kVolts;
    vTaskDelay(500); 
    mVolts = meas.ADSvoltsAvg;
    printADSavg();
    
    digitalWrite(KELPIN, HIGH); //
    vTaskDelay(500); // additional time for relay
    printADSavg();
    kVolts = meas.ADSvoltsAvg;
    
    Serial.printf("KT: vM %2.3f, vK %2.3f\n", mVolts, kVolts);
    digitalWrite(KELPIN, LOW);
}
