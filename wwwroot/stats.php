<?
require ('auth.php');
require ('mysql.php');
echo '<html><head>';
echo '<Meta http-equiv="Content-type" content="Text/html;charset=koi8-r">';
echo '<title>Статистика интернет: '.$_SESSION['fullname'].'</title>';
echo '</head><body>';
echo 'Здравствуйте, '.$_SESSION['firstname'].'!';


echo '<br><a href="index.php">На главную</a>';
echo '</body></html>';

?>