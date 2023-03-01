// VARIOUS MAX params (e.g. CDAC_OPMAX, PMAXOP)SET LOW FOR TEST
#define PROD
#define SOFT_VERSION 2
//#define NO_ERRORS // don't show screenError messages - mainly fir testing basic TFT functions
/* REMEMBER TO SET DISPLAY TYPE BEFORE COMPILING
 *  control() is a timer-driven xTask 
 *  ADS1115: interrupt driven, converts each used input in turn, adsProcess() has a watchdog
*/
#define  ILI9488 // select one
//#define  ILI9341 //


//#define BOARD_D 
#define WIFI    // faster startup for testing with WiFi off
//#define C_DEBUG // slow down control execution and enable serial diagnostics, for testing
#define PROCESS_EVERY_CDEBUG 10 //  
//#define PRINT_DIAGNOSTICS
bool _printMe = false;
bool startUp = true;
//bool numtest = false;

#ifdef PROD
  #define SPLASH // turn on for production
#else
 #define REALNET // use real network credentials for WiFi testing
#endif

#include "myLInst.h"
#include "myLoad.h"
#include "myLESP32.h"
#include "myLHelpers.h"
#include "myLScreenDefs.h"
#include "myLSwitches.h"
#include "myLADS1115.h"
#include "myLADS1115bits.h"   
#include "myLPOST.h"

//inHAL ADC_HAL = {&readADCHAL, 0, 0, 3.3, 0, 0, 16, false}; // ADS1115
void printfloat(char fmt[], float val, uint16_t highlight, uint16_t x, short y, uint16_t txtcol, uint16_t highcol, uint16_t bgcol);
int readADC(int ADCpin, bool printMe = false);
//long loopcntr;
bool myBlink = false; // set by timer at same time as regular redraw
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
#define TESTPIN FANPIN

long lTimer, mTimer, fTimer, vfTimer, vTimer, cTimer, tTimer, tUsec, cdTimer, lcount = 0; // various processing frequency timers for loop()
#define NBUCKETS 100
int timeBucket[NBUCKETS];
long fineTime;
TaskHandle_t controlTaskHandle;
xTimerHandle controlTimer;
// xtask params
#define TIMER_WAIT 1000   // something long - wait if non-resposnive
#define TIMER_DUR  1     // mS between callbacks. Should be >= 1mS to allow other processing (control loop ~ 500uS)
#define C_STACK    10000  // Stack size for control task
#define TASK_CPU  1
#define TASK_PRI  2   // higher number than loop() == 1
volatile long controlTime = 0;
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
  if(!SPIFFSstart())
    screenError("File system failure", MY_RED, 10, true); 
//Serial.println("SPIFFS started"); listDir();

//Serial.println("After getComms"); listDir();
 // settings from EEPROM
 if(!beginEE())
     Serial.println("Should possibly halt!");

  ts.setRotation(sc.tsRot); // some displays have a rotated touch screen (CAL menu)
  
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
    screenError("Unable to read comms profile\nFactory reset", MY_RED, 10, true);
  }

  // xTask for control()
  xTaskCreatePinnedToCore(controlTask, "cTask", C_STACK, NULL, TASK_PRI, &controlTaskHandle, TASK_CPU);
  controlTimer = xTimerCreate("timerC", pdMS_TO_TICKS(TIMER_DUR), true, (void*)0, controlCallback);
  xTimerStart(controlTimer, TIMER_WAIT);
  controlStart = micros();

//  Serial.printf("%s: value mult = %i\n",(daughter_mul == 2) ? "Daughter board" : "No daughter board", daughter_mul);
#ifdef WIFI
  // remote command handling - Telnet, UDP, SCPI
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

  Serial.println("******** Done setup *******");
  //printSTEP();

 //floattest();

  /* Profile fix ups
  halCal[ESP_T].maxVal =65; 
  halCal[ESP_V].lowCalVal = 0; 
  halCal[ESP_I].lowCalVal = 0.0; 
  halCal[ADS_V].lowCalVal = 0.0; 
  halCal[ADS_I].lowCalVal = 0.0; 
  */

  Serial.println("SCPI Command?");
  delay(50);
  
  lTimer =  mTimer = fTimer = vfTimer = vTimer = cTimer = tTimer = millis();

//isLimiting = true;
 //zeroBuckets();
 //A2Dtest();

}
static void controlCallback(xTimerHandle pxTimer)
 {
    //Serial.print("|");
    vTaskResume(controlTaskHandle);
 }

