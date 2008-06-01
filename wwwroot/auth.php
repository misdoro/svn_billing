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

if (isset($_GET['logout'])){
	$_SESSION['username']='';
	$_SESSION['password']='';
	$_SESSION['is_admin']=false;
	$_SESSION['is_cash_admin']=false;
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
		echo '<title>Авторизация!</title>';
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
				$ldap_uid=$fields[0]["uid"][0];
				$ldap_pwd=$fields[0]['userpassword'][0];
				if (($ldap_uid == $_SESSION['username']) && ($ldap_pwd == $_SESSION['password'])){
					$authok=true;
					$_SESSION['fullname']=$fields[0]["cn"][0];
					$_SESSION['firstname']=$fields[0]["givenname"][0];
					$_SESSION['lastname']=$fields[0]["sn"][0];
					if (in_array($_SESSION['username'],$admin_users)){
						$_SESSION['is_admin']=true;
					};
					if (in_array($_SESSION['username'],$cash_admins)){
						$_SESSION['is_cash_admin']=true;
					};
					//get numeric ID from MySQL
					$query='SELECT id,parent FROM users WHERE login="'.$_SESSION['username'].'"';
					$res=$mysqli->query($query);
					$l=$res->fetch_row();
					$_SESSION['user_id']=(int)$l[0];
					if ($l[1]>0) $_SESSION['bill_id']=(int)$l[1];
					else $_SESSION['bill_id']=(int)$l[0];
				}else{
					header('Location: auth.php');
					exit;
				};
			}else {
				echo "Failed to bind!";
			};
		}else{
			echo "Failed to connect!";
		};
	};
	if ($authok==false){
		header('Location: auth.php');
		exit;
	};
}

?>
