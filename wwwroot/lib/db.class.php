<?php

class DBException extends Exception
{
	function __construct($text = '')
	{
	    $message = mysqli_errno(DB::$link).' :: '.DB::$query;
		parent::__construct($text.' - '.$message, mysqli_error(DB::$link));
	}
}

class DB
{
	public static $link = null;
	public static $query = '';
	
	public static function connect() {
		$db = $GLOBALS['CORE_DB'];
		self::$link = mysqli_connect($db['host'],$db['user'],$db['pass'],$db['db'],$db['port']);
        if (!self::$link)
            throw new DBException('cannt connect to db');
		return true;
	}
	
	public static function doquery($query) {
	    self::$query = $query;
		if (!self::$link && !self::connect())
    	    return null;
		$result = mysqli_query(self::$link, $query);
		$id   = mysqli_errno(self::$link);
		if (mysqli_errno(self::$link) != 0)
		    throw new DBException('rise db error');
		return $result;
	}
	
	public static function query($query, $once = false) {
		if (!($result = self::doquery($query))) return $result;
				
		// ! надо будит переделать
		preg_match("/^SELECT|^SHOW|^DESCRIBE|^EXPLAIN|^DELETE|^INSERT|^REPLACE|^UPDATE/",strtoupper(substr(ltrim($query),0,10)),$a);
		switch (isset($a[0]) ? "".$a[0] : "")
		{
			case "INSERT":
				if ($once) return mysqli_insert_id(self::$link);
			case "DELETE":
			case "REPLACE":
			case "UPDATE":
				//для этой группы можно вернуть количество затронутых рядов и все, парсить ничего не надо.
				return mysqli_affected_rows(self::$link);
			case "SELECT":
			case "SHOW":
			case "DESCRIBE":
			case "EXPLAIN":
				//для этой группы только и можно производить дальнейшую обработку, так как у всех остальных $result = TRUE
				if (!mysqli_num_rows($result)) return null;
				break;
			default:
				return $result;
		}

		$ret = array();
		while ($row = mysqli_fetch_array($result, MYSQL_ASSOC)) {
			if ($once) return $row;
			$ret[] = $row;
		}
		return $ret;
	}

	public static function escape($query)
	{
		if (!self::$link)
		{
		    self::connect();
			if (!(self::$link))	return null;
		}
		return mysqli_real_escape_string(self::$link,$query);
	}
	
}