int count = 0;
#include "myLTouchProcess.h"
#define PROCESS_EVERY_VF 1     //  1 mS set dynamic values (inc slew).
#define PROCESS_EVERY_F 5     //   5 mS - switches (debounce)
#define PROCESS_EVERY_CTRL 10 //  10 mS - log and switch update
#define PROCESS_EVERY_M 100    //  100mS -   screen touch and update
#define PROCESS_EVERY_L 750   // 750 mS - display and readings; fan
#define PROCESS_EVERY_VL 10000  // 10  S - test for EE save
#define PROCESS_EVERY_T 3000         // for tesing/debug only
#define CYCLES_STEP 8
uint16_t potval = 0;

bool updateZero;
short UDPcnt;
long lastADS = 0;
bool dp;
int adsCount = 0;

// all I2C processing called from here to avoid conflicts.
// maybe need to have a MUTEX on ADS reads and DAC writes.
// dacSet()?


long loops = 0;
void controlTask(void *pvParameters)
 {
  long tim;
  while(1) // infinite
  {
    tim = micros();
    adsProcess(); // there was another call in printX()
    //Serial.printf("C" );
    loops++;
     if (_outOn) // only control if output is On
   {
      if(adsSetready) // slow/fine control
      {
        //*********** need to add an errorI for readings out of range.
        //cvtADSReadings(); 
        adsCount++;        
        adsSetready = false;
        //
#ifdef C_DEBUG // slow down control for debug
        //Serial.printf("\n ADS Gap %2.3f ", settingsGap(ADC_ADS));
#endif      
    // only control if required
        if (abs(settingsGap(ADC_ADS)) > C_VARFINE || abs(settingsRatioGap(ADC_ADS)) > C_VARFINE)
        {        
#ifdef C_DEBUG 
         //  Serial.print("< ");
#endif

  //Serial.print("< ");
          DACis = control(ADS_CONTROL); // use ADS, fine tune
        }
      } // fast/coarse
      else
      {     
        float sg  = settingsGap(ADC_ESP);         
        float srg = settingsRatioGap(ADC_ESP);
        int  cCount = 0;
        if(getESPviReadings()) // don't try fast control if readings are out of range
        {
          if (((abs(srg) > 1 + C_VARCOARSER) || (abs(srg) < 1 - C_VARCOARSER)) && (abs(sg) > C_VARCOARSE)) 
          {
#ifdef C_DEBUG 
#endif           
            //Serial.printf("[%c %1.3f, %1.3f: A %1.3f, E %1.3f]\n", (sg > 0) ? '>' : '<', sg ,srg,  meas.ADSamps, meas.ESPamps);
            DACis = control(ESP_CONTROL); // use ESP, coarse only
            //cCount++;
          }
        }
      }
   }
   else
     DACis = dacSet(0);
     
   controlTime += (micros() - tim);

    //vTaskDelay(1000);
     vTaskSuspend(NULL);
  }
   vTaskDelete(NULL);
 }
 long ADStlast;
