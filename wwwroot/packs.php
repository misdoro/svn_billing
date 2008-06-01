<?
require ('auth.php');

$result='';

//Activate requested pack, if possible
if (isset($_POST['user']) && ($_POST['user'] == $_SESSION['bill_id']|| $_SESSION['is_admin'])){
	//Check user data:
	$uid=(int)($_POST['user']);
	//if ($uid!=$_SESSION[bill_id])
	$query='SELECT id,debit+credit FROM users WHERE id='.$uid.'';
	//echo $query;
	$res=$mysqli->query($query);
	$l=$res->fetch_row();
	$id=(int)$l[0];
	$money=(int)$l[1];
	//Get pack data
	$pack_id=(int)$_POST['pack_name'];
	$query='SELECT id,unitcount,unit_zone,on_price,duration,durationtype,bonuspack FROM packs WHERE id='.$pack_id;
	//echo $query;
	$res=$mysqli->query($query);
	$pack = $res->fetch_array(MYSQLI_ASSOC);
	//Get bonus pack data:
	if ($pack['bonuspack']){
		$query='SELECT id,unitcount,unit_zone,on_price,duration,durationtype,bonuspack FROM packs WHERE id='.$pack['bonuspack'];
		//echo $query;
		$res=$mysqli->query($query);
		$bpack = $res->fetch_array(MYSQLI_ASSOC);
	};
	//check if user can use this pack
	if ($money < $pack['on_price']){
		echo $money.'и'.$pack['on_price'];
		$result='<p color=red>Недостаточно средств для активации пакета';
	};
	
	
	//If no errors occured, activate packs:
	if ($result==''){
		//Construct expiration date:
		if ($pack['durationtype']==4){
			$expires= mktime(0, 0, 0, date("m")+$pack['duration'], 1, date("Y"));
		};
		if ($bpack['durationtype']==4){
			$bexpires= mktime(0, 0, 0, date("m")+$bpack['duration'], 1, date("Y"));
		};
		$query='INSERT into userpacks (user_id, pack_id, units_left, date_expire, unit_zone, unittype) values ( '.$uid.','.$pack['id'].',1048576*'.$pack['unitcount'].',FROM_UNIXTIME('.$expires.'),'.$pack['unit_zone'].',1);';
		logquery($query,$mysqli);
		if ($pack['bonuspack']){
			$query='INSERT into userpacks (user_id, pack_id, units_left, date_expire, unit_zone, unittype) values ( '.$uid.','.$bpack['id'].',1048576*'.$bpack['unitcount'].',FROM_UNIXTIME('.$bexpires.'),'.$bpack['unit_zone'].',1);';
			logquery($query,$mysqli);
		};
		$query='UPDATE users SET debit=debit-'.$pack['on_price'].' WHERE id='.$uid;
		logquery($query,$mysqli);
		//echo $query;
	};
	
}else if(isset($_GET['user']) && $_SESSION['is_admin']){
	$id=(int)$_GET['user'];
}else
{
	$id=$_SESSION['bill_id'];
};


//Build user packs table:
		$query='SELECT userpacks.pack_id, userpacks.date_on, userpacks.units_left/1048576, userpacks.date_expire, packs.name, groupnames.caption FROM userpacks, packs, groupnames WHERE groupnames.id=userpacks.unit_zone AND userpacks.unittype=1 AND userpacks.pack_id=packs.id AND userpacks.user_id='.$id.' AND userpacks.date_expire>CURRENT_TIMESTAMP order by userpacks.date_on';
//echo $query;
$res=$mysqli->query($query);
$packstable='';
if ($res){
	$packstable='
	<table>
	<tr>
		<th>
			Пакет
		</th>
		<th>
			Дата подключения	
		</th>
			<th>
			Зона
			</th>
			<th>
				Остаток, мб
			</th>
			<th>
				Дата окончания
			</th>
		</tr>
	';
	while ($l=$res->fetch_row()){
		$packstable.='<tr><td>'.$l[4].'</td><td>'.$l[1].'</td><td>'.$l[5].'</td><td>'.$l[2].'</td><td>'.$l[3].'</td></tr>';
	};
};
$packstable.='</table>';


//Build users list for pack activation if admin
if ($_SESSION['is_admin']){
	$users='<select name="user" id="user">
			<option value="Выберите пользователя:">Выберите пользователя:';
	$query='select id,login,debit+credit from users where parent is NULL;';
	$res=$mysqli->query($query);
	while ($l=$res->fetch_row()){
		$users.='<option value="'.$l[0].'" >'.$l[1].' ( доступно '.$l[2].' руб)';
	};
	$users.='</SELECT>';
}else{
	$users=$_SESSION['username'].'<INPUT type="hidden" name="user" id="user" value="'.$_SESSION['bill_id'].'">';
	
};

//Build available packs list:
	if ($_SESSION['is_admin']) $query='SELECT packs.id,packs.name,packs.on_price from packs';
	else $query='SELECT packs.id,packs.name,packs.on_price from users,packs WHERE packs.on_price < (users.debit + users.credit) AND users.id = '.$_SESSION['bill_id'];
//echo $query;
$res=$mysqli->query($query);
$packlist='
		<select name="pack_name" id="pack_name">
		<option value="0">Выберите пакет:
		';
while ($l=$res->fetch_row())	$packlist.= '<option value="'.$l[0].'" >'.$l[1].' ( подключение '.$l[2].' рублей ) ';
$packlist.= '</SELECT>';



?>
<html>
		<head>
			<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">
			<title>Управление пакетами: <?=$_SESSION['fullname'];?></title>
		</head>
		<body>
				<p>Подключенные пакеты:
				<br><?=$packstable;?>
				<br>
				<form action="packs.php" method="POST">
				<p>Подключить пакет:
				<br><?=$packlist;?>
				<br>для пользователя <?=$users;?>
				<br><input type="submit" value="Подключить!">
				</form>
				<?=$result;?>
		</body>
</html>