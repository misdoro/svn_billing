<?
require ('auth.php');
require ('mysql.php');
echo '<html><head>';
echo '<Meta http-equiv="Content-type" content="Text/html;charset=koi8-r">';
echo '<title>������ �������: '.$_SESSION['fullname'].'</title>';
echo '</head><body>';
echo '������������, '.$_SESSION['firstname'].'!';

//Print out credit and debit values:
$query='select id,debit,credit from users where login=\''.$_SESSION['username'].'\';';
//echo $query;
$res=$mysqli->query($query);
$l=$res->fetch_row();
if ($l){
	$_SESSION['id']=$l[0];
	echo "<br>������ �������� �����: ".sprintf('%f',$l[1]).' ������.';
	echo "<br>������: ".sprintf('%f',$l[2]).' ������.';
}


echo '<br><a href="stats.php">���������� ��������</a>';
echo '<br><a href="index.php?logout=1">�����!</a>';
echo '</body></html>';

?>
