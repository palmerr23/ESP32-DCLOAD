/* update readings quickly, update other controls not as often */ 
loopsWithoutJSON = 0;
currentScreen = 'M';
currentScreenType = 'M';
var dial_el; 
var dial;
var lastDial = 0;
var dialX = 0;
var settingToChange = null;
var settingToChangeName = null;
var buttonMul = 1;
var fetchTime = 1000;	// 1000 for production
var disconLoops = 15000/fetchTime;

sessionID = -1; // Set random at runtime. Sent with every request. 
//var dial_val = 0;

setInterval(
  () => {
	loopsWithoutJSON++;
	ems = document.getElementById("input_errorID"); // errorText
 
	//console.log("sType: no fetch", currentScreenType);
	/*
	if(currentScreenType == 'L') // update data is useless here
	{
		  //console.log("Log: no fetch");
		  return 0;
	}
	*/
	//console.log("Fetching");
	if(loopsWithoutJSON > disconLoops) // (~15 secs) increase value for shorter Intervals
	{
		  console.log("No FETCH JSON " + loopsWithoutJSON);		 
		  ems.value = "Disconnected";
	}
	var sid = document.getElementById("sessionIDM"); // any one will do
	//console.log("Current screen ", currentScreen);
    fetch("/readings?screen="+currentScreen+"&sessionID="+sid.value)
    .then(function (response) {
      return response.json();
    })
    .then(function (myJson) {      
	  loopsWithoutJSON = 0;
	  ems.value = "";
	  //console.log("/readings");
	  // floats - 2 decimals
      for(i of ["voltsR","ampsR","powerR","resistR","voltsS","ampsS","powerS","resistS", "volts1S", "amps1S", "volts2S", "amps2S"]) 
      {
        // get reading or setting i
           //objStr = i.toString;
		 // console.log(i);
		   //console.log(objstr);
          if(i in myJson)
          {          
              ss = myJson[i]; // value to set           
              //console.log(i, ss);
              bs = document.getElementById(i+"ID");
			  if(bs != null)
			  {
              //fs = document.getElementById("lvbar"+(i).toString());
				bs.value = parseFloat(ss).toFixed(2);              
				//bs.setShowSoftInputOnFocus(false);
			  }
          } 
	  }
	  // integer values
	  for(i of ["time1S", "steps1S",  "time2S", "steps2S", "cycS", "riseS", "logtS"]) 
	  {
          if(i in myJson)
          {          
              ss = myJson[i]; // value to set           
              //console.log(i, ss);
              bs = document.getElementById(i+"ID");
			  if(bs != null)
              //fs = document.getElementById("lvbar"+(i).toString());
				bs.value = parseFloat(ss).toFixed(0);              
          } 
	  }
	  
	  // buttons
	  for(i of ["CV", "CC",  "CP", "CR", "NOR", "ON", "BT", "ST",  "OFF"]) 
		  if(i in myJson)
		  {
			// SET checked as well

			var bs = document.getElementById(i);
			var valx = myJson[i].toFixed(0);
			//console.log("Button", i, valx);
			if(bs != null)
				bs.checked = (valx > 0.5) ? true : false; // 1 or 0. Not sure if arithmetic is float here
			setButtonColor(i);		// this may cause a loop, by triggering clickDiv
		  }
      // screens
	  if("screen" in myJson)
	  {             
		 ss = myJson["screen"]; // value to set
		 currentScreen = ss;
		//console.log("Setting currentScreen to",ss); 
		 dispC = "none";
		 dispA = "center";
		 bs = document.getElementById("screenName");
		 sb = document.getElementById("submit"+ss);
		 if(ss == "M") 
		 {
			bs.value = "Main";	
			currentScreenType = 'M';
		 }
		 if (ss == "S")
	     {
		    bs.value = "Step"; 
		    currentScreenType = 'S';					  
	     }
		 
		 // if (ss == "L") // nothing to do.
		 
		 sb.style.backgroundColor = "#e6f7ff";	
		 sb.style.borderColor = "#e6f7ff";
		 //sb.style.color = "white";	
		 bs.style.textAlign = dispA;	// realign ScreenName text	 
	  }	  
	  if("device" in myJson)
	  {
		  ss = myJson["device"]; // value to set
		  sb = document.getElementById("devName");
		  sb.value = ss;
		  //console.log("Device", ss);
	  }
    })
    .catch(function (error) {
      console.log("Error: " + error);
  });
  },
  fetchTime // 500-1000 mS is about right
);

