<?
require ('auth.php');

check_admin();

//Syncing to LDAP directory,
//select all usernames from tree,
//Try to add each to database, if succeeded, add 
if ($_GET['sync_ldap'] && $auth_mode=='ldap'){
	//select all usernames from ldap tree,
	$ds=ldap_connect($ldap_uri);
	if ($ds){
		if (ldap_set_option($ds, LDAP_OPT_PROTOCOL_VERSION, 3)) {
					//echo "Using LDAPv3";
		} else {
						echo "Failed to set LDAP protocol version to 3";
		}
		$bind=ldap_bind($ds,$ldap_binddn,$ldap_bindpass);
		if ($bind){
			$ldap_fields=array("uid");
			$filter='(uid=*)';
			//echo $filter."<br>";
			$sr = ldap_search($ds, $ldap_searchbase, $filter,$ldap_fields);
			for($i=ldap_first_entry($ds,$sr);$i!=false;$i=ldap_next_entry($ds,$i))
			{
				//echo var_dump(ldap_get_values($ds,$i,'uid'));
				$array=ldap_get_values($ds,$i,'uid');
				$login=$array[0];
				//echo $login;
				//Try to insert new entries, only ones with non-existent login will succeed.
				//$query='SELECT id,parent FROM users WHERE login="'..'"';
				$query='insert into users (login,password,user_ip,active) select \''.$login.'\',\'\',0,0 from users where login=\''.$login.'\' having count(id)=0;';
				$mysqli->query($query);
				if ($userid=$mysqli->insert_id){
					$query='insert into usergroups (user_id,group_id) select id,'.$userid.' from groupnames';
					$mysqli->query($query);
				};
			};
		}else {
			echo "Failed to bind LDAP!";
		};
	}else{
		echo "Failed to connect to LDAP server!";
	};
};


function getrow($l){
	global $lang;
	global $business_mode;
	global $auth_mode;
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
		if ($auth_mode=='mysql')
			$ret.='<a href="javascript:askpassword('.$l[0].');">'.'<img src="img/password.png"  border=0></img>'.'</a>';
		$ret.='<a href="javascript:modgroups('.$l[0].');">'.'<img src="img/zgroups.png"  border=0></img>'.'</a>';
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
	//If we only need to refresh row
	if ($_POST['action']=='getrow' || $_POST['action']=='savegroups'){
		$uid=(int)$_POST['user'];
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
	//If we modify users's groups:
	if ($_POST['action']=='modgroups'){
		$uid=(int)$_POST['user'];
		$query='SELECT id,login FROM users WHERE id='.$uid;
		$res=$mysqli->query($query);
		$l=$res->fetch_row();
		$query='SELECT groupnames.id,groupnames.caption,usergroups.user_id FROM groupnames LEFT JOIN usergroups ON groupnames.id=usergroups.group_id AND usergroups.user_id='.$uid.' ORDER BY groupnames.caption';
		$res1=$mysqli->query($query);
		//Build custom table row:
		$ret='<td>';
		$ret.=$l[1];
		$ret.='</td><td colspan=2>';
		$ret.='Группы траффика: <SELECT	id="groupsel"	SIZE=5	MULTIPLE>';
		while ($gr=$res1->fetch_row()){
	      		$ret.='<OPTION VALUE=\''.$gr[0].'\' ';
			if ($gr[2]) $ret.='SELECTED';
			$ret.=' >'.$gr[1];
		};
		$ret.='</SELECT>';
		$ret.='</td><td colspan=2>';
		
		
		$ret.='</td><td nowrap>';
		$ret.='<a href="javascript:savegroups('.$l[0].');">'.'<img src="img/ok.png"  border=0></img>'.'</a>';
		$ret.='&nbsp;&nbsp;';
		$ret.='<a href="javascript:reloadrow('.$l[0].');">'.'<img src="img/del.png" border=0></img>'.'</a>';
		$ret.='</td>';
		echo $ret;	
	};
	//If we are saving groups:
	if ($_POST['action']=='savegroups'){
		$uid=(int)$_POST['user'];
		$count=(int)$_POST['count'];
		$query='DELETE FROM usergroups WHERE user_id='.$uid;
		$mysqli->query($query);
		for ($i=1;$i<=$count;$i++){
			if ($gid=(int)$_POST['gr'.$i]){
				$query='INSERT INTO usergroups (user_id,group_id) values('.$uid.','.$gid.');';
				$mysqli->query($query);
			}
		};
		
	};
	die();
};
//create table header
		if ($business_mode) $users_table='<table id="userstable"><thead><tr><th>Логин</th><th>IP</th><th>Остаток, МБ</th><th>месячный лимит, МБ</th><th>активен</th><th><a href="javascript:adduser(0);">'.'<img src="img/add.png"  border=0></img>'.'</a>';
else $users_table='<table id="userstable"><thead><tr><th>Логин</th><th>IP</th><th>Баланс, руб.</th><th>кредит, руб.</th><th>активен</th><th><a href="javascript:adduser(0);">'.'<img src="img/add.png"  border=0></img>'.'</a>';
//If using LDAP integration, add button to import users from LDAP directory. 
if ($auth_mode=='ldap'){
	$users_table.='<a href="users.php?sync_ldap=1">'.'<img src="img/load.png"  border=0></img>'.'</a>';
};
$users_table.='</th></tr></thead><tbody>';

//Append users:
$query='select id,login,INET_NTOA(user_ip),debit,credit,mlimit,active,parent from users order by user_ip asc;';
$res=$mysqli->query($query);
while ($l=$res->fetch_row()){
	$users_table.='<tr id="row'.$l[0].'">'.getrow($l).'</tr>';
};
if ($business_mode) {
	$users_table.='</tbody><tfoot><tr><th>Логин</th><th>IP</th><th>Остаток, МБ</th><th>месячный лимит, МБ</th><th>активен</th><th><a href="javascript:adduser();">'.'<img src="img/add.png"  border=0></img>'.'</a>';
}else{
	$users_table.='</tbody><tfoot><tr><th>Логин</th><th>IP</th><th>Баланс, руб.</th><th>кредит, руб.</th><th>активен</th><th><a href="javascript:adduser(1);">'.'<img src="img/add.png"  border=0></img>'.'</a>';
};
//If using LDAP integration, add button to import users from LDAP directory. 
if ($auth_mode=='ldap'){
	$users_table.='<a href="users.php?sync_ldap=1">'.'<img src="img/load.png"  border=0></img>'.'</a>';
};
$users_table.='</th></tr></tfoot></table>';
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
