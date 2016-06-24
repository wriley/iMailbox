<html><head><title>iMailbox - Admin - Light Sensor Override</title>
<link rel="stylesheet" type="text/css" href="../style.css">
<script type="text/javascript">
var ledShow = %ledShow%;
var ledShowArray=["Off","On"];

function createShowSelect() {
	var el = document.getElementById("ledShow");
	for (var i = 0; i < 2; i++) {
		var opt = document.createElement('option');
		opt.value = i;
		opt.innerHTML = ledShowArray[i];
		if(i == ledShow) {
			opt.selected = true;
		}
		el.appendChild(opt);
	}
}

window.onload=function(e) {
	createShowSelect();
}
</script>
</head>
<body>
<div id="main">
<p><a href="/">iMailbox</a> - <a href="/admin">Admin</a> - Light Sensor Override</p>
<p>
Light Sensor Override
<br>
<form method="post" action="setledshow.cgi">
<select id="ledShow" name="ledShow">
</select>
<input type="submit" name="submit" value="Set Light Sensor Override">
</form>
</div>
</body></html>