// screen load
function setScreen(element) {
		// a random number to differentiate from other sessions from the same IP - may get changed on each page load?
	if (sessionStorage.getItem('sessionID') == null)
	{
		
		ssval = parseInt(Math.random() * 1000000);
		sessionStorage.setItem('sessionID', ssval); 
	}
	sessionID = sessionStorage.getItem('sessionID');
	//console.log("SessionID", sessionID);
	var scr = document.getElementById("screenID");
	for(i of ['M','S','L']) 
	{ 
		ss = document.getElementById("sessionID"+i)		
		ss.value = sessionID;
	 }
	currentScreen = scr.value;		
	if (document.body.id == "bodyO")
	{
		 currentScreenType = 'S';	//a settings screen
	
		//console.log("starting knob");
		dial_el = document.getElementById("jog_dial");
		var dial_options = {wheelSize:'111px', knobSize:'30px', minDegree:null, maxDegree:null, degreeStartAt: 0};//debug: true, 
		dial = JogDial(dial_el, dial_options).on('mousemove', function(evt){ 
			var delta = 0;

			var dialVar = document.getElementById("dialVal");
			var dTest = document.getElementById("testVal");
			var ddV = document.getElementById("deltaVal");
			var mBut = document.getElementsByName("mulBut");
			var errLine = document.getElementById("input_errorID");
			var decimals = 2;
			if(["time1S", "steps1S",  "time2S", "steps2S", "cycS", "riseS"].includes(settingToChangeName)) // integers
				decimals = 0;
			for(i = 0; i < mBut.length; i++)
				if(mBut[i].checked)
				{
					//console.log("mBut found", i);
					delta = +mBut[i].value;
					ddV.value = delta;
				}
			//console.log("DM", delta);
			var dial_val = evt.target.rotation;
			dialX = 0;
			if( dial_val - lastDial > 5)
			{	
				dialX = +delta;			
				lastDial = dial_val;				
			}
			else if(dial_val - lastDial < -5)
			{
				dialX = -delta;	
				lastDial = dial_val;			
			}
			dialVar.value = dialX; 
			if(dialX == 0)
				return;
			
			if(settingToChange != null) // don't allow overruns
			{ 
				dTest.value = +settingToChange.value + dialX;
				if(+settingToChange.value + dialX > +settingToChange.max)
				{
					settingToChange.value = (+settingToChange.max).toFixed(decimals);
					setChange(settingToChange);
					errLine.value = "Max value";				
				}
				else
					if(+settingToChange.value + dialX < +settingToChange.min)
					{				
						settingToChange.value = (+settingToChange.min).toFixed(decimals);
						errLine.value = "Min value";
						setChange(settingToChange);
					}
					else {
						settingToChange.value = (+settingToChange.value + dialX).toFixed(decimals);
						errLine.value = " ";
						setChange(settingToChange);
					}
			}
		});
	}
	else // bodyL
	{
		 sb = document.getElementById("submitL"); 
		 //sb.style.color = "white";	
		 currentScreenType = 'L';	// log screen/  will be updated on first GET?	 
	}
};

// Channel buttons 
// this may cause a loop, by triggering clickDiv
function setButtonColor(con)
{
	div = "D" +con;
	spa = "S"+con;	// the span
	divEl = document.getElementById(div) 		// Div
	conEl = document.getElementById(con);		// control
   // labEl = document.getElementById(lab);		// label
	spaEl = document.getElementById(spa);		// span
	var oncolor = "blue";
	var found = false;
	for(i of ["CV", "CC", "CP", "CR", "NOR", "ON"]) 
	  if(i == con)
	  {
		oncolor = "green";		// this may cause a loop, by triggering clickDiv
		found = true;
	  }
	for(i of ["BT", "ST", "OFF"]) 
	  if(i == con)
	  {
		oncolor =  "red";
		found = true;
	  }
	if (!found)
		return;
	
	if (conEl.checked)
	{
	//console.log("CHK");
	  divEl.style.background = oncolor; //oncolor;
	  spaEl.style.background = oncolor;
	  spaEl.style.color = "white"
	}
	else
	{
		//	console.log("UN-CHK");
	  divEl.style.background = "#ccc";
	  spaEl.style.background = "#ccc";
	  spaEl.style.color = "black";
	}
	//console.log("setButtonColor ID =", con, div, spa, conEl.checked, oncolor);
};

// clicked a button
function clickDiv(element) {
	var xhr = new XMLHttpRequest();
	//con = (element.id).substring(1); // actual control name
	con = element.id; //(element.id).substring(1,4);
//	lab = "L"+con;	// the label
	div = "D" +con;
	spa = "S"+con;	// the span
	//nam = "N" + con;
	//console.log(con, spa, div);
	divEl = document.getElementById(div); 		// Div
	conEl = document.getElementById(con);		// control
   // labEl = document.getElementById(lab);		// label
	spaEl = document.getElementById(spa);		// span
	//console.log("ClickDiv ID =", con, conEl.checked);
	
	// state changes before we get here
	setButtonColor(con);

	// send 'uncheck' update
	var sv = (conEl.checked) ? +1 : +0 ;
	xhr.open("GET", "/update?cmd=3&value_1="+con+"&value_2="+sv+"&screen="+currentScreen, true); 
	xhr.send();		
};

