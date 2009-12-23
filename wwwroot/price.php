<?
require ('auth.php');

if ($_SESSION['is_admin'] && isset($_POST['user'])) $_SESSION['price_uid']=(int)$_POST['user'];
if (!isset($_SESSION['price_uid'])) $_SESSION['price_uid']=(int)$_SESSION['user_id'];
$uid=(int)$_SESSION['price_uid'];

$result='';
//Activate requested price, if possible
if (isset($_POST['priceName'])){
	$price_id=(int)$_POST['priceName'];
	$query = 'SELECT pn.id,pn.name,pf.fee FROM price_names pn LEFT JOIN (price_fees pf) ON (pf.price_id=pn.id AND pf.period=0) WHERE  pn.active AND pn.id='.$price_id;
	//echo $query."<br>";	
	$res=$mysqli->query($query);
	$price_id='';
	$price_name='';
	$price_fee=0;
	if ($res) {
		$l=$res->fetch_row();
		$price_id=$l[0];
		$price_name=$l[1];
		$price_fee=$l[2];
	}
	if (!$price_fee) $price_fee=0;
	if ($_POST['immediate']){
		//User asked immediate change!
		//Check if user has enough money:
		$query='SELECT u.id,u.debit,u.active_price FROM users u WHERE  u.id='.$uid.'';
		//echo $query."<br>";
		$res=$mysqli->query($query);
		$l=$res->fetch_row();
		$id=(int)$l[0];
		$money=(int)$l[1];
		$active_price=(int)$l[2];
		if ($active_price==$price_id){
			$result='<h2 style="color:red">Выбранный тариф совпадает с активным!</h2>';
		}else if ($money>=$price_fee){
			$query='UPDATE users SET active_price='.$price_id.',request_price=0,debit=debit-'.$price_fee.' WHERE id='.$uid;
			//echo $query."<br>";
			logquery($query,$mysqli);
			$mysqli->query($query);
			$result='<h2 style="color:green">Тариф изменен на '.$price_name.'</h2>';
		}else {
			$result='<h2 style="color:red">Недостаточно средств для подключения выбранного тарифа!</h2>';
		}
	}else{
		$query='UPDATE users SET request_price='.$price_id.' WHERE id='.$uid;
		//echo $query."<br>";
		$mysqli->query($query);
	};
};


//Build users list if admin
if ($_SESSION['is_admin']){
	$usersel='<form action="price.php" method="POST"><p>Пользователь:';
	$usersel.='<select name="user" id="user">
			<option value="Выберите пользователя:">Выберите пользователя:';
	$query='select id,login,debit from users where parent is NULL;';
	$res=$mysqli->query($query);
	while ($l=$res->fetch_row()){
		$usersel.='<option value="'.$l[0].'"';
		if ($l[0]==$uid) $usersel.=' selected ';
		$usersel.='>'.$l[1].' ( Баланс '.$l[2].' руб)';
	};
	$usersel.='</SELECT><input type="submit" value="выбрать"></form>';
}else{
	$usersel = 'Пользователь: '.$_SESSION['username'];
};

//Build available prices list:
$query='SELECT pn.id,pn.name,pf.fee FROM users u,price_names pn LEFT JOIN (price_fees pf) ON (pf.price_id=pn.id AND pf.period=0) WHERE pn.active  AND ((pn.positive_debit AND u.debit>=0) OR (NOT pn.positive_debit)) AND u.id='.$uid;
//$query='SELECT packs.id,packs.name,packs.on_price from users,packs WHERE packs.on_price < (users.debit + users.credit) AND users.id = '.$uid;
$res=$mysqli->query($query);
$priceList='
		<SELECT name="priceName" id="priceName">
		<OPTION value="0">Выберите тариф:';
while ($l=$res->fetch_row()) {
	if (!$l[2]) $l[2]='0.00';
	$priceList.= '<OPTION value="'.$l[0].'" >'.$l[1];
	$priceList.=' (подключение '.$l[2].' рублей) ';
};
$priceList.= '</SELECT>';

//Get current and requested prices:
$activePrice='';
$selectedPrice='';
$query='SELECT pn.name FROM price_names pn, users u WHERE u.active_price=pn.id AND u.id='.$uid;
$res=$mysqli->query($query);
if ($res && ($l=$res->fetch_row())){
  $activePrice=$l[0];
};
$query='SELECT pn.name FROM price_names pn, users u WHERE u.request_price=pn.id AND u.id='.$uid;
$res=$mysqli->query($query);
if ($res && ($l=$res->fetch_row())){
  $selectedPrice='Выбранный тариф: <br> '.$l[0];
};

?>
<html>
		<head>
			<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">
			<link rel=stylesheet href=style.css>
			<title>Выбор тарифа: <?=$_SESSION['fullname'];?></title>
		</head>
		<body>
				<script language="JavaScript" type="text/javascript">
				function ask_submit(){
					pricelist=document.getElementById("priceName");
					pricename=pricelist.options[pricelist.selectedIndex].text;
					if (document.getElementById("immediate").checked){
					  mytext='Изменить тариф на '+pricename+' немедленно?';
					}else{
					  mytext='Изменить тариф на '+pricename+' по расписанию?';
					};
					if (pricelist.selectedIndex)
					if (confirm(mytext)) {
						document.forms["priceform"].submit();
					};
				};
				</script>
				<?=$usersel;?>
				
				<p>Текущий тариф:
				<br><?=$activePrice;?>
				<p><?=$selectedPrice;?>
				<form action="price.php" name="priceform" method="POST">
				<p>Выбрать тариф:
				<br><?=$priceList;?>
				<br><input type="checkbox" name="immediate" id="immediate" value="1">  <label for="immediate"> Изменить немедленно*</label>
				<br><input type="button" onclick="ask_submit();" value="Выбрать!">
				</form>
				<p>*Внимание, при немедленной смене тарифа взимается стоимость подключения выбранного тарифа!
				<br>В противном случае тариф меняется автоматически в 00:01 следующего дня, плата за подключение не взимается.
				<?=$result;?>
				<br><a href="index.php">На главную</a>
		</body>
</html>