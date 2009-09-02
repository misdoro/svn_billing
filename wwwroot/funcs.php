<?

function filterit($string){
return preg_replace("/([^a-zA-Z0-9_.])/","",$string);
};

function logquery($query,$mysqli){
	$mysqli->query($query);
	$query1='insert into oplog (uid,query) select id,\''.$mysqli->real_escape_string($query).'\' from users where login=\''.$_SESSION['username'].'\'';
	return $mysqli->query($query1);
};

function trimstr($str,$len){
	$str1=mb_substr($str,0,$len,'utf-8');
	if (mb_strlen($str,'utf-8')<=mb_strlen($str1,'utf-8')+5) return $str;
	else return $str1.'...';
};

function check_admin(){
	if (($_SESSION['is_admin']!=true)){
		echo '<html><head>';
		echo '<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">';
		echo '</head><body>';
		echo '<h1>Вам сюда нельзя...</h1>';
		echo '</body></html>';
		die();
	};
	return 1;
};

?>