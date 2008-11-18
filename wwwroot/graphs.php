<?
require ('auth.php');

$debug=0;

if (!$debug) header ("Content-type: image/png");
$props=array();
$props['sid']=(int)$_GET['sid'];
$props['gid']=(int)$_GET['gid'];
//$uid=(int)$_GET['uid'];
//Image dimensions:
$props['width']=(int)$_GET['x'];
$props['height']=(int)$_GET['y'];
if (!$props['width']) $props['width']=640;
if (!$props['height']) $props['height']=480;

//Create image
$myImage = imagecreatetruecolor($props['width'], $props['height']);

//Fill background:
$white = imagecolorallocate($myImage, 255, 255, 255);
imagefilledrectangle  ($myImage,0,0,$props['width'],$props['height'],$white);

//define colors:
$gridcolor = imagecolorallocate($myImage,0,0,0);
$in_color = imagecolorallocatealpha($myImage,0,255,0,30);
$out_color = imagecolorallocatealpha($myImage,255,0,0,30);

//Get scaling parameters:
$props['graphwidth']=$props['width']-100;
$props['graphheight']=$props['height']-50;
$props['x_zero']=10;
$props['y_zero']=$props['graphheight']+10;
$props['dt']=0;

//Autoscale X:
$query='select min(UNIX_TIMESTAMP(stat_time)),max(UNIX_TIMESTAMP(stat_time)) from session_statistics where session_id='.$props['sid'];
if ($props['gid']) $query.= ' and zone_group_id='.$props['gid'];
$res=$mysqli->query($query);
if ($res){
	$l=$res->fetch_row();
	$props['start']=(int)$l[0];
	$props['end']=(int)$l[1];
	$props['dt']=(int)round(($props['end']-$props['start'])/$props['graphwidth'])+1;
	$props['dx']=1/$props['dt'];
	if ($props['dt']<60){
		$props['dt']=60;
		$props['dx']*=60;
	}else{
		$props['dx']=1;
	}
};

//Autoscale Y:
$query='SELECT max(sum_in),max(sum_out) FROM (SELECT sum(traf_in) AS sum_in,sum(traf_out) AS sum_out from session_statistics where session_id='.$props['sid'];
if ($props['gid']) $query.= ' and zone_group_id='.$props['gid'];
$query.=' group by UNIX_TIMESTAMP(stat_time) div '.$props['dt'].') AS sums';
if ($debug)echo $query."<br>";

$res=$mysqli->query($query);
if ($res){
	$l=$res->fetch_row();
//$props['dx']=1/$props['dt'];
	$props['max_in']=(int)$l[0];
	$props['max_out']=(int)$l[1];
	$props['max_traf']=max($props['max_in'],$props['max_out']);
	$props['dy']=$props['graphheight']/$props['max_traf'];
};

function getgraphx($props,$time){
	return (int)(round(($time-$props['start'])/$props['dt'] *$props['dx'])+$props['x_zero']);
};

function getgraphy($props,$traf){
	return (int)($props['y_zero']-round($traf*$props['dy']));
};

$query='select sum(traf_in),sum(traf_out),zone_group_id,UNIX_TIMESTAMP(stat_time) div '.$props['dt'].' from session_statistics where session_id='.$props['sid'];
if ($props['gid']) $query.= ' and zone_group_id='.$props['gid'];
$query.=' group by UNIX_TIMESTAMP(stat_time) div '.$props['dt'];
if ($debug)echo $query."<br>";
$res=$mysqli->query($query);
$xprev=$props['x_zero'];
if ($res){
	while($l=$res->fetch_row()){
		if ($debug) echo getgraphy($props,$l[0]).' -- '.getgraphy($props,$l[1]).' -- '.getgraphx($props,$l[3]*$props['dt']).' -- '.(getgraphx($props,$l[3]*$props['dt'])+$props['dx'])."<br>";
		$props['x_zero'];
		if (!$debug) imagefilledrectangle($myImage,$xprev,$props['y_zero'],getgraphx($props,$l[3]*$props['dt']),getgraphy($props,$l[0]),$in_color);
		if (!$debug) imagefilledrectangle($myImage,$xprev,$props['y_zero'],getgraphx($props,$l[3]*$props['dt']),getgraphy($props,$l[1]),$out_color);
		//imageLine($myImage,$l[3]-$start_time,480,$l[3]-$start_time,480-round(($l[0]/$maxtrafin)*480),$in_color);
		//imageLine($myImage,$l[3]-$start_time,480,$l[3]-$start_time,480-round(($l[1]/$maxtrafout)*480),$out_color);
		$xprev=getgraphx($props,$l[3]*$props['dt'])+1;
	};
};
if ($debug)print_r(var_dump($props));
if (!$debug)imagepng($myImage);
imagedestroy($myImage);
?>
