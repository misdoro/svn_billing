/*
Table for zone record specification:
*/
CREATE TABLE IF NOT EXISTS `allzones` (
	`id`		int unsigned NOT NULL auto_increment,
	`ip`		int unsigned NOT NULL default '0',
	`mask`		smallint unsigned NOT NULL default '0',
	`dstport`	smallint unsigned NOT NULL default '0',
	`comment`	varchar(255) NOT NULL,
	PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*
table for groups:
*/
CREATE TABLE IF NOT EXISTS `groupnames` (
	`id`		int NOT NULL auto_increment,
	`caption`	varchar(255) NOT NULL,
	PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*
Table for per-server session summary stats
*/
CREATE TABLE IF NOT EXISTS `hostport_stat` (
	`id`		int unsigned NOT NULL auto_increment,		/*record ID*/
	`host`		int unsigned NOT NULL default 0,
	`port`		smallint unsigned NOT NULL default 0,
	`traf_in`	bigint unsigned NOT NULL default '0',
	`traf_out`	bigint unsigned NOT NULL default '0',
	`packets_in`	int unsigned NOT NULL default '0',
	`packets_out`	int unsigned NOT NULL default '0',
	`session_id`	int unsigned NOT NULL default '0',
	`updatescount`	smallint unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `session_id` (`session_id`),
  key `net_key` (`host`,`port`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Table for NAS list:
*/
CREATE TABLE IF NOT EXISTS `naslist` (
	`id`		int unsigned NOT NULL auto_increment,	/*just NAS ID*/
	`flow_ip`	int unsigned NOT NULL,		/*IP for netflow and userconnect events*/
	`flow_port`	smallint unsigned NOT NULL,	/*source port for netflow datagrams*/
	`manage_ip`	int unsigned NOT NULL,		/*management ip*/
	`manage_port`	smallint unsigned NOT NULL,	/*management port*/
	`name`		varchar(255) NOT NULL,		/*NAS name*/
	`info`		varchar(255) NOT NULL,		/*some info*/
	PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*
Operations log (all updates and inserts using web-interface)
*/
CREATE TABLE IF NOT EXISTS `oplog` (
	`id`		int unsigned NOT NULL auto_increment,	/*Record ID*/
	`uid`		int unsigned NOT NULL,			/*Who taken action*/
	`affected_uid`	int unsigned NOT NULL,			/*Affected user*/
	`debit_change`	double NOT NULL default 0,		/*Debit change*/
	`action_type`	int unsigned NOT NULL,			/*Action type*/
	`query`		text NOT NULL,				/*SQL query in action*/
	`date`		timestamp NOT NULL default CURRENT_TIMESTAMP, /*Date executed*/
	PRIMARY KEY  (`id`),
	key `user_id` (`uid`),
	key `affected` (`affected_uid`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Packs table:
*/
CREATE TABLE IF NOT EXISTS `packs` (
	`id`		int unsigned NOT NULL auto_increment,
	`name`		varchar(255) NOT NULL default '',
	`unittype`	tinyint unsigned default '1', /*1 is megabytes, others - for something else*/
	`unitcount` bigint unsigned NOT NULL default '0',	/*bytes count per-pack*/
	`unit_zone` int unsigned NOT NULL default '0',	/*If is byte-pack, take only bytes from this zone into account*/
	`on_price` decimal(11,2) NOT NULL default '0',	/*Amount of money to charge for adding this pack*/
	`duration` int unsigned NOT NULL default '0',	/*Pack duration in seconds*/
	`durationtype` tinyint unsigned default '1',	/*Pack duration units: 1 - exact duration, seconds
	2 - days, 3 - weeks, 4- months until next duration-length standard period, e.g. monday, 1st day of month, etc*/
	PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*###################
Tables for prices (aka groups in business version)
####################*/
CREATE TABLE IF NOT EXISTS `price_fees` (
	`id`		int unsigned NOT NULL auto_increment,	/*record ID*/
	`price_id`	int unsigned NOT NULL,	/*Price ID*/
	`fee`		decimal(6,2) default '0',
	`period`	smallint unsigned NOT NULL,
	`conditional`	boolean NOT NULL default true,
	PRIMARY KEY (`id`),
	key `price` (`price_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Periods:
*1 - daily fee
*2 - weekly fee
*3 - monthly fee
*/

CREATE TABLE IF NOT EXISTS `price_groups` (
	`id`		int unsigned NOT NULL auto_increment,	/*record ID*/
	`price_id`	int unsigned NOT NULL,	/*Price ID*/
	`group_id`	int unsigned NOT NULL,
	`mb_cost`	decimal(6,2) default '0',
	`bw_in`		int unsigned default '0',
	`bw_out`	int unsigned default '0',
	`priority`	int unsigned NOT NULL default '0',
	PRIMARY KEY (`id`),
	key `price` (`price_id`,`group_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `price_names` (
	`id` int unsigned NOT NULL auto_increment,      /*Price ID*/
	`name` varchar(255) NOT NULL,
	`active` boolean NOT NULL default 1,
	`positive_debit` boolean NOT NULL default 1,
	`fallback_price` int unsigned NOT NULL default 0,
	`packs_available` boolean NOT NULL default 0,
	PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*
Table for session data:
*/
CREATE TABLE IF NOT EXISTS `sessions`	(
	`id`		int unsigned NOT NULL auto_increment,
	`user_id`	int unsigned NOT NULL default '0',	/*If user found in users table, save his ID, elsewhere save username*/
	`user_name`	varchar(255) NULL default '',
	`session_id`	varchar(255) NOT NULL default '',
	`nas_port`	smallint unsigned NOT NULL default '0',
	`nas_linkname`	varchar(255) NOT NULL default '',	/*Used to drop users*/
	`ppp_ip`	int unsigned NOT NULL default '0',		/*NAS-assigned IP*/
	`called_ip`	int unsigned NOT NULL default '0',
	`called_mac`	bigint unsigned NOT NULL default '0',
	`called_ident`	varchar(255) NOT NULL default '', /*called ID, contains hostname and OS info ]:-> */
	`sess_start`	timestamp NOT NULL default CURRENT_TIMESTAMP,
	`sess_end`	timestamp NOT NULL default '0000-00-00 00:00:00',
	`state`		tinyint unsigned NOT NULL default '0',
	`active_price`  int(10) unsigned NOT NULL default '0',
	PRIMARY KEY  (`id`),
	UNIQUE KEY `sess_id` (`session_id`(7)),
	key `user_id` (`user_id`),
	key `user_name` (`user_name`),
	key `caller` (`called_ip`,`called_mac`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Table for per-zone statistics (per-session summary)
*/
CREATE TABLE IF NOT EXISTS `session_statistics` (
	`id`		int unsigned NOT NULL auto_increment,		/*record ID*/
	`zone_group_id`	int unsigned  NOT NULL,
	`traf_in`	int unsigned NOT NULL default '0',
	`traf_out`	int unsigned NOT NULL default '0',
	`traf_in_money`	double NOT NULL default '0',	/*Let it be double precision*/
	`stat_time`	timestamp NOT NULL default CURRENT_TIMESTAMP,	/*record date*/
	`session_id`	int unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `user_id` (`zone_group_id`),
  KEY `session_id` (`session_id`),
  KEY `timekey` (`stat_time`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Per-user connected packs
*/
CREATE TABLE IF NOT EXISTS `userpacks` (
	`id`		int(10) unsigned NOT NULL auto_increment,
	`user_id`	int(10) unsigned NOT NULL default '0',
	`pack_id`	int(10) unsigned NOT NULL default '0',
	`date_on`	timestamp NOT NULL default CURRENT_TIMESTAMP,
	`units_left`	bigint(20) unsigned NOT NULL default '0',
	`date_expire`	timestamp NOT NULL default '0000-00-00 00:00:00',
	`unit_zone`	int(10) unsigned NOT NULL default '0',
	`unittype`	int(10) unsigned NOT NULL default '0',
	PRIMARY KEY	(`id`),
	KEY `user_id`	(`user_id`),
	KEY `pack_id`	(`pack_id`),
	KEY `date_on`	(`date_on`),
	KEY `expires`	(`date_expire`),
	KEY `unit_zone`	(`unit_zone`,`unittype`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=3 ;

/*
Users table:
*/
CREATE TABLE IF NOT EXISTS `users` (
	`id`		int unsigned NOT NULL auto_increment,
	`login`		varchar(64) NOT NULL,	/*same as in LDAP*/
	`password`	varchar(64) NOT NULL,	/*Not used, auth using LDAP*/
	`user_ip`	int unsigned NOT NULL default '0',	/*per-user defined IP, violates prices*/
	`active`	tinyint unsigned default '1',	/*Is user enabled or not for some reasons*/
	`debit`		double NOT NULL default '0',	/*double precision*/
	`credit`	decimal(11,2) NOT NULL default '0',						/*11 digit precision, fractional 2*/
	`mlimit`	decimal(11,2) NOT NULL default '0',                                          /*11 digit precision, fractional 2*/
	`parent`	int(10) unsigned default NULL,
	`active_price`	int(10) unsigned NOT NULL default '0',
	`last_active`	timestamp NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

/*
Zones assignment to groups:
*/
CREATE TABLE IF NOT EXISTS `zone_groups` (
	`id`		int unsigned NOT NULL auto_increment,
	`zone_id`	int unsigned NOT NULL default '0',
	`group_id`	int unsigned NOT NULL default '0',
	`priority`	smallint unsigned NOT NULL,
	PRIMARY KEY  (`id`),
	KEY `zone_id` (`zone_id`),
	KEY `group_id` (`group_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;











