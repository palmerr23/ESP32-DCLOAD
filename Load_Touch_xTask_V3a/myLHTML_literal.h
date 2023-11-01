/* LITERAL HTML */
#ifndef MYLITERAL_H
#define MYLITERAL_H
// main page code
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>DC Load</title>
  <meta name="viewport" content="width=840, initial-scale=1, user-scalable=yes, minimum-scale=0.4">
 <link rel="stylesheet" href="myLoad.css"> 
</head>
<body onload="setScreen()" id="bodyO">
	<div class="pg_container">
	  <div class="buttonTop">
		  <input type="text" readonly id="devName" class="headlineL" size=5 value ="XXX">
		  <input type="hidden" id="screenName" class="headlineS" value ="XXX" style="pointer-events: none; text-align: center;">
		  %SCREENBUTTONS% 
	  </div>
	  <div class="tm_container">   
	    %SETTINGS%
      </div> 
  </div>
  <script src="myLoad.js"></script>
  <script src="jogDial.min.js"></script>  
</body>
</html>
)rawliteral";
/*
<input type="text" id="oName" class="headlineO" value ="Waiting..." onChange="sendOname(this)" size="10" maxlength="10">
  */
// log page code
const char settings_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>DC Load</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
 <link rel="stylesheet" href="myLoad.css"> 
</head>
<body onload="setScreen()" id="bodyS">
	<div class="pg_container" >
		<div class="buttonTop">
		  <input type="text" readonly id="devName" class="headlineL" size=5 value ="XXX">
		  <input type="hidden" id="screenName" class="headlineS" value ="Log" style="pointer-events: none; text-align: center;">
	   	  %SCREENBUTTONS%
	    </div>
	  <div class="tm_container" style="width: 500px" >
		<div class="row" id="all_sets">        
		  <h3>Save Log to File</h3> 
		  <label for="fname">Save as: </label> 
		  <input type=type='text' id="fname" name="fname" value="loadLog"> 
		  <button type="button" onclick="saveLog();">Click to Save Log</button><br><br>  
		  <textarea id="outLog" rows="4" cols="60">Log data will load here and then save as CSV using the filename above.</textarea>
		</div>
	  </div>  
	  <BR>
	  <div class="tm_container" >
		 <h3>Plot Log</h3> 
		 <button type="button" onclick="getPlot(1)">Whole log</button>&nbsp; &nbsp;
		 <button type="button" onclick="getPlot(2)">Latest 10</button><br> <BR>
		 
		 <canvas id="myCanvas" width="500" height="250" style="border:1px solid #000000;"></canvas> 
		 </div><br>

	  <input type="text" id="input_errorID" readonly class="blinking" value ="" style="color: red"><BR>
	</div>
  <script src="FileSaver.js"></script>
  <script src="myLoad.js"></script>   
</body>
</html>
)rawliteral";
/*
		<input type="text" hidden readonly id="devName" class="headlineS" size=4 value="XXX">
     <h3>Load Preset</h3>
      <label for="getFilefile">Preset file to load: </label>
      <input id='getFile' type='file' onchange='openFile(event)'><BR><br>
      <textarea id="inJSON" rows="6" cols="60">JSON preset from file will load here, and then be activated.</textarea>
	  

*/

/*
		  %SETTINGSGROUP%
	  <div class="bm_container">

	  </div> <BR> 
	  
	  
	   <input type="text" readonly id="errorText" class="blinking" value ="" style="color: red"><BR>
	  */
#endif
