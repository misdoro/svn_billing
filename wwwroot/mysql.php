<?

$mysqli = new mysqli($mysql_host, $mysql_user, $mysql_password, $mysql_database);

/* check connection */
if (mysqli_connect_errno()) {
    printf("Connect failed: %s\n", mysqli_connect_error());
    exit();
}


/* change character set to utf8 */
if (!$mysqli->set_charset($dbcharset)) {
    //printf("Error loading character set $charset: %s\n", $mysql->error);
} else {
    //printf("Current character set: %s\n", $mysqli->character_set_name());
}


/* Print current character set */
$charst = $mysqli->character_set_name();
?>