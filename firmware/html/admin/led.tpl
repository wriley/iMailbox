<html><head><title>iMailbox - Admin - Status LED</title>
<link rel="stylesheet" type="text/css" href="../style.css">
</head>
<body>
<div id="main">
<p><a href="/">iMailbox</a> - <a href="/admin">Admin</a> - Status LED</p>
<p>
If there's a LED connected to GPIO0, it's now %ledstate%. You can change that using the buttons below. (1 = off, 0 = on)
</p>
<form method="post" action="led.cgi">
<input type="submit" name="led" value="1">
<input type="submit" name="led" value="0">
</form>
</div>
</body></html>