<html><head><title>iMailbox - Light Threshold</title>
<link rel="stylesheet" type="text/css" href="../style.css">
<link rel="stylesheet" href="themes.css">
<script type="text/javascript">
function CheckAndSubmit() {
	var el = document.getElementById("lightThreshold");
	var hex = el.value;
	if(el.value < 0 || el.value > 65535) {
		alert("Invalid threshold value entered");
		return false;
	}
	console.log("lightThreshold: "+el.value);
	return true;
}
</script>
</head>
<body>
<div id="main">
<p><a href="/">iMailbox</a> - <a href="/admin">Admin</a> - Light Threshold</p>
<div style="height:300px;">
	<form method="post" action="setlightthreshold.cgi" onSubmit="return CheckAndSubmit()">
		<input type="text" name="lightThreshold" id="lightThreshold" size="6" maxlength="6" value="%currentLightThreshold%"><br>
		<input type="submit" name="setlightthreshold" value="Set Light Threshold">
	</form>
</div>
</body></html>