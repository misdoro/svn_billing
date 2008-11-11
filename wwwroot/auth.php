<?
//
// Authentication handler
//
if (!isset($config_set)){
	require('config.php');
};

if (!isset($mysqli)){
	require('mysql.php');
};

if (!isset($language)){
	require('lang.php');
};

if (isset($_GET['logout'])){
	session_destroy();
	header('Location: auth.php');
	exit;
};
//If we are calling ourself:
if (strstr($_SERVER['PHP_SELF'],'auth.php')){
	sleep(1);
	if (isset($_POST['username']) && isset($_POST['password'])){
		$_SESSION['username']=filterit($_POST['username']);
		$_SESSION['password']=filterit($_POST['password']);
		$_SESSION['is_admin']=false;
		$_SESSION['is_cash_admin']=false;
		$_SESSION['user_id']=0;
		$_SESSION['bill_id']=0;
		header('Location: index.php');
		exit;
	}else {
		echo '<html><head>';
		echo '<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">';
		echo '<title>'.$lang['auth'].'</title>';
		echo '</head><body>';
		echo '<form action="auth.php" method="POST">';
		echo '<input name="username" size="20"><br>';
		echo '<input name="password" type="password" size="20"><br>';
		echo '<input Value="Вход" type="submit">';
		echo '</form>';
		echo '';
		echo '</body></html>';
	}
}else{
	$authok=false;
	if (isset($_SESSION['username']) && $_SESSION['username'] && isset($_SESSION['password']) && $_SESSION['password']){
		$auth_res=array();
		
		switch ($auth_mode) {
		case "ldap"://If we use LDAP to auth users:
			$ds=ldap_connect($ldap_uri);
			if ($ds){
				if (ldap_set_option($ds, LDAP_OPT_PROTOCOL_VERSION, 3)) {
					//echo "Using LDAPv3";
				} else {
					echo "Failed to set protocol version to 3";
				}
				$bind=ldap_bind($ds,$ldap_binddn,$ldap_bindpass);
				if ($bind){
					$ldap_fields=array("cn","uid","userPassword","sn","givenName");
					$filter='(uid='.$_SESSION['username'].')';
					//echo $filter."<br>";
					$sr = ldap_search($ds, $ldap_searchbase, $filter,$ldap_fields);
					$fields = ldap_get_entries($ds, $sr);
					$auth_res['real_user']=$fields[0]["uid"][0];
					$auth_res['real_password']=$fields[0]['userpassword'][0];
					$auth_res['fullname']=$fields[0]["cn"][0];
					$auth_res['firstname']=$fields[0]["givenname"][0];
					$auth_res['lastname']=$fields[0]["sn"][0];
					//get numeric ID from MySQL
					$query='SELECT id,parent FROM users WHERE login="'.$_SESSION['username'].'"';
					$res=$mysqli->query($query);
					$l=$res->fetch_row();
					$auth_res['user_id']=(int)$l[0];
					if ($l[1]>0) $auth_res['bill_id']=(int)$l[1];
					else $auth_res['bill_id']=(int)$l[0];
				}else {
					echo "Failed to bind!";
				};
			}else{
				echo "Failed to connect!";
			};
			break;
		case "mysql"://If we gat password from the same table as debit:
			$query='select id,login,password,parent FROM users WHERE login="'.$_SESSION['username'].'"';
			$res=$mysqli->query($query);
			$l=$res->fetch_row();
			$auth_res['user_id']=$l[0];
			$auth_res['real_user']=$l[1];
			$auth_res['real_password']=$l[2];
			if ($l[3]>0) $auth_res['bill_id']=(int)$l[3];
			else $auth_res['bill_id']=(int)$l[0];
			$auth_res['fullname']=$l[1];
			$auth_res['firstname']=$l[1];
			$auth_res['lastname']='';
			$res->close();
			break;
		};
		if (($auth_res['real_user'] == $_SESSION['username']) && ($auth_res['real_password'] == $_SESSION['password'])){
			$authok=true;
			$_SESSION['fullname']=$auth_res['fullname'];
			$_SESSION['firstname']=$auth_res['firstname'];
			$_SESSION['lastname']=$auth_res['lastname'];
			$_SESSION['user_id']=$auth_res['user_id'];
			$_SESSION['bill_id']=$auth_res['bill_id'];
			if (in_array($_SESSION['username'],$admin_users)){
				$_SESSION['is_admin']=true;
			};
			if (in_array($_SESSION['username'],$cash_admins)){
				$_SESSION['is_cash_admin']=true;
			};
		}else{
			header('Location: auth.php');
			exit;
		}
	};
	if ($authok==false){
		header('Location: auth.php');
		exit;
	};
}

?>
