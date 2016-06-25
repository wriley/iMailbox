<html><head><title>iMailbox - Admin - Single LED Color</title>
<link rel="stylesheet" type="text/css" href="../style.css">
<link rel="stylesheet" href="themes.css">
<script type="text/javascript" src="colorpicker.min.js"></script>
<script type="text/javascript">
function CheckAndSubmit() {
	var el = document.getElementById("currentColor");
	var hex = el.value;
	if(el.value.length != 6) {
		alert("Invalid color value entered");
		return false;
	}
	el = document.getElementById("rgb_r");
	el.value = parseInt("0x"+hex.substring(0,2));
	console.log("r:"+hex.substring(0,2)+" = "+el.value);
	el = document.getElementById("rgb_g");
	el.value = parseInt("0x"+hex.substring(2,4));
	console.log("g:"+hex.substring(2,4)+" = "+el.value);
	el = document.getElementById("rgb_b");
	el.value = parseInt("0x"+hex.substring(4,6));
	console.log("b:"+hex.substring(4,6)+" = "+el.value);
	return true;
}
</script>
<style type="text/css">
	#picker { width: 200px; height: 200px }
    #slide { width: 30px; height: 200px }
    #example { width: 40px; height: 40px; }
</style>
</head>
<body>
<div id="main">
<p><a href="/">iMailbox</a> - <a href="/admin">Admin</a> - Single LED Color</p>
	<div style="height:300px;">
		<div id="color-picker" class="cp-default">
			<div class="picker-wrapper">
				<div id="picker" class="picker"></div>
				<div id="picker-indicator" class="picker-indicator"></div>
			</div>
			<div class="slide-wrapper">
				<div id="slide" class="slide"></div>
				<div id="slide-indicator" class="slide-indicator"></div>
			</div>
			<div id="example" class="slide-wrapper"></div>
			<form method="post" action="setcolor.cgi" onSubmit="return CheckAndSubmit()">
			<div class="slide-wrapper"><input type="text" name="currentColor" id="currentColor" size="6" maxlength="6" value="%currentColor%"></div>
			<div class="slide-wrapper">
				<input type="hidden" name="rgb_r" id="rgb_r" value="0">
				<input type="hidden" name="rgb_g" id="rgb_g" value="0">
				<input type="hidden" name="rgb_b" id="rgb_b" value="0">
				<input type="submit" name="setcolor" value="Set Color">
			</div>
			</form>
		</div>
		<script type="text/javascript">
			cp = ColorPicker(document.getElementById('slide'), document.getElementById('picker'), 
					function(hex, hsv, rgb, mousePicker, mouseSlide) {
						currentColor = hex;
						ColorPicker.positionIndicators(
							document.getElementById('slide-indicator'),
							document.getElementById('picker-indicator'),
							mouseSlide, mousePicker
						);
						var el = document.getElementById("example");
						el.style.backgroundColor = hex;
						el = document.getElementById("rgb_r");
						el.value = rgb.r;
						el = document.getElementById("rgb_g");
						el.value = rgb.g;
						el = document.getElementById("rgb_b");
						el.value = rgb.b;
						el = document.getElementById("currentColor");
						el.value = hex.replace('#','');
					});
			cp.setHex('#%currentColor%');
		</script>
	</div>
</div>
</body></html>