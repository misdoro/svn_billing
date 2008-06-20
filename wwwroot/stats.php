<?
require ('auth.php');
require ('mysql.php');


//select sum(session_statistics.traf_in), sum(session_statistics.traf_out), sum(session_statistics.traf_in_money), groupnames.caption from session_statistics,sessions,groupnames where session_statistics.stat_time > '2008-05-01 00:00:00' and session_statistics.stat_time < '2008-06-01 00:00:00' and session_statistics.session_id=sessions.id and sessions.user_id=18 and session_statistics.zone_group_id=groupnames.id group by session_statistics.zone_group_id;

//Init session vars if necessary:
//if (!isset($_SESSION['stat_month'])) $_SESSION['stat_month']=date("m");
//if (!isset($_SESSION['stat_year'])) $_SESSION['stat_year']=date("Y");
if (!isset($_SESSION['stat_mode'])) $_SESSION['stat_mode']=0;
//stat start time and stat end time:
if (!isset($_SESSION['stat_start']) || !isset ($_SESSION['stat_end'])){
	$_SESSION['stat_start']=mktime(0, 0, 0, date("m"), 1, date("Y"));
	$_SESSION['stat_end']=mktime(0, 0, 0, date("m")+1, 1, date("Y"));
};
if (!isset($_SESSION['stat_uid'])) $_SESSION['stat_mode']=0;

//Update vars:
//if (isset($_GET['stat_year'])) $_SESSION['stat_year']=(int)$_GET['stat_year'];
//if (isset($_GET['stat_month'])) $_SESSION['stat_month']=(int)$_GET['stat_month'];
if (isset($_GET['stat_mode'])) $_SESSION['stat_mode']=(int)$_GET['stat_mode'];
if (isset($_POST['groupby_s'])) $_SESSION['stat_mode']=(int)$_POST['groupby_s'];
if (isset($_POST['startdate'])) $_SESSION['stat_start']=strtotime($_POST['startdate']);
if (isset($_POST['enddate'])) $_SESSION['stat_end']=strtotime($_POST['enddate']);

//Stat uid handling, with is_admin check:
if (!isset($_SESSION['stat_uid'])) $_SESSION['stat_uid']=$_SESSION['user_id'];
if ($_SESSION['is_admin'] && isset($_POST['user'])) $_SESSION['stat_uid']=(int)$_POST['user'];
if ($_SESSION['is_admin'] && isset($_GET['user'])) $_SESSION['stat_uid']=(int)$_GET['user'];
$stat_uid=$_SESSION['stat_uid'];

$stmode=(int)$_SESSION['stat_mode'];
$stat_start=(int)$_SESSION['stat_start'];
$stat_end=(int)$_SESSION['stat_end'];


$start_date=strftime("%d.%m.%Y",$stat_start);
$end_date=strftime("%d.%m.%Y",$stat_end);

//Month / year selector:
$period_s='<form action="stats.php" method="POST">';
$period_s.='С';
$period_s.='<input type="text" maxlength="10" size="10" name="startdate" value="'.$start_date.'">';
$period_s.='до';
$period_s.='<input type="text" maxlength="10" size="10" name="enddate" value="'.$end_date.'">';
$period_s.='<input type="Submit" value="Обновить">
</form>
';

