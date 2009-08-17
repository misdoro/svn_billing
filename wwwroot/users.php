<?
require ('auth.php');

if (($_SESSION['is_admin']!=true)){
	echo '<html><head>';
	echo '<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">';
	echo '</head><body>';
	echo '<h1>Вам сюда нельзя...</h1>';
	echo '</body></html>';
	die();
};


function getrow($l){
	global $lang;
	global $business_mode;
	$ret='<td>';
	if ($l[0]){
		$ret.='<input type="hidden" value="'.$l[0].'" id="uid'.$l[0].'">';
		$ret.=$l[1];
		$ret.='<input type="hidden" value="'.$l[1].'" id="login'.$l[0].'">';
		$ret.='</td><td>';
		$ret.='<input type="text" value="'.$l[2].'" id="ip'.$l[0].'" onchange="javascript:updaterow('.$l[0].')">';
		$ret.='</td><td>';
		$ret.='<input type="text" value="'.$l[3].'" id="debit'.$l[0].'" onchange="javascript:updaterow('.$l[0].')">';
		$ret.='</td><td>';
		if ($business_mode) $ret.='<input type="text" value="'.$l[5].'" id="limit'.$l[0].'" onchange="javascript:updaterow('.$l[0].')">';
		else $ret.='<input type="text" value="'.$l[4].'" id="credit'.$l[0].'" onchange="javascript:updaterow('.$l[0].')">';
		$ret.='</td><td class="td-c" >';
		$ret.='<input type="checkbox" ';if ($l[6]) $ret.='checked';$ret.=' id="active'.$l[0].'" onchange="javascript:updaterow('.$l[0].')">';
		$ret.='<td nowrap>';
		$ret.='<a href="stats.php?user='.$l[0].'">'.'<img src="img/stats.png"  border=0></img>'.'</a>';
		$ret.='<a href="sarg/thismonth/">'.'<img src="img/www.png"  border=0></img>'.'</a>';
		$ret.='<a href="javascript:askpassword('.$l[0].');">'.'<img src="img/password.png"  border=0></img>'.'</a>';
		$ret.='&nbsp;&nbsp;';
		$ret.='<a href="javascript:rmuser('.$l[0].');">'.'<img src="img/del.png" border=0></img>'.'</a>';
		$ret.='</td>';
	}else{
		$l[0]=0;
		$ret.='<input type="text" value="'.$l[1].'" id="login'.$l[0].'">';
		$ret.='</td><td>';
		$ret.='<input type="text" value="'.$l[2].'" id="ip'.$l[0].'">';
		$ret.='</td><td>';
		$ret.='<input type="text" value="'.$l[3].'" id="debit'.$l[0].'">';
		$ret.='</td><td>';
		if ($business_mode) $ret.='<input type="text" value="'.$l[5].'" id="limit'.$l[0].'">';
		else $ret.='<input type="text" value="'.$l[4].'" id="credit'.$l[0].'">';
		$ret.='</td><td class="td-c" >';
		$ret.='<input type="checkbox" checked id="active'.$l[0].'">';
		$ret.='<td nowrap>';
		$ret.='<a href="javascript:doadd();">'.'<img src="img/ok.png"  border=0></img>'.'</a>';
		$ret.='&nbsp;&nbsp;';
		$ret.='<a href="javascript:dontadd();">'.'<img src="img/del.png" border=0></img>'.'</a>';
		$ret.='</td>';
	};
	return $ret;
};

