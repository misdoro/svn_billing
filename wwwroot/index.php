<?
require ('auth.php');

echo '<html><head>';
echo '<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">';
echo '<title>Личный кабинет: '.$_SESSION['fullname'].'</title>';
echo '</head><body>';
echo 'Здравствуйте, '.$_SESSION['firstname'].'!';

//Print out credit and debit values:
$query='select id,debit,credit from users where id=\''.$_SESSION['bill_id'].'\';';
//echo $query;
$res=$mysqli->query($query);
$l=$res->fetch_row();
if ($l){
	//$_SESSION['id']=$l[0];
	echo "<br>Баланс лицевого счета: ".sprintf('%f',$l[1]).' рублей.';
	echo "<br>Кредит: ".sprintf('%f',$l[2]).' рублей.';
}


echo '<br><a href="stats.php">Статистика интернет</a>';
if ($_SESSION['is_admin']) echo '<br><a href="admin.php">Админка</a>';
if ($_SESSION['is_cash_admin']) echo '<br><a href="payment.php">Зачисление платежей</a>';
?>
		<br><a href="packs.php">Управление пакетами</a>
		<br><a href="index.php?logout=1">Выйти!</a>
	</body>
</html>