function selSet(element)
{
	clearSettingBackgrounds();
	el = document.getElementById(element.id) 		
	el.style.backgroundColor = "white";
	//el.style.border = "1px";
	el.blur();
};
function clearSettingBackgrounds()
{
	// ugly but effective!
	var setList;
	if(currentScreen == 'S')
		setlist =["volts1SID", "amps1SID", "time1SID", "steps1SID",  "volts2SID", "amps2SID", "time2SID", "steps2SID", "cycSID", "riseSID"];

	if(currentScreen == 'M')
		setlist =["voltsSID", "ampsSID", "powerSID", "resistSID",  "logtSID"];
	for( i of setlist)
	{
		elx = document.getElementById(i) 	
		elx.style.backgroundColor = "transparent";
		//elx.style.border = "none";
	}

};

 //var slider = document.getElementById("sampleSlider3");  
  // Update the current slider value (each time you drag the slider handle)
function setChange(element) {
    var x3hr = new XMLHttpRequest();
    var strx = "/update?cmd=4&value_1="+element.id+"&value_2="+element.value+"&screen="+currentScreen;
    x3hr.open("GET", strx, true); x3hr.send();
   // console.log("set Change",strx);
};


 var RegExp = new RegExp(/^-?\d*\.?\d*$/); // pos, neg floats or integers only

function selSetting(elem)
{
	settingToChange = elem; // this one gets changed by knob
	settingToChangeName = elem.name;
	//console.log("STC:", settingToChangeName);
}

function valid(elem) {
	var val = elem.value;	
	var errLine = document.getElementById("input_errorID");
	if (RegExp.test(val)) {
		errLine.value = " ";
		// should probably include tests here for max/min being present
		if (+val > +elem.max) 
			errLine.value = "Max = " + elem.max;
		if (+val < +elem.min) 
			errLine.value = "Min = " + elem.min;
	} else {
		//elem.value = val;
		errLine.value = "Illegal char.";
	}
}

function saveLog() 
{
     ems = document.getElementById("input_errorID");
     ems.value ="Calculating log...";
		// console.log("Logging");
	 fileID = document.getElementById("fname");
	 var fname = fileID.value;
	 if(fname.indexOf(".") < 0)
		 fname = fname + ".csv";
	 
     fetch("/log")
     .then(function(response)
     { 
       return response.text();
     })
     .then(function (text)
     {
      document.getElementById("outLog").value = text;
      var userInput = document.getElementById("outLog").value;
      var blob = new Blob([userInput], { type: "text/plain;charset=utf-8" });
      saveAs(blob, fname);
	
     });
	
	  ems.value ="";
};

function getPlot(which)
{
	fetch("/logJSON?plot="+which)
    .then(function (response) {
      return response.json();
    })
    .then(function (myJson) {
//console.log("getPlot", myJson)		
	   drawCanvas(myJson);
	})	
	/*.catch(function (error) {
      console.log("LJ Error: " + error);
  })
  */;
}

