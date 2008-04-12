<?
//
// Authentication handler
//
if (!isset($config_set)){
	require('config.php');
};
if (isset($_GET['logout'])){
	$_SESSION['username']='';
	$_SESSION['password']='';
};
//If we are calling ourself:
if (strstr($_SERVER['PHP_SELF'],'auth.php')){
	if (isset($_POST['username']) && isset($_POST['password'])){
		$_SESSION['username']=preg_replace("/([^a-zA-Z0-9])/","",$_POST['username']);
		$_SESSION['password']=preg_replace("/([^a-zA-Z0-9])/","",$_POST['password']);
		header('Location: index.php');
		exit;
	}else {
		echo '<html><head>';
		echo '<Meta http-equiv="Content-type" content="Text/html;charset=koi8-r">';
		echo '<title>Авторизация!</title>';
		echo '</head><body>';
		echo '<form action="auth.php" method="POST">';
		echo '<input name="username" size="20"><br>';
		echo '<input name="password" type="password" size="20"><br>';
		echo '<input name="SubmitBtn" type="submit">';
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
				//echo "Failed to set protocol version to 3";
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
					$_SESSION['fullname']=iconv("UTF-8","KOI8-U",$fields[0]["cn"][0]);
					$_SESSION['firstname']=iconv("UTF-8","KOI8-U",$fields[0]["givenname"][0]);
					$_SESSION['lastname']=iconv("UTF-8","KOI8-U",$fields[0]["sn"][0]);
				}else{
					//header('Location: auth.php');
					//exit;
				};
			}else {
				echo "Failed to bind!";
			};
		}else{
			echo "Failed to connect!";
		};
	}
	if ($authok==false){
		header('Location: auth.php');
		exit;
	};
}

?>
