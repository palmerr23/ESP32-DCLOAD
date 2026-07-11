/* myWiFi.h
*** not WiFi multi
- setup Wifi
*/
#ifndef MYWIFI_H
#define MYWIFI_H

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiServer.h>
//#include <AutoConnect.h>
#include <ESPmDNS.h>
void setHostNameIP(void);
bool sendSSIDpacket(bool);
void updateWiFi(const char *ss, const char *pass, bool newCreds);
void printStoredWiFi();


IPAddress nullIP = { 0, 0, 0, 0 };
/* */
char wifiStatusText[7][20] = {
  "WL_IDLE_STATUS",
  "WL_NO_SSID_AVAIL",
  "WL_SCAN_COMPLETED",
  "WL_CONNECTED",
  "WL_CONNECT_FAILED",
  "WL_CONNECTION_LOST",
  "WL_DISCONNECTED"
  // WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
};


#define CONNWAIT 10

bool wifiBegin() {
  char buf[256] = "";
  char lanList[256] = "";
  int i, d;
  bool res;
  strncpy(myWiFi[0].ssid, ESPNETNAME, SSIDLEN);  // make sure defaults are always available
  strncpy(myWiFi[0].pass, ESPNETPW, NETPASSLEN);
#ifdef REALNET
  strncpy(myWiFi[1].ssid, LOCAL_SS, SSIDLEN);
  strncpy(myWiFi[1].pass, LOCAL_PASS, NETPASSLEN);
#endif
  //printStoredWiFi();
  if (WiFi.status() == WL_CONNECTED)
    WiFi.disconnect();
  myWiFi[0].lastConn = 1;
  wifiStatus = WIFI_DISCON;
  WifiConnected = false;
  IamAP = false;
  SDEBUG1.println("**** Configuring WiFi *****");
  for (i = WIFISTORED - 1; i >= 0; i--)  // Last first
    if (strlen(myWiFi[i].ssid) > 0) {
      strcat(lanList, myWiFi[i].ssid);
      strcat(lanList, "\n");
      //Serial.printf("%s[%s]\n", myWiFi[i].ssid, myWiFi[i].pass);
      SDEBUG1.printf("%s: ", myWiFi[i].ssid);

      WiFi.begin(myWiFi[i].ssid, myWiFi[i].pass);
      sprintf(buf, "%s ", myWiFi[i].ssid);
      d = 0;
      while ((WiFi.status() != WL_CONNECTED) && (d < CONNWAIT)) 
      {
        yield();
        vTaskDelay(1000/portTICK_PERIOD_MS);
        strcat(buf, ">");
        screenError(buf, ERR_BG_A, 0, false);
        SDEBUG1.print(">");
        d++;
      }
      if (WiFi.status() == WL_CONNECTED)
        break;
    }
  SDEBUG1.println();

  // try connecting to saved networks
  // SDEBUG1.printf("Trying existing\nWiFi LAN networks:\n%s\n", lanList);
  //sprintf(buf, "Trying saved SSIDs:\n%s", lanList);
  //screenError(buf, ERR_BG_A, 1, false);
  if (WiFi.status() != WL_CONNECTED) {
    screenError("Failed to connect\nto saved LANs", ERR_BG_A, 5, false);
  } 
  else  // normal connect to existing LAN
  {
    String sss = WiFi.SSID();
    const char *ssp = sss.c_str();
    strcpy(myID.local_ssid, ssp);
    sprintf(buf, "Connected to local LAN\nSSID = %s\n", myID.local_ssid);
    SDEBUG1.printf(buf);
    screenError(buf, ERR_BG_A, 1, false);
    myIP = WiFi.localIP();
    myBroadcastIP = WiFi.broadcastIP();
    mySubnetMask = WiFi.subnetMask();
    //Serial.print("STA broadcast ");
    //Serial.println(myBroadcastIP);
    MDNS.begin(myID.instName);
    setHostNameIP();
    wifiStatus = WIFI_CONN;
    WifiConnected = true;
    //	    strcpy(pSet.conn_ssid, myID.local_ssid);
    updateWiFi(myID.local_ssid, "", false);  // no change, just register connection
    // printStoredWiFi();
    //WiFi.printDiag(SDEBUG1);
    return true;
  }

  // Failed LAN - set up local ESPNET
  //feedLoopWDT();
  WiFi.disconnect();
  delay(500);
  //feedLoopWDT();
  // set up soft AP
  SDEBUG1.printf("Launching as AP+STA: SSID %s\n", myWiFi[0].ssid);
  sprintf(buf, "Failed to connect\nto stored SSIDs\nLaunching %s AP", myWiFi[0].ssid);
  screenError(buf, ERR_BG_A, 5, false);
  WiFi.config(nullIP, nullIP, nullIP);  // cause WiFi to forget previous networks

  if (!WiFi.softAP(myWiFi[0].ssid, myWiFi[0].pass)) {
    SDEBUG2.printf("Failed to launch AP %s\nNo network connection.", myWiFi[0].ssid);
    sprintf(buf, "Wifi - failed to launch ESPNET AP SSID = %s\nNo network connection.\nWill retry later.", myWiFi[0].ssid);
    screenError(buf, ERR_BG_B, 5, false);
    //WiFi.printDiag(SDEBUG1); // returns wrong credentials
    SDEBUG1.println(WiFi.softAPIP());
    return false;
  }

  // set up IP range for AP network
  delay(1000);
  if (WiFi.softAPConfig(myAP_IP, myAP_GATEWAY, myAP_SubnetMask)) {
    SDEBUG1.println("Soft AP Config OK");
    WiFi.printDiag(SDEBUG1);
    SDEBUG1.println(WiFi.softAPIP());
    myIP = WiFi.softAPIP();
    myBroadcastIP = WiFi.softAPBroadcastIP();
    Serial.print("AP broadcast ");
    Serial.println(myBroadcastIP);
    mySubnetMask = cidr2netmask(WiFi.softAPSubnetCIDR());
    WiFi.softAPsetHostname(myID.instName);
    IamAP = true;
    MDNS.begin(myID.instName);
    setHostNameIP();
    wifiStatus = WIFI_SOFTAP;
    WifiConnected = true;
    updateWiFi(myWiFi[0].ssid, "", false);
    // printStoredWiFi();
    SDEBUG1.println(WiFi.softAPIP());
    return true;
  }

  SDEBUG2.println("AP IP config bad");
  WiFi.printDiag(SDEBUG1);
  return false;
}