function drawCanvas(data)
{
	var canvas = document.getElementById("myCanvas");
	var ctx = canvas.getContext("2d");
	var stepSize;
	var headLine = 30;
	var colAxis = 40;
	var colAxisR = 40;
	var legend = 30;
	var margin = 3;
	var scaleItems = 10;
	var legText = "xx";

	var font1 = "12px Arial";
	var font2 = "18px Arial";

//	console.log("drawCanvas")	
	ctx.beginPath();	// kill any residual unplotted data
	ctx.resetTransform(); // translate stacks
	ctx.clearRect(0, 0, canvas.width, canvas.height);
	ctx.fillStyle = "#FFF"; 	
	ctx.fillRect(0, 0, canvas.width, canvas.height);
	// set scaling and plot window translaation 
	var points = data["points"];	// points to plot
	var logLen = data["logLength"]; // entire length of log
	var plotXmax = canvas.width - colAxis - colAxisR;
	var plotYmax = canvas.height - headLine - legend;
	var xScale = plotXmax / (points -1);

// headline - before translate
	var headText = "Log ("+points+" of "+logLen+" points)";
	ctx.fillStyle = "#000";
	ctx.font = font2;
	ctx.fillText(headText, 150, 20);
	
	// all further display is relative to bottom LHS of plot area
	ctx.translate(colAxis, canvas.height - legend); 

// plot box and horizontal gridlines
	ctx.fillStyle = "#F8F8F8"; // lt grey
	ctx.strokeStyle = "#222";	
	ctx.fillRect(0, 0, plotXmax, - plotYmax);
	ctx.strokeRect(0, 0, plotXmax, - plotYmax);
	
	ctx.strokeStyle = "#AAA";	
	ctx.lineWidth = 1;
	for (i = 1; i < scaleItems; i++) 
	{
		ctx.moveTo(0,-(i * plotYmax / scaleItems));
		ctx.lineTo(plotXmax, -(i * plotYmax / scaleItems));				
	}
	ctx.stroke();

// left axis labels
	var valMax = data["max1"];
	var valMin = data["min1"];
	var yScale = plotYmax / (valMax - valMin);
//	console.log(valMin, valMax); 
	ctx.fillStyle = ctx.strokeStyle = "#F00";
	ctx.font = font1;
	for (i = 0; i < scaleItems; i++) 
	{
		legText = (valMin + i * (valMax - valMin)/(scaleItems -1)).toFixed(2);
		// offset to left of plot area
		ctx.fillText(legText, -colAxis + margin, -i * (canvas.height - legend)/(scaleItems+1));
	}

// data1 plot- left axis
	ctx.font = font2;
	ctx.fillText(data["label1"], 20, 20 );
	
	ctx.fillStyle =	ctx.strokeStyle="#F00";
	ctx.beginPath();
	ctx.moveTo(0, - (data.data1[0] - valMin) * yScale);
	for (i = 1; i < points; i++) 
	{
		ctx.lineTo(i * xScale, - (data.data1[i] - valMin) * yScale);
	}
	ctx.stroke();
// data3 - left axis (always the same scale as data1)
	ctx.fillStyle = ctx.strokeStyle="#0C0";
	ctx.beginPath();
	ctx.moveTo(0, - (data.data3[0] - valMin) * yScale);
	for (i = 1; i < points; i++) 
	{
		ctx.lineTo(i * xScale, - (data.data3[i] - valMin) * yScale);
	}
	ctx.stroke();
	ctx.fillText(data["label3"], 100, 20 );
	
// data3 plot - right axis
	//console.log("plot2")	
	valMax = data["max2"];
	valMin = data["min2"];
	yScale = plotYmax / (valMax - valMin);
//console.log(valMin, valMax); 
	ctx.fillStyle =	ctx.strokeStyle = "#00F";
	ctx.font = font1;
	// right axis labels
	for (i = 0; i < scaleItems; i++) 
	{
	//ctx.moveTo(-colAxis, i* (canvas.height - legend)/scaleItems);
		legText = (valMin + i * (valMax - valMin)/(scaleItems-1)).toFixed(2);
		ctx.fillText(legText, canvas.width - colAxis - colAxisR + margin, -i* (canvas.height - legend)/(scaleItems+1));
	}
	yScale = (canvas.height - headLine - legend) / (valMax - valMin);
	ctx.beginPath();
	ctx.moveTo(0, - (data.data2[0] - valMin) * yScale);
	for (i = 1; i < points; i++) 
	{
		ctx.lineTo(i * xScale, - (data.data2[i] - valMin) * yScale);
	}
	ctx.stroke();
	ctx.font = font2;
	ctx.fillText(data["label2"], 350, 20 );
	// http://www.tutorialspark.com/html5/HTML5_Canvas_dataplot_Demo.php
	/*for (let i in data.v1) {x = JSON.v1[i];}
	ctx.moveTo(0, 0);
ctx.lineTo(200, 100);
ctx.stroke();
*/
};
// no longer used
/*
function openFile(event)
{
    var file = event.target.files[0];
	textTarget = document.getElementById("inJSON");
	if (!file)
	{
		textTarget.value = "No file selected to load.";	
		return;	
	}
    var reader = new FileReader();
    reader.onload = function(e){
		var contents = e.target.result;			 
		textTarget.value = contents;
		// execute the changes!
    };
	reader.readAsText(file);
};
*/
/*
function sendCname(element) {
  var xhr = new XMLHttpRequest();
  var strx = "/update?cmd=5&value_1="+element.id+"&value_2="+element.value+"&screen="+currentScreen;
  xhr.open("GET", strx, true); 
  xhr.send();
  console.log(strx);
};
function sendOname(element) {
  var xhr = new XMLHttpRequest();
  var strx = "/update?cmd=6&value_1="+element.id+"&value_2="+element.value+"&screen="+currentScreen;
  xhr.open("GET", strx, true); 
  xhr.send();
  console.log(strx);
};
*/
/*
 //var slider = document.getElementById("sampleSlider3");  
  // Update the current slider value (each time you drag the slider handle)
function sliderChange(element) {
    var x3hr = new XMLHttpRequest();
    var strx = "/update?cmd=4&value_1="+element.id+"&value_2="+element.value+"&screen="+currentScreen;
    x3hr.open("GET", strx, true); x3hr.send();
    console.log(strx);
};
*/