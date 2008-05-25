<?
require ('auth.php');

if (isset($_POST['username'])){
	$user=filterit($_POST['username']);
	$query='SELECT id,debit+credit,login FROM users WHERE login="'.$user.'" limit 1';
	$res=$mysqli->query($query);
	$l=$res->fetch_row();
	$id=$l[0];
	$debit=$l[1];
	$uname=$l[2];
	echo $id.$debit.$uname;
}else 
{
	$id=$_SESSION['user_id'];
};


//Build user packs table:
		$query='SELECT userpacks.pack_id, userpacks.date_on, userpacks.units_left/1048576, userpacks.date_expire, packs.name, groupnames.caption FROM userpacks, packs, groupnames WHERE groupnames.id=userpacks.unit_zone AND userpacks.unittype=1 AND userpacks.pack_id=packs.id AND userpacks.user_id='.$id.' order by userpacks.date_on';
//echo $query;
$res=$mysqli->query($query);
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
$packstable.='</table>';

?>
<html>
		<head>
			<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">
			<title>Управление пакетами: <?=$_SESSION['fullname'];?></title>
		</head>
		<body>
				<p>Подключенные пакеты:
				<br><?=$packstable;?>
		</body>
</html>