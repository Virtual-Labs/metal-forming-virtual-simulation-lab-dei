<?php session_start();
ini_set("display_errors","Off");
if($_SESSION['auth']!="rahulMEM103_2016swarupsharma")
{
header("location:mem103.php");
}
else
{
?>
<!DOCTYPE HTML public "-w3c//dtd//xhtml 1.0 strict//en" "http://www.w3.org/tr/xhtml1/dtd/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<title>Manufacturing Processes-I</title>
<link rel="shortcut icon" type="image/x-icon" href="images/icon.ico">
<link href="css/mem.css" rel="stylesheet" type="text/css">
</head>
<body style="background:#FFFFFF; margin:auto; width: 1024px; height:100%;">
<div id="header">
<br/>
<b>MEM-103 Manufacturing Processes-I</b></div>
<div>
<table width="100%"><tr>
<td width="50%" style="font-size:14px; color:#ff0066; font-weight:bold;">Welcome <?php echo $_SESSION['name'];?></td>
<td style="text-align:right;"><a href="Unit3Lesson1.php" title="Lesson 1 Casting Process">Lesson 1 Casting Process</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<a href="memHome.php" title="Manufacturing Process-I">MEM103 Home</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href="mem_out.php" title="Sign out from Manufacturing Process">Logout</a></td>
</tr></table><br/></div>
<div>
<b>Lesson 1 Self-Check Questions</b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href="MEM103/Unit3/Lesson1/Unit3Lesson1scq.pdf" target="_blank" title="Download Self-Check Questions">Self-Check Questions Download</a><br/><br/>
<center><table border="0" width="100%"><tr>
<td valign="top" width="25px">1</td>
<td valign="top">Components having more details can easily produced by:<br/> 
(a)	 extrusion<br/>		
(b)	 hot rolling<br/>
(c)	 casting<br/>			
(d)	 none of them</td>
<td valign="top" width="25px">2</td>
<td valign="top">The bottom half of the casting is termed as:<br/>
(a)	 bottom<br/>
(b)	 cope<br/>
(c)	 drag<br/>
(d)	 end</td></tr>
<tr>
<td valign="top">3</td>
<td valign="top">Cores are used to:<br/> 
(a)	 remove pattern easily<br/>
(b)	 increase strength of mould<br/>
(c)	 make desired recess in casting<br/>
(d)	 uniform distribution of molten metal</td>
<td valign="top">4</td>
<td valign="top">The top half of a casting used in moulding is termed:<br/> 
(a)	 drag<br/>
(b)	 top<br/>
(c)	 set<br/>
(d)	 cope</td>
</tr>
<tr>
<td valign="top">5</td>
<td valign="top">A sprue pin is used in casting to:<br/>
(a)	 hold the cast together<br/>
(b)	 form the hole for the molten metal<br/>
(c)	 make small holes to allow the gas to escape<br/>  
(d)	 smooth the face of the mould</td>
<td valign="top">6</td>
<td valign="top">The tool used to cut a channel to allow molten metal to enter a casting mould is termed a:<br/> 
(a)	channel cutter<br/>
(b)	gate cutter<br/>
(c)	sand cutter<br/>
(d)	ream cutter</td>
</tr>
<tr>
<td valign="top">7</td>
<td valign="top">The main aim of gate is:<br/>
(a)	 to help in feeding casting materials.<br/>
(b)	 to provide storage capacity for molten metal.<br/>
(c)	 to feed the casting at a rate consistent with the rate of solidification.<br/>
(d)	 none of them.</td>
<td valign="top">8</td>
<td valign="top">Cores are used for:<br/>
(a)	 act as the supporting device<br/>
(b)	 give strength to the moulding sand.<br/>
(c)	 both (a) & (b)<br/> 
(d)	 making desired recess in casting.</td>
</tr></table></center><br/><br/>
<b>Possible answers to self check questions</b><br/><br/>
<table border=0 width="250px"><tr>
<td>1</td><td>c</td><td>2</td><td>c</td></tr>
<tr>
<td>3</td><td>c</td><td>4</td><td>d</td></tr>
<tr>
<td>5</td><td>b</td><td>6</td><td>b</td></tr>
<tr>
<td>7</td><td>c</td><td>8</td><td>d</td>
</tr></table>
</div><br/>
<div id="footer">
&copy; MEM103 - Dayalbagh Educational Institute (www.dei.ac.in)</div>
</body>
</html>
<?php
}
 	//Opening file to get counter value
	$fp = fopen ("../counter.txt", "r");
	$count_number = fread ($fp, filesize ("../counter.txt"));
	fclose($fp);
	$counter = (int)($count_number) + 1;
    $count_number = (string)($counter);
	$fp = fopen ("../counter.txt", "w");
	fwrite ($fp, $count_number);
	fclose($fp);
?>