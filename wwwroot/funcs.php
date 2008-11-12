<?

function filterit($string){
return preg_replace("/([^a-zA-Z0-9_.])/","",$string);
};

function logquery($query,$mysqli){
	$mysqli->query($query);
	$query1='insert into oplog (uid,query) select id,\''.$mysqli->real_escape_string($query).'\' from users where login=\''.$_SESSION['username'].'\'';
	return $mysqli->query($query1);
};

?>