void wifiEnd(bool wifioff = false) {
  bool ret;
  Serial.printf("Wifi off\n");
  ret = WiFi.softAPdisconnect(wifioff);
  //Serial.printf("Soft AP disconnect %i\n", ret);
  ret = WiFi.disconnect(wifioff);
  //Serial.printf("STA disconnect %i\n", ret);
  WifiConnected = false;
}
void updateHostname(void) {
  // update hostname
  strcpy(myHostName, myID.instName);
  strcat(myHostName, ".local");
}
// set hostname and IP in string format
void setHostNameIP(void) {
  updateHostname();
  sprintf(IPstring, "%i.%i.%i.%i", myIP[0], myIP[1], myIP[2], myIP[3]);

  Serial.printf("Hostname: %s\n", myHostName);
  Serial.printf("IP: %s\n", IPstring);
}

// make a copy of myID net credentials to test for change after OSK edit
// ********** unused???
int copyLAN(int x)  // button callback, argument not used
{
  //Serial.printf("copyLAN Copy lanPkt to myID\n");
  //strcpy(lanPkt.ssid, myID.local_ssid);
  //strcpy(lanPkt.pass, myID.local_pass);
  return CALL_EX;
}
int newLAN(int x)  // button callback, argument not used
{
  char buf[200] = "Wait for EEPROM save\nThen cycle power on unit\n to use new WiFi settings.";
  // send update packet if values have changed
  valChanged(VAL_ID_PROF_MASK | VAL_EE_MASK);  // save to EE if we've been in this menu (whether vals changed or not)
  // copied myID fields to lanPkt before editing
  // alsways send. may need to update other controllers, even if not changed
  //if(strcmp(lanPkt.ssid, myID.local_ssid) || strcmp(lanPkt.pass, myID.local_pass))
  {
    // update stored EE values
    updateWiFi(myID.local_ssid, myID.local_pass, true);

    valChanged(VAL_ID_PROF_MASK | VAL_EE_MASK);
    //Serial.printf("newLAN: Sending SSID pkt\n");
    sendSSIDpacket(false);
    //Serial.printf("newLAN: Send done\n");
    screenError(buf, ERR_BG_A, 5, false);
    //Serial.printf("newLAN: Screen Error\n");
  }
  //else 		Serial.printf("newLAN: Not sending SSID pkt\n");
  return CALL_EX;
}
// *********** Not used.
int deleteAllCredentials(int x)  // button callback, argument not used
{
  //Send set Network to head unit.
  WiFi.disconnect(true, true);  //WiFi.disconnect(true); ?
  return CALL_EX;
}

// if newCreds are true, the new or changed entry is updated and WiFi list is saved to EEPROM
void updateWiFi(const char *ss, const char *pass, bool newCreds) {
  // new entry or changed password
  if (newCreds) {
    int found = -1;
    int oldEmpty = -1;
    // search through for exsiting SSID
    for (int i = 1; i < WIFISTORED; i++)  // never change ESPMON SSID
    {
      if ((strcmp(myWiFi[i].ssid, ss)) == 0)  // existing
      {
        found = i;
        break;
      }
    }
    // search through for oldest or empty SSID
    if (found < 0) {
      for (int i = 1; i < WIFISTORED; i++) {
        // any empty slot
        if (strlen(myWiFi[i].ssid) == 0) {
          found = i;
          break;
        }
        // least recently used entry
        if (myWiFi[i].lastConn > oldEmpty) {
          found = i;
          oldEmpty = myWiFi[i].lastConn;
        }
      }
    }
    // update password and ssid
    // immediate EEsave
    if (found > 0 && newCreds)  // don't ever change default
    {
      strcpy(myWiFi[found].ssid, ss);
      strcpy(myWiFi[found].pass, pass);
      // Serial.printf("Changed WiFi[%i]: %s  %s\n", found, myWiFi[found].ssid, myWiFi[found].pass);
    }
    // next section will set lastConn
    valChanged(VAL_EE_MASK | VAL_ID_PROF_MASK);
  }
 
  for (int i = 0; i < WIFISTORED; i++) {
    if (strcmp(myWiFi[i].ssid, ss) == 0) {
      //strcpy(myID.local_pass, myWiFi[i].pass);
      myWiFi[i].lastConn = 1;
    } else  // increment the others
      myWiFi[i].lastConn = constrain(++myWiFi[i].lastConn, 1, WIFIMAXCONN);
  }
  // save updated last connected counters
  valChanged(VAL_EE_MASK | VAL_ID_PROF_MASK);
  //  printStoredWiFi();
}

void printStoredWiFi(void) {
  Serial.println("Stored WiFi:");
  for (int i = 0; i < WIFISTORED; i++)
    Serial.printf("%i: |%s| |%s| %i\n", i, myWiFi[i].ssid, myWiFi[i].pass, myWiFi[i].lastConn);
}

#endif /* MYWIFI_H */
