<?
require ('auth.php');

function getgraphx($props,$time){
	return (int)(round(($time-$props['start'])/$props['dt'] *$props['dx'])+$props['x_zero']);
};

function getgraphy($props,$traf){
	return (int)($props['y_zero']-round($traf*$props['dy']));
};

$debug=0;

if (!$debug) header ("Content-type: image/png");
$props=array();
$props['sid']=(int)$_GET['sid'];
$props['uid']=(int)$_GET['uid'];
$props['start']=(int)$_GET['start'];
$props['end']=(int)$_GET['end'];
if ($props['uid']&&(!$props['start'])){
	$props['start']=mktime(0, 0, 0, date("m"), date("d")-3, date("Y"));
	$props['end']=time();
};
$props['gid']=(int)$_GET['gid'];
//$uid=(int)$_GET['uid'];
//Image dimensions:
$props['width']=(int)$_GET['width'];
$props['height']=(int)$_GET['height'];
if (!$props['width']) $props['width']=800;
if (!$props['height']) $props['height']=480;

$props['head_height']=10;
$props['x_zero']=60;
$props['y_zero']=$props['height']-50;
$props['graphwidth']=$props['width']-$props['x_zero'];
$props['graphheight']=$props['y_zero']-$props['head_height'];
$props['dt']=0;
$props['grid_font_size']=8;
$props['grid_font']='lib/FreeSans.ttf';
$props['traf_grid_min_step']=25;
$props['time_grid_min_step']=25;
//Create image
$myImage = imagecreatetruecolor($props['width'], $props['height']);

//Fill background:
$white = imagecolorallocate($myImage, 255, 255, 255);
imagefilledrectangle  ($myImage,0,0,$props['width'],$props['height'],$white);

//define colors:
$textcolor = imagecolorallocate($myImage,0,0,0);
$gridcolor1 = imagecolorallocate($myImage,0,0,0);
$gridcolor2 = imagecolorallocate($myImage,200,200,200);
$in_color = imagecolorallocatealpha($myImage,0,255,0,30);
$out_color = imagecolorallocatealpha($myImage,255,0,0,30);



//Autoscale X:
if ($props['sid']){
	$query='select min(UNIX_TIMESTAMP(stat_time)),max(UNIX_TIMESTAMP(stat_time)) from session_statistics where session_id='.$props['sid'];
	if ($props['gid']) $query.= ' and zone_group_id='.$props['gid'];
	if ($debug)echo $query."<br>";
	$res=$mysqli->query($query);
	if ($res){
		if ($l=$res->fetch_row()){
			$props['start']=(int)$l[0];
			$props['end']=(int)$l[1];
		}else die();
	}else die();
};

//Here we got both start and end time, calculate bar width, distance between bars, dt in each bar
$props['dt']=(int)round(($props['end']-$props['start'])/$props['graphwidth'])+1;
$props['dx']=1/$props['dt'];
if ($props['dt']<60){//If we got less than minute per pixel, force to draw wide bars, minute per bar
	$props['dt']=60;
	//$props['dx']=1;
	$props['dx']*=60;
	$props['barwidth']=$props['dx'];
}else{//If we got less than minute per pixel, draw pixel-width bars
	$props['dx']=1;
	$props['barwidth']=0;
};

//Autoscale Y:
if ($props['sid']){
	$query='SELECT max(sum_in),max(sum_out) FROM (SELECT sum(traf_in) AS sum_in,sum(traf_out) AS sum_out from session_statistics where session_id='.$props['sid'];
	if ($props['gid']) $query.= ' and zone_group_id='.$props['gid'];
	$query.=' group by UNIX_TIMESTAMP(stat_time) div '.$props['dt'].') AS sums';
}else if ($props['uid']){
	if ($props['uid']>0)
	{
			$query='SELECT max(sum_in),max(sum_out) FROM (SELECT sum(session_statistics.traf_in) AS sum_in,sum(session_statistics.traf_out) AS sum_out from session_statistics,sessions where session_statistics.session_id=sessions.id and sessions.user_id='.$props['uid'].' and unix_timestamp(session_statistics.stat_time)>= '.$props['start'].' and unix_timestamp(session_statistics.stat_time)<='.$props['end'];
	}else{
		$query='SELECT max(sum_in),max(sum_out) FROM (SELECT sum(session_statistics.traf_in) AS sum_in,sum(session_statistics.traf_out) AS sum_out from session_statistics where session_statistics.stat_time>= from_unixtime('.$props['start'].') and session_statistics.stat_time<= from_unixtime('.$props['end'].')';
	};
			
	if ($props['gid']) $query.= ' and session_statistics.zone_group_id='.$props['gid'];
	$query.=' group by UNIX_TIMESTAMP(session_statistics.stat_time) div '.$props['dt'];
	//if ($props['uid']>0) 
		$query.=') AS sums';
};
if ($debug)echo $query."<br>";

