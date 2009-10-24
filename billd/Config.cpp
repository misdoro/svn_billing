/*
Class for reading & store config options
*/
#include "billing.h"

bool Config::readconfig(const char* filename){
	//Temp variables:
	string tempstr;

	stayalive=true;
	//Init config:
	try {
		ConfigFile config(filename);
		//NetFlow sink port and address:
		config.readInto( netflow_listen_port, "billd_netflow_port" );//Port to listen for netflow data
		string tempstr;

		config.readInto( tempstr, "billd_netflow_addr", string("127.0.0.1") );
		if (inet_aton(tempstr.c_str(),&netflow_listen_addr)==0){
			logmsg(DBG_ALWAYS,"INCORRECT NetFlow listen address",tempstr.c_str());
		};
		//Netflow socket timeout
		config.readInto( netflow_timeout, "billd_netflow_timeout",(uint16_t) 5);
		//Host and port for events
		config.readInto( tempstr, "billd_events_addr", string("127.0.0.1") );
		if (inet_aton(tempstr.c_str(),&events_listen_addr)==0){
			logmsg(DBG_ALWAYS,"INCORRECT events listen address",tempstr.c_str());
		};
		config.readInto( events_listen_port, "billd_events_port", (uint16_t) 10203);
		config.readInto( events_threads, "billd_events_threads", (uint16_t) 5);
		//config.readInto( cfg.events_threads_min, "billd_events_minthreads", (uint16_t) 5);
		config.readInto( events_timeout, "billd_events_timeout", (uint16_t) 1);

		config.readInto( mysql_server,	"billd_mysql_host",	string("127.0.0.1"));
		config.readInto( mysql_port,	"billd_mysql_port",	(uint16_t) 3306);
		config.readInto( mysql_database,	"billd_mysql_database",	string("billing"));
		config.readInto( mysql_username,	"billd_mysql_username",	string(""));
		config.readInto( mysql_password,	"billd_mysql_password",	string(""));
		config.readInto( debug_locks,	"billd_debug_locks", 	false);
		config.readInto( debug_netflow,	"billd_debug_netflow", 	false);
		config.readInto( debug_offload,	"billd_debug_offload", 	false);
		config.readInto( debug_events,	"billd_debug_events",	false);
		config.readInto( debug_threads, "billd_debug_threads",	false);
		config.readInto( debug_hpstat, 	"billd_debug_hpstat",	false);
		config.readInto( log_date,	"billd_log_date",	true);
		config.readInto( verbose_daemonize,	"billd_debug_daemonize",false);
		config.readInto( do_fork,		"billd_daemon_mode",	true);
		config.readInto( mpd_shell_port,	"billd_mpd_shell_port");
		config.readInto( mpd_shell_addr,	"billd_mpd_shell_addr");
		config.readInto( mpd_shell_user,	"billd_mpd_shell_user");
		config.readInto( mpd_shell_pass,	"billd_mpd_shell_pass");
		config.readInto( logfile,		"billd_log_file",	string("/dev/null"));
		config.readInto( appendlogs,	"billd_append_logs",	true);
		config.readInto( pidfile,		"billd_pid_file",	string("/var/run/billd.pid"));
		config.readInto( lockfile,		"billd_lock_file",	string("/var/run/billd.lock"));
		config.readInto( user,		"billd_run_user",	string("nobody"));
		config.readInto( workingdir,	"billd_working_dir",	string("/"));
		config.readInto( stats_update_interval, "billd_stats_update_interval", (uint32_t) 25);
		config.readInto( fs_update_interval, "billd_stats_update_interval", (uint32_t) 25);
		config.readInto( die_time_interval, "billd_user_grace_time", (uint32_t) 30);
		return true;
	}catch(ConfigFile::file_not_found){
		logmsg (DBG_ALWAYS,"Config file %s not found!",filename);
		return false;
	};
};
