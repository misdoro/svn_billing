<?
require ('auth.php');
require ('mysql.php');
if ($_SESSION['is_admin']!=true){
	echo '<html><head>';
	echo '<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">';
	echo '</head><body>';
	echo '<h1>Вам сюда нельзя...</h1>';
	echo '</body></html>';
	die();
};

echo '<html><head>';
echo '<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">';
echo '<title>Админка: '.$_SESSION['fullname'].'</title>';
echo '</head><body>';
echo 'Здравствуйте, '.$_SESSION['firstname'].'!';

/*//Print out credit and debit values:
$query='select id,debit,credit from users where login=\''.$_SESSION['username'].'\';';
//echo $query;
$res=$mysqli->query($query);
$l=$res->fetch_row();
if ($l){
	$_SESSION['id']=$l[0];
	echo "<br>Баланс лицевого счета: ".sprintf('%f',$l[1]).' рублей.';
	echo "<br>Кредит: ".sprintf('%f',$l[2]).' рублей.';
}


echo '<br><a href="stats.php">Статистика интернет</a>';
if ($_SESSION['is_admin']) echo '<br><a href="admin.php">Админка</a>';
*/
echo '<br><a href="index.php">На главную</a>';
echo '</body></html>';

?>
