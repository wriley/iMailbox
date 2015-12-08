<html>
<head><title>iMailbox</title>
<link rel="stylesheet" type="text/css" href="style.css">
<script type="text/javascript" src="wifi/140medley.min.js"></script>
<script type="text/javascript">

var xhr=j();

function updateInfo() {
	xhr.open("GET", "status.cgi");
	xhr.onreadystatechange=function() {
		if (xhr.readyState==4 && xhr.status>=200 && xhr.status<300) {
			var data=JSON.parse(xhr.responseText);
			
			var statusDiv=document.getElementById("uptimeSeconds");
			statusDiv.innerHTML=data.result.uptimeSeconds;
			
			var statusDiv=document.getElementById("ledMode");
			if (data.result.ledMode == 0) {
				statusDiv.innerHTML="Off";
			} else {
				statusDiv.innerHTML="On";
			}
			
			var statusDiv=document.getElementById("lightReading");
			statusDiv.innerHTML=data.result.lightReading;
			
			var statusDiv=document.getElementById("batteryStatus");
			statusDiv.innerHTML=data.result.batteryStatus;
			
			window.setTimeout(updateInfo, 5000);
		}
	}
	xhr.send();
}


window.onload=function(e) {
	updateInfo();
};
</script>
</head>
<body>
<div id="main">
<h1>iMailbox</h1>
<p>
<strong>Uptime:</strong> <span id="uptimeSeconds"></span> seconds<br>
<strong>Lights</strong>: <span id="ledMode"></span><br>
<strong>Light Reading</strong>: <span id="lightReading"></span><br>
<strong>Battery Status</strong>: <span id="batteryStatus"></span><br> 
</p>
<hr>
<p>
<a href="/wifi">WiFi config</a>
</p>
</div>
</body></html>
