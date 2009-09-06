//
// Database structure for REJIK management.
//

CREATE TABLE IF NOT EXISTS `r_groups` (
	`id` int unsigned NOT NULL auto_increment,
	`name` varchar(255) NOT NULL default 'noname',
	`redir_url` varchar(255) NOT NULL default 'http://ya.ru',
	`inverse` boolean NOT NULL default '0',
	PRIMARY KEY  (`id`)
)ENGINE=MyISAM DEFAULT CHARSET=utf8;
	
CREATE TABLE IF NOT EXISTS `r_users` (
	`id` int unsigned NOT NULL auto_increment,
	`r_group` int unsigned NOT NULL,
	`user_id` int unsigned NOT NULL,
	PRIMARY KEY  (`id`),
	KEY `r_group` (`r_group`),
	KEY `user_id` (`user_id`)
)ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `r_urlrec` (
	`id` unsigned NOT NULL auto_increment,
	`record` varchar(255) NOT NULL default '',
	`r_group` int unsigned NOT NULL,
	PRIMARY KEY  (`id`),
	KEY `r_group` (`r_group`)
)ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `r_pcrec` (
	`id` unsigned NOT NULL auto_increment,
	`record` varchar(255) NOT NULL default '',
	`r_group` int unsigned NOT NULL,
	PRIMARY KEY  (`id`),
	KEY `r_group` (`r_group`)
)ENGINE=MyISAM DEFAULT CHARSET=utf8;