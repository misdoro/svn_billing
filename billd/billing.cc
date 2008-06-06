//
// File:   billing.cc
// Author: flexx
//

#include "billing.h"

using namespace std;

//Define mutexes:
pthread_mutex_t users_table_m = PTHREAD_MUTEX_INITIALIZER;

//Define global variables:
user * firstuser;

// configuration
struct configuration cfg;

// need to close all files, kill threads e.t.c.
void end_me (int sig) {
	logmsg(DBG_ALWAYS,"Terminating application...");
	cfg.stayalive=false;
	logmsg(DBG_ALWAYS,"Please wait 30 seconds");
}

int main(int argc, char** argv) {
// here - read configuration from /etc/billd.conf
	ConfigFile config( "/etc/billd.conf" );
	cfg.stayalive=true;
	//NetFlow sink port and address:
	config.readInto( cfg.netflow_listen_port, "billd_netflow_port" );//Port to listen for netflow data
	string nla;
	config.readInto( nla, "billd_netflow_addr", string("127.0.0.1") );
	cfg.netflow_listen_addr=ipFromStrToInt(nla.c_str());
	//Netflow socket timeout
	config.readInto( cfg.netflow_timeout, "billd_netflow_timeout",(uint16_t) 5);
	//Host and port for events
	string ela;
	config.readInto( ela, "billd_events_addr", string("127.0.0.1") );
	cfg.events_listen_addr=ipFromStrToInt(ela.c_str());
	config.readInto( cfg.events_listen_port, "billd_events_port", (uint16_t) 10203);
	config.readInto( cfg.events_threads, "billd_events_threads", (uint16_t) 5);
	//config.readInto( cfg.events_threads_min, "billd_events_minthreads", (uint16_t) 5);
	config.readInto( cfg.events_timeout, "billd_events_timeout", (uint16_t) 1);

	config.readInto( cfg.mysql_server,	"billd_mysql_host",	string("127.0.0.1"));
	config.readInto( cfg.mysql_port,	"billd_mysql_port",	(uint16_t) 3306);
	config.readInto( cfg.mysql_database,	"billd_mysql_database",	string("billing"));
	config.readInto( cfg.mysql_username,	"billd_mysql_username",	string(""));
	config.readInto( cfg.mysql_password,	"billd_mysql_password",	string(""));
	config.readInto( cfg.debug_locks,	"billd_debug_locks", 	false);
	config.readInto( cfg.debug_netflow,	"billd_debug_netflow", 	false);
	config.readInto( cfg.debug_offload,	"billd_debug_offload", 	false);
	config.readInto( cfg.debug_events,	"billd_debug_events",	false);
	config.readInto( cfg.debug_threads, "billd_debug_threads",	false);
	config.readInto( cfg.debug_hpstat, 	"billd_debug_hpstat",	false);
	config.readInto( cfg.log_date,	"billd_log_date",	true);
	config.readInto( cfg.verbose_daemonize,	"billd_debug_daemonize",false);
	config.readInto( cfg.do_fork,		"billd_daemon_mode",	true);
	config.readInto( cfg.mpd_shell_port,	"billd_mpd_shell_port");
	config.readInto( cfg.mpd_shell_addr,	"billd_mpd_shell_addr");
	config.readInto( cfg.mpd_shell_user,	"billd_mpd_shell_user");
	config.readInto( cfg.mpd_shell_pass,	"billd_mpd_shell_pass");
	config.readInto( cfg.logfile,		"billd_log_file",	string("/dev/null"));
	config.readInto( cfg.appendlogs,	"billd_append_logs",	true);
	config.readInto( cfg.pidfile,		"billd_pid_file",	string("/var/run/billd.pid"));
	config.readInto( cfg.lockfile,		"billd_lock_file",	string("/var/run/billd.lock"));
	config.readInto( cfg.user,		"billd_run_user",	string("nobody"));
	config.readInto( cfg.workingdir,	"billd_working_dir",	string("/"));
	config.readInto( cfg.stats_update_interval, "billd_stats_update_interval", (uint32_t) 25);
	config.readInto( cfg.fs_update_interval, "billd_stats_update_interval", (uint32_t) 25);
	config.readInto( cfg.die_time_interval, "billd_user_grace_time", (uint32_t) 30);
	/*if ((cfg.myconn= connectdb()) == NULL) {
		logmsg(DBG_ALWAYS,"Error. Could not connect to database.\n");
	}*/
	//Init MySQL library:
	my_init();
//	makeDBready();

// here - fork application if configuration right
	if (cfg.do_fork){
		daemonize();
	};

	signal(SIGTERM, end_me);
	signal(SIGINT, end_me);
	signal(SIGPIPE, SIG_IGN);
// here - connect to mysql, read usertables, zones
//
// here - start threads
	pthread_t threads[4];
	int rc = 0;
	uint64_t t=0;
//Create user connect/disconnect listener
	rc = pthread_create(&threads[0], NULL, userconnectlistener, (void *)t);
	if (rc) {
		logmsg(DBG_ALWAYS,"ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
//Create NETFLOW listener
	rc = pthread_create(&threads[1], NULL, netflowlistener, (void *)t);
	if (rc) {
		logmsg(DBG_ALWAYS,"ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
//Create stats update thread:
	rc = pthread_create(&threads[2], NULL, statsupdater, (void *)t);
	if (rc) {
		logmsg(DBG_ALWAYS,"ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
//Create hostport stats update thread:
	rc = pthread_create(&threads[3], NULL, flowstatsupdater, (void *)t);
	if (rc) {
		logmsg(DBG_ALWAYS,"ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

	if (cfg.do_fork) fflush(stdout);
	while (cfg.stayalive) sleep(1);
	logmsg(DBG_THREADS,"Main thread prepares to quit");
	sleep(5);
	//if (cfg.do_fork) fclose(stdout);
	mysql_thread_end();
	mysql_library_end();
	logmsg(DBG_ALWAYS,"Main thread quits");
	pthread_exit(NULL);
}

