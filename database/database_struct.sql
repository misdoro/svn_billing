/*
Table for zone record specification:
*/
CREATE TABLE IF NOT EXISTS `allzones` (
	`id` int unsigned NOT NULL auto_increment,
	`ip` int unsigned NOT NULL default '0',
	`mask` smallint unsigned NOT NULL default '0',
	`dstport` smallint unsigned NOT NULL default '0',
	`comment` varchar(255) NOT NULL,
	PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*
Zone group, also defining per-mb price:
*/
CREATE TABLE IF NOT EXISTS `zone_groups` (
	`id` int unsigned NOT NULL auto_increment,
	`zone_id` int unsigned NOT NULL default '0',
	`group_id` int unsigned NOT NULL default '0',
	`priority` smallint unsigned NOT NULL,
	PRIMARY KEY  (`id`),
	UNIQUE KEY `zone_id` (`zone_id`),
	KEY `group_id` (`group_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*
table for prices:
*/
CREATE TABLE IF NOT EXISTS `groupnames` (
	`id` int NOT NULL auto_increment,
	`caption` varchar(255) NOT NULL,
	`ippoolid` varchar(20) NOT NULL, /*ID of ip pool for this price, IP pool rules defined using pf, pool itself is stored in MPD*/
	`mb_cost` DECIMAL(11,2) NOT NULL default '0',   /*11 digit precision, fractional 2*/
	PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*
Table for session data:
*/
CREATE TABLE IF NOT EXISTS `sessions`	(
	`id` int unsigned NOT NULL auto_increment,
	`user_id` int unsigned NOT NULL default '0',	/*If user found in users table, save his ID, elsewhere save username*/
	`user_name` varchar(255) NOT NULL default '',
	`session_id` varchar(255) NOT NULL default '',
	`nas_port` smallint unsigned NOT NULL default '0',
	`nas_linkname` varchar(255) NOT NULL default '',	/*Used to drop users*/
	`ppp_ip` int unsigned NOT NULL default '0',		/*NAS-assigned IP*/
	`called_ip` int unsigned NOT NULL default '0',
	`called_mac` bigint unsigned NOT NULL default '0',
	`called_ident` varchar(255) NOT NULL default '', /*called ID, contains hostname and OS info ]:-> */
	`sess_start` timestamp NOT NULL default CURRENT_TIMESTAMP,
	`sess_end` timestamp NOT NULL default '0000-00-00 00:00:00',
	`state` tinyint unsigned NOT NULL default '0',
	PRIMARY KEY  (`id`),
	UNIQUE KEY `sess_id` (`session_id`(7)),
	key `user_id` (`user_id`),
	key `user_name` (`user_name`),
	key `caller` (`called_ip`,`called_mac`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Table for per-zone statistics (non summary)
*/
CREATE TABLE IF NOT EXISTS `statistics` (
	`id` int unsigned NOT NULL auto_increment,		/*record ID*/
	`user_id` int unsigned NOT NULL default '0',
	`zone_group_id` int unsigned  NOT NULL,
	`traf_in` int unsigned NOT NULL default '0',
	`traf_out` int unsigned NOT NULL default '0',
	`stat_time`	timestamp NOT NULL default CURRENT_TIMESTAMP,	/*record date*/
	`session_id` int unsigned NOT NULL default '0',
	PRIMARY KEY  (`id`),
	KEY `user_id` (`user_id`,`zone_group_id`),
	KEY `session_id` (`session_id`),
	KEY `stat_time` (`stat_time`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Table for per-zone statistics (per-session summary)
*/
CREATE TABLE IF NOT EXISTS `session_statistics` (
	`id` int unsigned NOT NULL auto_increment,		/*record ID*/
	`user_id` int unsigned NOT NULL default '0',
	`zone_group_id` int unsigned  NOT NULL,
	`traf_in` bigint unsigned NOT NULL default '0',
	`traf_out` bigint unsigned NOT NULL default '0',
	`traf_in_money` double NOT NULL default '0',	/*Let it be double precision*/
	`stat_time`	timestamp NOT NULL default CURRENT_TIMESTAMP,	/*record date*/
	`session_id` int unsigned NOT NULL default '0',
	PRIMARY KEY  (`id`),
	KEY `user_id` (`user_id`,`zone_group_id`),
	KEY `session_id` (`session_id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Table for user groups
*/
CREATE TABLE IF NOT EXISTS `usergroups` (
  `id` int unsigned NOT NULL auto_increment,
  `user_id` int unsigned NOT NULL,
  `group_id` int unsigned NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Users table:
*/
CREATE TABLE IF NOT EXISTS `users` (
  `id` int unsigned NOT NULL auto_increment,
  `login` varchar(64) NOT NULL,	/*same as in LDAP*/
  `password` varchar(64) NOT NULL,	/*Not used, auth using LDAP*/
  `user_ip` int unsigned NOT NULL default '0',	/*per-user defined IP, violates prices*/
  `active` tinyint unsigned default '1',	/*Is user enabled or not for some reasons*/
  `debit` double NOT NULL default '0',	/*double precision*/
  `credit` decimal(11,2) NOT NULL default '0',						/*11 digit precision, fractional 2*/
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Packs table:
*/
CREATE TABLE IF NOT EXISTS `packs` (
	`id` int unsigned NOT NULL auto_increment,
	`unittype` tinyint unsigned default '1', /*1 is megabytes, others - for something else*/
	`unitcount` bigint unsigned NOT NULL default '0',	/*bytes count per-pack*/
	`unit_zone` int unsigned NOT NULL default '0',	/*If is byte-pack, take only bytes from this zone into account*/
	`on_price` decimal(11,2) NOT NULL default '0',	/*Amount of money to charge for adding this pack*/
	`duration` int unsigned NOT NULL default '0',	/*Pack duration in seconds*/
	`durationtype` tinyint unsigned default '1',	/*Pack duration type: 1 - exactly duration, 2 - until next duration-length standard period, e.g. monday, 1st day of month, etc*/
	PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Per-user connected packs
*/
CREATE TABLE IF NOT EXISTS `userpacks` (
	`id` int unsigned NOT NULL auto_increment,		/*Record ID*/
	`user_id` int unsigned NOT NULL default '0',	/*User of this pack*/
	`pack_id` int unsigned NOT NULL default '0',	/*Pack ID*/
	`date_on` timestamp NOT NULL default CURRENT_TIMESTAMP, /*Date on*/
	`units_left` bigint unsigned NOT NULL default '0',	/*units (bytes) left in pack*/
	PRIMARY KEY  (`id`),
	key `user_id` (`user_id`),
	key `pack_id` (`pack_id`),
	key `date_on` (`date_on`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;


CREATE TABLE IF NOT EXISTS `oplog` (
	`id` int unsigned NOT NULL auto_increment,	/*Record ID*/
	`uid` int unsigned NOT NULL default '0',		/*Who taken action*/
	`query` text NOT NULL,	/*SQL query in action*/
	`date` timestamp NOT NULL default CURRENT_TIMESTAMP, /*Date executed*/
	PRIMARY KEY  (`id`),
	key `user_id` (`uid`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;