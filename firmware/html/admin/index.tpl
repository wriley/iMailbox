<html>
<head><title>iMailbox - Admin</title>
<link rel="stylesheet" type="text/css" href="../style.css">
<script type="text/javascript">
var ledMode = %ledMode%;
var ledModeArray=["Off","Single Color","RGB Fade","Single Color Fade","2 Color Fade"];

function createModeSelect() {
	var el = document.getElementById("ledMode");
	for (var i = 0; i < ledModeArray.length; i++) {
		var opt = document.createElement('option');
		opt.value = i;
		opt.innerHTML = ledModeArray[i];
		if(i == ledMode) {
			opt.selected = true;
		}
		el.appendChild(opt);
	}
	
}

window.onload=function(e) {
	createModeSelect();
}
</script>
</head>
<body>
<div id="main">
<p><a href="/">iMailbox</a> - Admin</p>
<p>
<form method="post" action="ledmode.cgi">
<select id="ledMode" name="ledMode">
</select>
<input type="submit" name="submit" value="Set LED Mode">
</form>
<a href="/admin/setcolor.tpl">Set LED Color</a><br>
</p>
</div>
</body></html>