$res=$mysqli->query($query);
if ($res){
	if ($l=$res->fetch_row()){
//$props['dx']=1/$props['dt'];
		$props['max_in']=(int)$l[0];
		$props['max_out']=(int)$l[1];
		$props['max_traf']=max($props['max_in'],$props['max_out']);
	}else die();
}else die();

//Check if we got some data to display:
if ($props['max_traf']<=$props['dy']) die();


/*
* VERTICAL (BANDWIDTH) SCALE
*/
//Get units:
$props['units_mul']=0;
while ($props['max_traf']>$props['dt']*pow(1024,$props['units_mul']+1) ) $props['units_mul']++;
//Normalise graph to next round value:
//$props['max_traf']=10*pow(1024,$props['units_mul'])*(ceil(($props['max_traf']/pow(1024,$props['units_mul']))*0.1));
$props['dy']=$props['graphheight']/$props['max_traf'];

//Get grid dy:
$props['grid_dy']=$props['max_traf'];
while (($props['y_zero']-getgraphy($props,$props['grid_dy']))>=$props['traf_grid_min_step']*2) $props['grid_dy']/=2;
$traf=0;

//Draw horizontal grid lines:
while ($traf<=$props['max_traf']){
	$y=getgraphy($props,$traf);
	$text=round($traf/($props['dt']*pow(1024,$props['units_mul'])),2);
	$text.=$lang['traf_units'][$props['units_mul']];
	$bbox=imagettfbbox($props['grid_font_size'],0,$props['grid_font'],$text);
	$textx=$props['x_zero']-$bbox[2] + $bbox[0];
	$texty=$y-($bbox[5]-$bbox[3])/2;
	imageLine($myImage,$props['x_zero'],$y,$props['x_zero']+$props['graphwidth'],$y,$gridcolor2);
	imagettftext($myImage,$props['grid_font_size'],0,$textx,$texty,$textcolor,$props['grid_font'],$text);
	$traf+=$props['grid_dy'];
}

//Print out vertical units:
imagettftext($myImage,10,90,10,$props['height']/2,$textcolor,'lib/FreeSans.ttf',$lang['units'][$props['units_mul']].'bytes/s');

/*
* HORIZONTAL (TIME) SCALE
*/
//Get units:
		$props['time_index']=0;
		if ($debug) echo 'gms*dt:'.$props['dt']*$props['time_grid_min_step'].' and ts'.$lang['time_seconds'][$props['time_index']].'<br>';
	if ($props['dt']>60){
		while (($props['dt']*$props['time_grid_min_step']>$lang['time_seconds'][$props['time_index']])||($props['time_index']>5))
		{
			$props['time_index']++;
			if ($debug) echo  'ti:'.$props['time_index'].' '.$lang['time_seconds'][$props['time_index']].'<br>';
		}
		$props['time_step']=$lang['time_seconds'][$props['time_index']];
	}else{
		$props['time_step']=3600;
		while (abs(getgraphx($props,$props['start'])-getgraphx($props,$props['start']+$props['time_step']))>=$props['time_grid_min_step']*2) $props['time_step']/=2;
		$props['time_index']=1;
	};
	$props['timeparts']=array(idate('s',$props['start']),
														idate('i',$props['start']),
														idate('H',$props['start']),
														idate('d',$props['start']),
														idate('m',$props['start']),
														idate('Y',$props['start']),
													 );
	for ($i=0;$i<$props['time_index'];$i++){
		$props['timeparts'][$i]=0;
	};
	$props['time_startgrid']=mktime($props['timeparts'][2],$props['timeparts'][1],$props['timeparts'][0],$props['timeparts'][4],$props['timeparts'][3],$props['timeparts'][5]);
	//Print out horizontal units:
	imagettftext($myImage,10,0,$props['width']/2,$props['height']-10,$textcolor,'lib/FreeSans.ttf',$lang['time_units_long'][$props['time_index']]);
	if ($debug) echo $props['time_index'].' time:'.$props['time_startgrid'].'<br>';
	if ($props['time_startgrid']< $props['start']){
		$time=$props['time_startgrid']+$props['time_step'];
	}else{
		$time=$props['time_startgrid'];
	};
	//Draw vertical grid lines:
	while ($time<=$props['end']){
		$x=getgraphx($props,$time);
		if ($props['time_index']>2){//Print date:
			$text=date("d M Y",$time);
			$text2=date("H:i:s",$time);
		}else {//Print time:
			$text=date("H:i:s",$time);
		};
		$bbox=imagettfbbox($props['grid_font_size'],315,$props['grid_font'],$text);
		$textx=$x-($bbox[2] + $bbox[0])/2;
		$texty=$props['y_zero']+($bbox[3]-$bbox[5]);
		imageLine($myImage,$x,$props['y_zero'],$x,$props['y_zero']-$props['graphheight'],$gridcolor2);
		imagettftext($myImage,$props['grid_font_size'],315,$textx,$texty,$textcolor,$props['grid_font'],$text);
		imagettftext($myImage,$props['grid_font_size'],315,$textx-5,$texty+15,$textcolor,$props['grid_font'],$text2);
		$time+=$props['time_step'];
		if ($debug) echo $text.' time:'.$time.'<br>';
	};




