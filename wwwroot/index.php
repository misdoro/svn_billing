<?
require ('auth.php');

echo '<html><head>';
echo '<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">';
echo '<title>Личный кабинет: '.$_SESSION['fullname'].'</title>';
echo '</head><body>';
echo 'Здравствуйте, '.$_SESSION['firstname'].'!';

//Get credit, debit and monthly limit values:
$query='select id,debit,credit,mlimit from users where id=\''.$_SESSION['bill_id'].'\';';
//echo $query;
$res=$mysqli->query($query);
$l=$res->fetch_row();
if ($l){
	//$_SESSION['id']=$l[0];
	if ($business_mode){
		echo '<br> '.$lang['remaining'].sprintf('%01.2f',$l[1]).$lang['traffic_units'].'.';
		echo '<br> '.$lang['monthlylimit'].sprintf('%01.2f',$l[3]).$lang['traffic_units'].'.';
		
	}else{
		echo '<br> '.$lang['debit'].sprintf('%01.2f',$l[1]).$lang['price_units'].'.';
		echo '<br> '.$lang['credit'].sprintf('%01.2f',$l[2]).$lang['price_units'].'.';
		
	};
};

$packs_available=0;
//User active price:
if (!$business_mode){
$query = 'SELECT pn.name,pn.packs_available FROM price_names pn, users u WHERE u.active_price=pn.id AND u.id='.$_SESSION['user_id'];
$res=$mysqli->query($query);
$l=$res->fetch_row();
if ($l){
	echo '<br> Текущий тариф: '.$l[0].'.';
	$packs_available=$l[1];
};

$query = 'SELECT pn.name FROM price_names pn, users u WHERE u.request_price=pn.id AND u.id='.$_SESSION['user_id'];
$res=$mysqli->query($query);
$l=$res->fetch_row();
if ($l){
	echo '<br> Заказана смена тарифа на: '.$l[0].'.';
};

};

echo '<br><a href="stats.php">'.$lang['stats'].'</a>';

if ($business_mode){ 
	if ($_SESSION['is_admin']) echo '<br><a href="users.php">Все пользователи</a>';
	if ($_SESSION['is_admin']) echo '<br><a href="sarg/">Подробная статистика по www траффику</a>';
	if ($_SESSION['is_admin']) echo '<br><a href="zones.php">Редактирование зон траффика</a>';
	if ($_SESSION['is_cash_admin']) echo '<br><a href="payment.php">Добавление траффика</a>';
}else {
	echo '<br><a href="price.php">Выбор тарифа</a>';
	if ($_SESSION['is_admin']) echo '<br><a href="users.php">Список пользователей</a>';
	if ($_SESSION['is_admin']) echo '<br><a href="zones.php">Редактирование зон</a>';
	if ($_SESSION['is_cash_admin']) echo '<br><a href="payment.php">Зачисление платежей</a>';
};
if ($show_packs && $packs_available) echo '<br><a href="packs.php">Управление пакетами</a>';
?>
		<br><a href="index.php?logout=1">Выйти!</a>
	</body>
</html>
