<?
//$mysql_host='127.0.0.1';
$mysql_host='192.168.64.252';
$mysql_port=3306;
$mysql_user='billing';
$mysql_password='billadm';
$mysql_database='billing';
$dbcharset='utf8';

$ldap_uri='192.168.64.252';
$ldap_binddn='uid=billing,ou=admins,dc=openlan,dc=nnov,dc=ru';
$ldap_bindpass='vOVWpAk2g6';
$ldap_searchbase='ou=users,dc=openlan,dc=nnov,dc=ru';


session_start();

$admin_users=array();
$cash_admins=array();
$admin_users=array('misdoro');
$cash_admins=array('misdoro','flexx');


//Business mode makes it look like all price units are in MB etc
$business_mode=1;

//Show traffic packs
$show_packs=false;

//precision:
ini_set("precision", 15);

//Authentication mode
$auth_mode='mysql';
/*Auth mode to use: 
*	ldap - get user's password from OpenLDAP
*	mysql - get password from the same table as users
*	pam - use PAM to auth users
*/

//Enabled statistics modes (admin can choose every type anyway)
$stat_modes=array(0,1);
/*
0 - enable per-zone stats
1 - enable daily stats
2 - enable per-session stats
*/
$config_set=true;

require ('funcs.php');

?>