if ($props['sid']){
	$query='select sum(traf_in),sum(traf_out),zone_group_id,UNIX_TIMESTAMP(stat_time) div '.$props['dt'].' from session_statistics where session_id='.$props['sid'];
}else if ($props['uid']){
	if ($props['uid']>0){
		$query='select sum(session_statistics.traf_in),sum(session_statistics.traf_out),session_statistics.zone_group_id,UNIX_TIMESTAMP(session_statistics.stat_time) div '.$props['dt'].' from session_statistics,sessions where session_statistics.session_id=sessions.id and sessions.user_id='.$props['uid'].' and unix_timestamp(session_statistics.stat_time)>='.$props['start'].' and unix_timestamp(session_statistics.stat_time)<='.$props['end'];
	}else{
		$query='select sum(session_statistics.traf_in),sum(session_statistics.traf_out),session_statistics.zone_group_id,UNIX_TIMESTAMP(session_statistics.stat_time) div '.$props['dt'].' from session_statistics where session_statistics.stat_time >= FROM_UNIXTIME('.$props['start'].') and session_statistics.stat_time <= FROM_UNIXTIME('.$props['end'].')';
	};
};
if ($props['gid']) $query.= ' and session_statistics.zone_group_id='.$props['gid'];
$query.=' group by UNIX_TIMESTAMP(session_statistics.stat_time) div '.$props['dt'];
if ($debug)echo $query."<br>";
$res=$mysqli->query($query);
$xprev=$props['x_zero'];
if ($res){
	while($l=$res->fetch_row()){
		if ($debug) echo getgraphy($props,$l[0]).' -- '.getgraphy($props,$l[1]).' -- '.getgraphx($props,$l[3]*$props['dt']).' -- '.(getgraphx($props,$l[3]*$props['dt'])+$props['dx'])."<br>";
		$x1=getgraphx($props,$l[3]*$props['dt']);
		$x2=max($xprev+1,$x1);
		$x1+=$props['barwidth'];
		$xprev=$x1;
		if (!$debug) imagefilledrectangle($myImage,$x1,$props['y_zero'],$x2,getgraphy($props,$l[0]),$in_color);
		if (!$debug) imagefilledrectangle($myImage,$x1,$props['y_zero'],$x2,getgraphy($props,$l[1]),$out_color);
		//imageLine($myImage,$l[3]-$start_time,480,$l[3]-$start_time,480-round(($l[0]/$maxtrafin)*480),$in_color);
		//imageLine($myImage,$l[3]-$start_time,480,$l[3]-$start_time,480-round(($l[1]/$maxtrafout)*480),$out_color);
		//$xprev=getgraphx($props,$l[3]*$props['dt'])+1;
	};
}else die();
if ($debug)print_r(var_dump($props));
if (!$debug)imagepng($myImage);
imagedestroy($myImage);
?>