void loop() /**************** LOOP ***************************/
{ 
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
      if(pSet.modeB == MODE_ST) // mode resets to NOR once cycle(s) are completed
         dynSet = stepCalc();
         

      /*   
      if(pSet.modeB == MODE_NOR) // mode resets to NOR once cycle is completed
          dynSet = pSet;
          */
    }    
    yield(); // let ESP32 background tasks run
     
    if(millis() > fTimer + PROCESS_EVERY_F)
    {   
      fTimer = millis();  
      if(pSet.modeB == MODE_BAT) // mode resets to NOR once cycle is completed
         batTest();
     // Serial.print("F"); delay(1);
      // but.loop(); // encoder switch not used
      butL.loop(); // L & R switches
      butR.loop();
      processOnOffSw();
    }
    yield(); // let ESP32 background tasks run
    
    if(millis() - cTimer > PROCESS_EVERY_CTRL)
    { 
      cTimer = millis();
#ifdef WIFI
      process_SCPI(); // do this faster than we send out messages - or deadly embrace can occur.
#endif
    // logging
    if(_outOn)
    {
      addLog(false); // log after mode and timing analysis; don't force log entry 
    }
   }
   yield(); // let ESP32 background tasks run
   
   if(millis() > mTimer + PROCESS_EVERY_M)
   {  
    mTimer = millis();    
    //if (meas.ADSamps < RESPMIN_I || ((dynSet.mode != MODE_CC) &&  meas.ESPvolts < RESPMIN_V) ) Serial.print(" ESPLO ");
    // detect lost interrupt on adsReady (adSProcessCount should update every 16mS)
    if(adSProcessCount <= lastADS)
    {
       adsReady = true; // simulate an interrupt to kick the ADS process
#ifndef NO_ERRORS
       Serial.printf("*** Lost ADC interrupt, restarting handler after %i millisecs\n", millis()- ADStlast );
#endif
       ADStlast = millis();
    }
    lastADS = adSProcessCount;
    
     // control inputs: encoder, buttons and touch    
     processTouchSwEnc(); 

     if (dirtyScreen)
       redrawScreen();

     if(changedVal_remote)  // update all settings, as something has changed remotely
     {
     // Serial.print('#');
       drawSettings(-1, 0, true); // redraw all settings, nothing selected
       changedVal_remote = false; 
     }
       
     rReading = (iReading < SMALL_DIFF) ? 0.0 : vReading / iReading;
     pReading = vReading * iReading;
     if(pSet.logTime > 0 && pSet.logTime < MINLOG)
     {
       pSet.logTime = MINLOG; 
       valChanged(VAL_CHGD);
     }     
     drawReadings(); // only one per cycle
     count++;
  }
  yield(); // let ESP32 background tasks run
  
  // lower priority  - screen redraw
  if(millis() > lTimer + PROCESS_EVERY_L)
  {
    lTimer = millis();  
    //Serial.printf("ADSV %2.3f\n", meas.ADSvolts);
    meas.ESPtemp = readESP_T(); // update temp 
    if( meas.ESPtemp > TMAXOP) // slow response
    {
      onOffKill(0, false);
      errorI(true, true, meas.ESPtemp, "Maximum temperature exceeded");      
    }
    //setFan(60);
    fanControl();
   //Serial.printf("Fan %i pwm [%2.1f C]\n", fanControl(), meas.ESPtemp);

#ifdef xxxC_DEBUG // slow down control for debug
      DACis = control(ADS_CONTROL);
#endif
     startUp = false; // startUp concluded first time this is called
 
     drawIndicators(currentMenu);
     drawLegends(currentMenu);
     myBlink = !myBlink;  
     if(_outOn == false && updateZero) // re-calibrate zero current offset
     {
        currentZeroCal();
        updateZero = false;
     }
  } 
  yield(); // let ESP32 background tasks run
  
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
    //printDynSet();
   // printADC(true);
      //Serial.printf("Free heap %i\n",ESP.getFreeHeap());
   // printHalCal();
    //printDaughterCal();
    //printSTEP();
    Serial.printf("Heartbeat: Output %s\n", (_outOn) ? "On" : "Off");
    //Serial.printf("Output %s\n", (_outOn) ? "On" : "Off");
   // Serial.printf("pMaxOp %2.2f, dynset.current %2.2f, meas.ADSvolts %3.2f, calc pwr %3.2f, calc cur %2.2f\n", pMaxOp, dynSet.current, meas.ADSvolts, dynSet.current * meas.ADSvolts, pMaxOp / meas.ADSvolts);
   // Serial.printf("ESPamps %2.2f, ADSamps %2.2f\n", meas.ESPamps, meas.ADSamps);
  //  Serial.printf("dynSet.current %2.3f\n", dynSet.current);
   // Serial.printf("Fan %i - done\n",readFan());
     //  Serial.printf("Control time = %i\n", controlTime/loops);
 //   float ctRatio = (1000 * controlTime) / (micros() - controlStart);
  //  Serial.printf("control loop time %5.1f%%\n", ctRatio/10);
   // Serial.println();
  }
  lcount++;
  yield(); // let ESP32 background tasks run
   /*
  loopTimetest(); // fineTime = micros(); needs to be uncommented at the top of loop()
 */
}

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
void A2Dtest(void){
  int vall;
  for (float  i = 0; i < RDACHI_I; )
  {
    vall = ampsToDAC(i);
    //Serial.printf("DAC: %2.3f %i\n", i, vall);
    i = i * 1.2 + 0.01; 
  }
}
// test routine
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
    Serial.printf(", DAC: %i, OutOn %i\n", DACis, _outOn);  
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
  Serial.printf("halCal[DAC].maxVal %2.1f, maxErr %2.1f\n", halCal[ESP_I].maxVal, halCal[ESP_I].maxErr);
}
