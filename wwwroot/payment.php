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

if (isset($_POST['uname']) && isset($_POST['credit']) && isset($_POST['amount'])){
	$amount=(int)$_POST['amount'];
	$credit=(int)$_POST['credit'];
	$user=filterit($_POST['uname']);
	$query='update users set debit=debit+'.$amount.', credit='.$credit.' where login=\''.$user.'\'';
	logquery($query,$mysqli);
};


echo '<html><head>';
echo '<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">';
echo '<title>Зачисление платежей: '.$_SESSION['fullname'].'</title>';

echo "<script language='JavaScript' type='text/javascript'>var debit = new Array(); var credit = new Array();</script>";
echo '<script language="JavaScript" type="text/javascript">
		function ask_submit(){
	username=document.getElementById("user_name").value;
	uamount=document.getElementById("amount").value;
	ucredit=document.getElementById("credit").value;
	
	if (confirm("Добавить "+uamount+" рублей на баланс пользователя "+username+" с кредитом "+ucredit+"?")) {
		document.forms[0].submit();
	};
	
};
function changed_user(){
	uname=document.getElementById("user_name").value;
	ucredit=credit[uname];
	document.getElementById("uname").value=uname;
	document.getElementById("credit").value=credit[uname];
}
</script>';
/*
echo "colcount=".$vis['colcount'].";";
echo 'markedrowid=\'';
if ((int)$_SESSION['tasks_light']) {echo 'row_'.(int)$_SESSION['tasks_light'];};
echo '\';';
echo 'mypage=\'tasks.php\';';
echo 'inithandlers();';
echo "</script>";*/
echo '</head><body>';
echo 'Здравствуйте, '.$_SESSION['firstname'].'!';


//get all users and their debits, credit, create list, let choose

echo '<form action="payment.php" method="POST">';
echo '<select name="user_name" id="user_name" onchange="changed_user();">';
echo '<option value="Выберите пользователя:">Выберите пользователя:';
$query='select id,login,debit,credit from users;';
$res=$mysqli->query($query);
while ($l=$res->fetch_row()){
	echo '<option value="'.$l[1].'" ';
	if (isset($user)) { if ( $l[1] == $user ) {echo 'selected';};};
	echo '>';
	if (($l[2]+$l[3])< 0) echo '*';
	echo $l[1].' ( баланс '.$l[2].' руб, кредит '.$l[3].' руб)';
	echo "<script language='JavaScript' type='text/javascript'> debit['".$l[1]."']=".$l[2]."; credit['".$l[1]."']=".$l[3].";</script>";
};
echo '</SELECT>';
echo '<p>Зачислить пользователю: <input name="uname" id="uname" size="20">';
echo '<br><input name="amount" id="amount" size="20"> рублей';
echo ' и установить кредит <input name="credit" id="credit" size="20"> рублей';
echo '<br><input name="pay" value="Зачислить" type="button" onclick="ask_submit();">';
echo '</form>';


echo '<br><a href="index.php">На главную</a>';
echo '</body></html>';

?>
