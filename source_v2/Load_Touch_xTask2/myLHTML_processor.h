//Processor code 
// Replaces placeholders in the web page literal code
// Control element numbers have first three chars identify control, 5th char = channel number [0.. INCHANS-1]
#ifndef MYLHTTPX_H
#define MYLHTTPX_H
//#include <sstream>
char JSONstring[4096];
char currentWebScreen = 'M'; // start with iNput screen
char xbuf[64];
#define NUMSCREENS 3
// MAIN, STEP, LOG
char screenNames [][5] = {"Main","Step", "Log"}; // goes on the button and in headline
char screenList[] = {'M','S','L'}; // actual reference
String readingsBlock(void);
String modeButtons(void);
String modeButtonsB(void);
String knobBlock(void);
String stepBlocks(void);
void printSet();

int _thisClient = -1; // index to the client array, so that the right update data can be sent to each separate session.
char _thisScreen = 'M'; // HTTP screen that the request was issued from, defaults to Main settings screen
int _thisSession = -1;  // unique (randomID for session)
String processor(const String& var){
  //Serial.println(var);
   String buttons = "";

// generate entire channel group = N *(cName, level, Fader, Pad)
  if(var == "SETTINGS")
  { 
	if(_thisScreen == 'M')
	{
		buttons += "<div class=\"row\" id=\"all_sets\">";    
		   buttons += modeButtons();
		   buttons += readingsBlock( );
		
		   buttons += knobBlock();
		buttons += "</div>";
		// new row for the modeB buttons
		buttons += "<div class=\"row\" style=\"width:100%%;\">"; // String escaped % == %%  
		buttons += "<div class=\"column2\" style=\"background-color: #e6f7ff;\">"; // width:274px; float: center;width:400px; 
		   buttons += modeButtonsB( );
		buttons += "</div></div>";
	}
	else
		if(_thisScreen == 'S')
		{
			 buttons += "<div class=\"row\" id=\"all_sets\">";    
			 //buttons += "<H2>STEP screen</h2>";
			 buttons += stepBlocks( );			
			 buttons += knobBlock();
			 buttons += "</div></div>";
		}
    return buttons;
  }
  if(var == "SCREENBUTTONS")
  {
	 String scn = (String)_thisScreen;   // current client screen
	 String cli = (String)_thisClient;   // this client's ID (-1 if undefined, but shouldn't be!)
	 String sess = (String)_thisSession;
     String action;

     for (int i = 0; i < NUMSCREENS; i++)
     {
       String cc = (String)screenList[i];  // actual ID
	   char   c  = screenList[i];
       String cn = (String)screenNames[i]; // button and heading name
	  
       if(cc == "L")
          action = "/settings"; // was settings now need to fix Main/Log?.
       else
        action = "/";
       buttons += "<form action=\"" + action + "\" class=\"buttonForm\" method=\"post\">"; // 
       buttons += "<input type=\"text\" id=\"screenFI" + cc + "\" name=\"screen\" value=\"" + cc + "\" style=\"display: none;\">";
	   buttons += "<input type=\"hidden\" id=\"sessionID"+cc+"\" name=\"sessionID\" value=\"" + sess + "\" style=\"display: none;\">"; // dummy for SessionID value - filled in by JS on client
       buttons += "<input type=\"submit\" size= 5 id=\"submit"+cc+"\" class=\"screenChangeL\" value=\" ";
       buttons +=  cn + "\"  style=\"background-color : #9cf\">";
       buttons += "</form> ";
     }

	 // screen ID just used by JS
	  buttons += "<input type=\"hidden\" id=\"screenID\" name=\"screenID\" value=\"" + scn + "\" style=\"display: none;\">";
     return buttons;
  }
  // for js to decodes /levels fetches
  if (var == "BODYTYPE") // not called from settings screen
  {
	  String btype ="O";
	  if(_thisScreen == 'N')
	  {
		  btype = "N";
	  }
	  buttons += "<body onload=\"setScreen()\" id=\"body"+ btype+ "\">";
	  return buttons;
  }
   return String(); // empty string
}