//Get stats table:
$stats_table='';
if ($stmode ==0){
	$query="SELECT SUM(session_statistics.traf_in), SUM(session_statistics.traf_out), SUM(session_statistics.traf_in_money), groupnames.caption FROM session_statistics,sessions,groupnames WHERE UNIX_TIMESTAMP(session_statistics.stat_time) > '$stat_start' AND UNIX_TIMESTAMP(session_statistics.stat_time) < '$stat_end' AND session_statistics.session_id=sessions.id AND sessions.user_id=$stat_uid AND session_statistics.zone_group_id=groupnames.id GROUP BY session_statistics.zone_group_id;";
	//echo $query;
	$res=$mysqli->query($query);
	if ($res){
		$stats_table='
				<table>
				<tr>
				<th>
				входящий, байт
				</th>
				<th>
				исходящий, байт
				</th>
				<th>
				Направление
				</th>
				<th>
				Снято со счета
				</th>
				</tr>
				';
		while ($l=$res->fetch_row()){
			$stats_table.='<tr><td>'.$l[0].'</td><td>'.$l[1].'</td><td>'.$l[3].'</td><td>'.$l[2].'</td></tr>';
		};
	};
	$stats_table.='</table>';
}else if ($stmode == 1){
	$query="SELECT (sum(session_statistics.traf_in) div 104.8576)/10000, (sum(session_statistics.traf_out) div 104.8576)/10000, sum(session_statistics.traf_in_money), min(session_statistics.stat_time),max(session_statistics.stat_time), groupnames.caption FROM session_statistics,sessions,groupnames WHERE unix_timestamp(session_statistics.stat_time) > '$stat_start' AND unix_timestamp(session_statistics.stat_time) < '$stat_end' AND session_statistics.session_id=sessions.id AND sessions.user_id=$stat_uid AND session_statistics.zone_group_id=groupnames.id GROUP BY (unix_timestamp(session_statistics.stat_time)-$stat_start) div 86400, session_statistics.zone_group_id;";
	//echo $query;
	$res=$mysqli->query($query);
	if ($res){
		$stats_table='
				<table>
				<tr>
				<th>
				входящий, МБ
				</th>
				<th>
				исходящий, МБ
				</th>
				<th>
				Снято со счета
				</th>
				<th>
				Направление
				</th>
				<th>
				Интервал
				</th>
				</tr>
				';
		while ($l=$res->fetch_row()){
			$stats_table.='<tr><td>'.$l[0].'</td><td>'.$l[1].'</td><td>'.$l[2].'</td><td>'.$l[5].'</td><td>'.$l[3].' - '.$l[4].'</td></tr>';
		};
	};
	$stats_table.='</table>';
}else if ($stmode == 2){
	$query="SELECT (sum(session_statistics.traf_in) div 104.8576)/10000, (sum(session_statistics.traf_out) div 104.8576)/10000, sum(session_statistics.traf_in_money), min(session_statistics.stat_time),max(session_statistics.stat_time), groupnames.caption, session_statistics.session_id FROM session_statistics,sessions,groupnames WHERE unix_timestamp(session_statistics.stat_time) > '$stat_start' AND unix_timestamp(session_statistics.stat_time) < '$stat_end' AND session_statistics.session_id=sessions.id AND sessions.user_id=$stat_uid AND session_statistics.zone_group_id=groupnames.id GROUP BY session_statistics.session_id, session_statistics.zone_group_id;";
	//echo $query;
	$res=$mysqli->query($query);
	if ($res){
		$stats_table='
				<table text-align>
				<tr>
				<th>
				входящий, МБ
				</th>
				<th>
				исходящий, МБ
				</th>
				<th>
				Снято со счета
				</th>
				<th>
				Направление
				</th>
				<th>
				Интервал
				</th>
				<th>
				идентификатор сессии
				</th>
				</tr>
				';
		while ($l=$res->fetch_row()){
			$stats_table.='<tr><td>'.$l[0].'</td><td>'.$l[1].'</td><td>'.$l[2].'</td><td>'.$l[5].'</td><td>'.$l[3].' - '.$l[4].'</td><td>'.$l[6].'</td></tr>';
		};
	};
	$stats_table.='</table>';
};

//Group by selector:
$group_opts['0']='Сводная статистика';
$group_opts['1']='Статистика по дням';
$group_opts['2']='Статистика по сессиям';
$nopts=3;
$groupby_s='
		<form action="stats.php" method="POST">
		<select name="groupby_s" id="groupby_s">
		<option value="-1">Группировка:';
		for ($i=0;$i<$nopts;$i++){
			$groupby_s.='<option value="'.$i.'" ';
			if ($stmode==$i) $groupby_s.= 'selected';
			$groupby_s.='>'.$group_opts[$i];
		};
		
		$groupby_s.='</SELECT>
		<input type="Submit" value="Обновить">
		</form>
		';

//user selector if admin, username in other cases:
if ($_SESSION['is_admin']){
	
	$user_s='<form action="stats.php" method="POST">Пользователь:<select name="user" id="user">';
	$query='select id,login from users;';
	$res=$mysqli->query($query);
	while ($l=$res->fetch_row()){
		$user_s.='<option value="'.$l[0].'"';
		if ($l[0]==$stat_uid) $user_s.= 'selected';
		$user_s.='>'.$l[1];
	};
	$user_s.='</SELECT><input type="Submit" value="Обновить"></form>';
	
}else{
	$user_s='Пользователь:'.$_SESSION['username'];
};



?>

<html><head>
	<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">
	<link rel=stylesheet href=style.css>
	<title>Статистика интернет: <?=$_SESSION['fullname'];?></title>
	</head><body>
	Здравствуйте, <?=$_SESSION['firstname'];?>!
	<p><?=$groupby_s;?>
	<br><?=$period_s;?>
	<br><?=$user_s;?>
	<br><?=$stats_table;?>
	<br><a href="index.php">На главную</a>
</body></html>