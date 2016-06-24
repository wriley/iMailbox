<html>
<head><title>iMailbox</title>
<link rel="stylesheet" type="text/css" href="style.css">
<script type="text/javascript" src="wifi/140medley.min.js"></script>
<script type="text/javascript">
var xhr=j();
var batteryStatusArray=["On Battery","Low Battery","Unknown","Unknown","On Solar","Charging","Charge Complete","Temperature or Timer Fault"];
var ledModeArray=["Off","Single Color","RGB Fade","Single Color Fade","2 Color Fade"];
var ledShowArray=["Off","On"];

function updateInfo() {
	xhr.open("GET", "status.cgi");
	xhr.onreadystatechange=function() {
		if (xhr.readyState==4 && xhr.status>=200 && xhr.status<300) {
			var data=JSON.parse(xhr.responseText);
			
			var statusDiv=document.getElementById("uptimeSeconds");
			statusDiv.innerHTML=secondsToDateString(data.result.uptimeSeconds);
			
			var statusDiv=document.getElementById("ledMode");
			statusDiv.innerHTML=ledModeArray[data.result.ledMode];
			
			var statusDiv=document.getElementById("ledShow");
			statusDiv.innerHTML=ledShowArray[data.result.ledShow];
			
			var statusDiv=document.getElementById("lightReading");
			statusDiv.innerHTML=data.result.lightReading;
			
			var statusDiv=document.getElementById("lightThreshold");
			statusDiv.innerHTML=data.result.lightThreshold;
			
			var statusDiv=document.getElementById("batteryStatus");
			if(data.result.batteryStatus == 1 || data.result.batteryStatus == 7) {
				statusDiv.innerHTML="<font class=\"error\">"+batteryStatusArray[data.result.batteryStatus]+"</font>";
			} else if(data.result.batteryStatus == 6) {
				statusDiv.innerHTML="<font class=\"good\">"+batteryStatusArray[data.result.batteryStatus]+"</font>";
			} else if(data.result.batteryStatus == 4) {
				statusDiv.innerHTML="<font class=\"solar\">"+batteryStatusArray[data.result.batteryStatus]+"</font>";
			} else {
				statusDiv.innerHTML=batteryStatusArray[data.result.batteryStatus];
			}
			
			window.setTimeout(updateInfo, 5000);
		}
	}
	xhr.send();
}

function secondsToDateString(sec_num) {
	var days = Math.floor(sec_num / 86400);
	sec_num -= days * 86400;
	
	var hours = Math.floor(sec_num / 3600);
	sec_num -= hours * 3600;
	
	var minutes = Math.floor(sec_num / 60);
	sec_num -= minutes * 60;
	
	var seconds = sec_num;

    var time = days+'d '+hours+'h '+minutes+'m '+seconds+'s';

    return time;
}

window.onload=function(e) {
	updateInfo();
};
</script>
</head>
<body>
<div id="main">
<p>iMailbox</p>
<p>
<strong>Uptime:</strong> <span id="uptimeSeconds">--</span><br>
<strong>LED Mode:</strong> <span id="ledMode">--</span><br>
<strong>Light Sensor Override:</strong> <span id="ledShow">--</span><br>
<strong>Light Reading:</strong> <span id="lightReading">--</span><br>
<strong>Light Threshold:</strong> <span id="lightThreshold">--</span><br>
<strong>Battery Status:</strong> <span id="batteryStatus">--</span><br> 
</p>
<div style="text-align: right;">
<p><a href="/admin">admin</a></p>
</div>
</div>
</body></html>