//the readings block
String readingsBlock(void)
{
  char id = '0';
  String html = "";
  String idS = ""; // char id to string
  String bit;
  idS += id;
  //char buf[20];
 // String mins, maxs;
  //std::ostringstream cvt;
  int idN = idS.toInt();

  // embedded in a container and Row
  //READINGS
   html += "<div class=\"column\"id=\"rCol\"  style=\"background-color: #e6f7ff; width:110px;\" >";  

		//READINGS	 - not editable
	   html += "<div class=\"row\" style=\"font-weight: bold; \">Readings</div>\n"; // width:120px;
	   html += "<div class=\"row\"><input readonly type=\"text\" class=\"reads\" id=\"voltsRID\"  name=\"voltsR\"  value=\" \" style=\"color:red;\" maxlength=\"8\" size=\"3\"><span class=\"unit\" style=\"color:red;\">V</span></div>\n"; //size 3 seems to be big enough
	   html += "<div class=\"row\"><input readonly type=\"text\" class=\"reads\" id=\"ampsRID\"   name=\"ampsR\"   value=\" \" style=\"color:green;\" maxlength=\"8\" size=\"3\"><span class=\"unit\" style=\"color:green;\">A</span></div>\n";
	   html += "<div class=\"row\"><input readonly type=\"text\" class=\"reads\" id=\"powerRID\"  name=\"powerR\"  value=\" \" style=\"color:DarkOrange;\" maxlength=\"8\" size=\"3\"><span class=\"unit\" style=\"color:DarkOrange;\">W</span></div>\n";
	   html += "<div class=\"row\"><input readonly type=\"text\" class=\"reads\" id=\"resistRID\" name=\"resistR\" value=\" \" style=\"color:blue\" maxlength=\"8\" size=\"3\"><span class=\"unit\" style=\"color:blue;\">&Omega;</span></div>";

   html += "</div>"; // col
   //SETTINGS - these should be editable

   // add min, max pattern attributes???
    html += "<div class=\"column\" id=\"sCol\" style=\"background-color: #e6f7ff; width:115px; \" >";  
	   html += "<div class=\"row\" style=\"font-weight: bold; \">Settings</div>\n"; 
	   // for settings need to both select this one (one mousedown) for jog wheel; and validate any changes
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"voltsSID\"  name=\"voltsS\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(VMAXOP) +"\" min=\"" + String(VDROPOUT) + "\" value=\" \" style=\"color:red; \" size=\"3\"><span class=\"unit\" style=\"color:red;\">V</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"ampsSID\"   name=\"ampsS\"  onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(IMAXOP) +"\" min=\"" + String(IDROPOUT) + "\" value=\" \" style=\"color:green; \" size=\"3\"><span class=\"unit\" style=\"color:green;\">A</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"powerSID\"  name=\"powerS\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(PMAXOP) +"\" min=\"" + String(PDROPOUT) + "\" value=\" \" style=\"color:DarkOrange; \" size=\"3\"><span class=\"unit\" style=\"color:DarkOrange;\">W</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"resistSID\" name=\"resistS\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(RMAXOP) +"\" min=\"" + String(RDROPOUT) + "\" value=\" \" style=\"color:blue; \" size=\"3\"><span class=\"unit\" style=\"color:blue;\">&Omega;</span></div>\n";
	    html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"logtSID\" name=\"logtS\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(MAXUINTVAL) +"\" min=\"" + String(0) + "\" value=\" \" style=\"color:black; \" size=\"3\"><span class=\"unit\" style=\"color:black;\">mS</span></div>\n";
	   // validation text
	  // html += "<div class=\"row\"><input type=\"text\" maxlength=\"16\" class=\"errline\" readonly id=\"input_error\" value =\" \" style=\"color: red;\" size=\"9\"></div>\n"; // smaller chars, bigger size
   //html += "<div class=\"row\">";

   html += "</div>"; // col
 /*     
   // levels and fader  

     html += "<div class=\"progressV\" id=\"bvbar"+idS+"\"> <div class=\"progress_barV\" id=\"lvbar"+idS+"\"></div></div>"; //
     html += "<div class=\"slider-block\"> <input orient=\"vertical\" type=\"range\" id=\"fad_"+idS+"\" step=\"0.25\" class=\"slidery\" onchange=\"sliderChange(this)\"/></div>";
*/
   
   //html += "</div>  "; //  column  row </div>
   return html;
}
String stepBlocks(void)
{
  char id = '0';
  String html = "";
  String idS = ""; // char id to string
  String bit;
  idS += id;
  //char buf[20];
 // String mins, maxs;
  //std::ostringstream cvt;
  int idN = idS.toInt();

   // embedded in a container and Row
   //S1	
   html += "<div class=\"column\"  id=\"rCol\" style=\"background-color: #e6f7ff; width:120px;  \" >";  
		 
	   html += "<div class=\"row\"  style=\"font-weight: bold;\">S1</div>\n";
	   // for settings need to both select this one (one mousedown) for jog wheel; and validate any changes
	   // blur() takes the focus off the input for mobile.
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"volts1SID\"  name=\"volts1S\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(VMAXOP) +"\" min=\"" + String(VDROPOUT) + "\" value=\" \" style=\"color:red;\" size=\"3\"><span class=\"unit\" style=\"color:red;\">V</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"amps1SID\"   name=\"amps1S\"  onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(IMAXOP) +"\" min=\"" + String(IDROPOUT) + "\" value=\" \" style=\"color:green;\" size=\"3\"><span class=\"unit\" style=\"color:green;\">A</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"time1SID\"  name=\"time1S\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(MAXUINTVAL) +"\" min=\"" + String(0) + "\" value=\" \" style=\"color:DarkOrange;\" size=\"3\"><span class=\"unit\" style=\"color:DarkOrange;\">mS</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"steps1SID\" name=\"steps1S\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(MAXUINTVAL) +"\" min=\"" + String(0) + "\" value=\" \" style=\"color:blue;\" size=\"3\"><span class=\"unit\" style=\"color:blue;\">stp</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"riseSID\" name=\"riseS\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(MAXUINTVAL) +"\" min=\"" + String(0) + "\" value=\" \" style=\"color:blue;\" size=\"3\"><span class=\"unit\" style=\"color:blue;\">mS</span></div>\n";
	   // validation text
	  // html += "<div class=\"row\"><input type=\"text\" maxlength=\"16\" class=\"errline\" readonly id=\"input_error\" value =\" \" style=\"color: red;\" size=\"9\"></div>\n"; // smaller chars, bigger size
   html += "</div>"; // col
   //S2    
   html += "<div class=\"column\" id=\"sCol\" style=\"background-color: #e6f7ff; width:120px; \" >";  
	   html += "<div class=\"row\" style=\"font-weight: bold;\">S2</div>\n";
	   // for settings need to both select this one (one mousedown) for jog wheel; and validate any changes
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"volts2SID\"  name=\"volts2S\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(VMAXOP) +"\" min=\"" + String(VDROPOUT) + "\" value=\" \" style=\"color:red;\" size=\"3\"><span class=\"unit\" style=\"color:red;\">V</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"amps2SID\"   name=\"amps2S\"  onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(IMAXOP) +"\" min=\"" + String(IDROPOUT) + "\" value=\" \" style=\"color:green;\" size=\"3\"><span class=\"unit\" style=\"color:green;\">A</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"time2SID\" name=\"time2S\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(MAXUINTVAL) +"\" min=\"" + String(0) + "\" value=\" \" style=\"color:DarkOrange;\" size=\"3\"><span class=\"unit\" style=\"color:DarkOrange;\">mS</span></div>\n";
	   html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"steps2SID\" name=\"steps2S\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(MAXUINTVAL) +"\" min=\"" + String(0) + "\" value=\" \" style=\"color:blue;\" size=\"3\"><span class=\"unit\" style=\"color:blue;\">stp</span></div>\n";
	    html += "<div class=\"row\"><input type=\"none\" class=\"reads\" id=\"cycSID\" name=\"cycS\" onfocus=\"selSet(this);\" onmousedown=\"selSetting(this)\" oninput=\"valid(this)\" max=\""+ String(MAXUINTVAL) +"\" min=\"" + String(0) + "\" value=\" \" style=\"color:blue;\" size=\"3\"><span class=\"unit\" style=\"color:blue;\">cyc</span></div>\n";
	   // validation text
	  //
   //html += "<div class=\"row\">";

   html += "</div>"; // col
 /*     
   // levels and fader  

     html += "<div class=\"progressV\" id=\"bvbar"+idS+"\"> <div class=\"progress_barV\" id=\"lvbar"+idS+"\"></div></div>"; //
     html += "<div class=\"slider-block\"> <input orient=\"vertical\" type=\"range\" id=\"fad_"+idS+"\" step=\"0.25\" class=\"slidery\" onchange=\"sliderChange(this)\"/></div>";
*/
   
   //html += "</div>  "; //  column  row </div>
   return html;
}
String knobBlock(void)
{
	String html = "";
	html += "<div class=\"column\" id=\"dColID\" name=\"dCol\"  style=\"background-color: #e6f7ff;\" >";  
	//KNOB - attaches to last selected setting
	 html += "<div id=\"jog_dial\" style=\"height:110px; width:110px;\"></div>"; //class=\"dialX\" 
     //000.00 buttons	
	 html += "<input type=\"radio\" name=\"mulBut\" id=\"mulBut100\" value=100 size=1><input type=\"radio\" name=\"mulBut\" id=\"mulBut10\" value=10 size=1><input type=\"radio\" name=\"mulBut\" id=\"mulBut1\" value=1 size=1 checked>";
	 html += "<span style=\"font-weight: bold; font-size:24px;\">.</span><input type=\"radio\" name=\"mulBut\" id=\"mulBut01\" value=0.1 size=1><input type=\"radio\" name=\"mulBut\" id=\"mulBut001\" value=0.01 size=1><br>";
	// hidden calculation fields - remove from here and js file for production.
	html += "<input type=\"text\" hidden id=\"dialVal\" value=0 size=1> ";	// hide when not needed - code uses them
	 html += "<input type=\"text\" hidden id=\"deltaVal\" value=0 size=1>";	
	 html += "<input type=\"text\" hidden id=\"testVal\" value=0 size=1>";	
	 // error line for all inputs
	 html += "<input type=\"text\" maxlength=\"16\" class=\"errline\" readonly name=\"input_error\" id=\"input_errorID\" value =\" \" style=\"color: red;\" size=\"9\">\n"; 
	html += "</div>"; // col
	
    return html;
}
String modeButtons(void)
{
	String html = "";
 html += "<div class=\"column\" id=\"sCol\" style=\"background-color: #e6f7ff;\" padding-top: 10px; padding-bottom: 0px;>";  // not sure we need to identify this
   // buttons
   //html += "<div class=\"row\">";

	html += "<div id =\"DCV\" class=\"divz\"><label id =\"LCV\" class=\"lpad\" >";
	html += "<input type=\"checkbox\" id=\"CV\" name=\"NCV\" class=\"btnq\" onclick=\"clickDiv(this)\"/><span id =\"SCV\" class=\"btnz\">CV</span></label></div><BR> "; // CV but     

	html += "<div id =\"DCC\" class=\"divz\"><label id =\"LCC\" class=\"lpad\" >";
	html += "<input type=\"checkbox\" id=\"CC\" name=\"NCC\" class=\"btnq\" onclick=\"clickDiv(this)\"/><span id =\"SCC\" class=\"btnz\">CC</span></label></div><BR> "; // CC but  

	html += "<div id =\"DCP\" class=\"divz\"><label id =\"LCP\" class=\"lpad\" >";
	html += "<input type=\"checkbox\" id=\"CP\" name=\"NCP\" class=\"btnq\" onclick=\"clickDiv(this)\"/><span id =\"SCP\" class=\"btnz\">CP</span></label></div><BR> "; // CP but  

	html += "<div id =\"DCR\" class=\"divz\"><label id =\"LCR\" class=\"lpad\" >";
	html += "<input type=\"checkbox\" id=\"CR\" name=\"NCR\" class=\"btnq\" onclick=\"clickDiv(this)\"/><span id =\"SCR\" class=\"btnz\">CR</span></label></div><BR> "; // CR but  
 html += "</div>"; // column - </div> row
 return html;
}
// ST, BAT, NOR + ON OFF
String modeButtonsB(void)
{
	String html = ""; 
	html += "<div style=\" position: relative; display: table-cell; float: left; width: 25px;\">&nbsp;</div>"; // SPACER
	
	html += "<div id =\"DBT\" class=\"divz\" ><label id =\"LBT\" class=\"lpad\" >";
	html += "<input type=\"checkbox\" id=\"BT\" name=\"NBT\" class=\"btnq\"  onclick=\"clickDiv(this)\"/><span id =\"SBT\" class=\"btnz\">BT</span></label></div> "; // BT but 

	html += "<div id =\"DST\" class=\"divz\"><label id =\"LST\" class=\"lpad\" >";
	html += "<input type=\"checkbox\" id=\"ST\" name=\"NST\" class=\"btnq\" onclick=\"clickDiv(this)\"/><span id =\"SST\" class=\"btnz\">ST</span></label></div> "; // ST but 

	html += "<div id =\"DNOR\" class=\"divz\"><label id =\"LNOR\" class=\"lpad\" style=\" \">";
	html += "<input type=\"checkbox\" id=\"NOR\" name=\"NNOR\" class=\"btnq\" onclick=\"clickDiv(this)\"/><span id =\"SNOR\" class=\"btnz\">NOR</span></label></div> ";  // NOR but 

	html += "<div style=\" position: relative; display: table-cell; float: left; width: 50px;\">&nbsp;</div>"; // SPACER

	html += "<div id =\"DON\" class=\"divz\"><label id =\"LON\" class=\"lpad\" style=\" \">";
	html += "<input type=\"checkbox\" id=\"ON\" name=\"NON\" class=\"btnq\" onclick=\"clickDiv(this)\"/><span id =\"SON\" class=\"btnz\">On</span></label></div> ";  // On but 
	
	html += "<div style=\" position: relative; display: table-cell; float: left; width: 50px;\">&nbsp;</div>"; // SPACER
	
	html += "<div id =\"DOFF\" class=\"divz\"><label id =\"LOFF\" class=\"lpad\" style=\" \">";
	html += "<input type=\"checkbox\" id=\"OFF\" name=\"NOFF\" class=\"btnq\" onclick=\"clickDiv(this)\"/><span id =\"SOFF\" class=\"btnz\">Off</span></label></div> ";  // On but    // Label id name must be "L" + checkbox.id (js update)

	html += "<div style=\" position: relative; display: table-cell; float: left; width: 25px;\">&nbsp;</div>"; // SPACER
   // small checkbox, opacity = 0 to hide
 return html;
}
#endif