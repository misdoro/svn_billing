<?
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

$admin_users=array('misdoro');
$cash_admins=array('misdoro','flexx');

$config_set=true;

require ('funcs.php');

?>