if ($_POST['ajax']){
	header("Content-type: text/html; charset=utf-8");
	if (isset($_POST['getrow'])){
		$uid=(int)$_POST['getrow'];
		if ($uid){
			$query='select id,login,INET_NTOA(user_ip),debit,credit,mlimit,active,parent from users where id='.$uid;
			$res=$mysqli->query($query);
			$l=$res->fetch_row();
			echo getrow($l);
		};
	};
	//If we update user:
	if ($_POST['action']=='update'){
		$uid=(int)$_POST['user'];
		$login=filterit($_POST['login']);
		$ip=filterit($_POST['ip']);
		$debit=(double)$_POST['debit'];
		$credit=(double)$_POST['credit'];
		$limit=(double)$_POST['mlimit'];
		if ($_POST['active']=='true') $active=1; else $active=0;
		if ($uid){
			$uquery='update users set user_ip=INET_ATON(\''.$ip.'\'),debit='.$debit.',credit='.$credit.',mlimit='.$limit.',active='.$active.' where id='.$uid.';';
			//echo $uquery;
			logquery($uquery,$mysqli);
			$query='select id,login,INET_NTOA(user_ip),debit,credit,mlimit,active,parent from users where id='.$uid;
			$res=$mysqli->query($query);
			$l=$res->fetch_row();
			echo getrow($l);
		};
	};
	//If we add user:
	if ($_POST['action']=='adduser'){
		//$uid=(int)$_POST['user'];
		$login=filterit($_POST['login']);
		$ip=filterit($_POST['ip']);
		$debit=(double)$_POST['debit'];
		$credit=(double)$_POST['credit'];
		$limit=(double)$_POST['mlimit'];
		if ($_POST['active']=='true') $active=1; else $active=0;
		if ($login){
			$uquery='INSERT INTO users (login,user_ip,debit,credit,mlimit,active) values (\''.$login.'\',INET_ATON(\''.$ip.'\'),'.$debit.','.$credit.','.$limit.','.$active.');';
			//echo $uquery;
			logquery($uquery,$mysqli);
			//sleep(1);
			$query='select id,login,INET_NTOA(user_ip),debit,credit,mlimit,active,parent from users where login=\''.$login.'\'';
			//echo $query;
			$res=$mysqli->query($query);
			$l=$res->fetch_row();
			echo getrow($l);
			$query='INSERT INTO usergroups (user_id,group_id) SELECT '.$l[0].',id FROM groupnames;';
			logquery($query,$mysqli);
		};
	};
	
	//If we delete user:
	if ($_POST['action']=='delete'){
		$uid=(int)$_POST['user'];
		if ($uid){
			$query='delete from users where id='.$uid;
			logquery($query,$mysqli);
		};
	};
	//If we change password:
	if ($_POST['action']=='password'){
		$uid=(int)$_POST['user'];
		$password=filterit($_POST['password']);
		if ($uid){
			$query='update users set password=\''.$password.'\' where id='.$uid.';';
			logquery($query,$mysqli);
			$query='select id,login,INET_NTOA(user_ip),debit,credit,mlimit,active,parent from users where id='.$uid;
			$res=$mysqli->query($query);
			$l=$res->fetch_row();
			echo getrow($l);
		};
	};
	//If we add new user:
	if ($_POST['action']=='getemptyrow'){
		echo getrow(array());
	};
	
	die();
};

//create table header
		if ($business_mode) $users_table='<table id="userstable"><thead><tr><th>Логин</th><th>IP</th><th>Остаток, МБ</th><th>месячный лимит, МБ</th><th>активен</th><th><a href="javascript:adduser(0);">'.'<img src="img/add.png"  border=0></img>'.'</a></th></tr></thead><tbody>';
else $users_table='<table id="userstable"><thead><tr><th>Логин</th><th>IP</th><th>Баланс, руб.</th><th>кредит, руб.</th><th>активен</th><th><a href="javascript:adduser(0);">'.'<img src="img/add.png"  border=0></img>'.'</a></th></tr></thead><tbody>';

//Append users:
$query='select id,login,INET_NTOA(user_ip),debit,credit,mlimit,active,parent from users order by login asc;';
$res=$mysqli->query($query);
while ($l=$res->fetch_row()){
	$users_table.='<tr id="row'.$l[0].'">'.getrow($l).'</tr>';
};
if ($business_mode) $users_table.='</tbody><tfoot><tr><th>Логин</th><th>IP</th><th>Остаток, МБ</th><th>месячный лимит, МБ</th><th>активен</th><th><a href="javascript:adduser();">'.'<img src="img/add.png"  border=0></img>'.'</a></th></tr></tfoot></table>';
else $users_table='</tbody><tfoot><tr><th>Логин</th><th>IP</th><th>Баланс, руб.</th><th>кредит, руб.</th><th>активен</th><th><a href="javascript:adduser(1);">'.'<img src="img/add.png"  border=0></img>'.'</a></th></tr></tfoot></table>';
?>

<html><head>
	<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">
	<link rel=stylesheet href=style.css>
	<title>Управление пользователями</title>
	<script src="js/ajax.js" language="JavaScript" type="text/javascript"></script>
	<script src="js/users.js" language="JavaScript" type="text/javascript"></script>
</head><body>
	<script language='JavaScript' type='text/javascript'>
		var rmconfirm="Действительно удалить пользователя ";
		var passwordprompt="Введите новый пароль для пользователя ";
		var passwordprompt2="Повторите новый пароль для пользователя ";
		var addnew=0;
	</script>
	Здравствуйте, <?=$_SESSION['firstname'];?>!
	<p><?=$users_table;?></p>
	<br><a href="index.php">На главную</a>
</body></html>
