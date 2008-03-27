// 
// File:   billing.cc
// Author: flexx
//

#include "billing.h"

using namespace std;

//Define mutexes:
pthread_mutex_t users_table_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mysql_mutex = PTHREAD_MUTEX_INITIALIZER;

//Define global variables:
user * firstuser;

// configuration
struct configuration cfg;

void err_func(char *msg) {
    perror(msg);
    exit(0);
}

// need to close all files, kill threads e.t.c.
void mysigterm (int sig) {
	printf("Terminating application...\n");
	cfg.terminate = 1;
}

int main(int argc, char** argv) {
// here - read configuration from /usr/local/billing/billd.conf
	ConfigFile config( "/usr/local/billing/billd.conf" );
	cfg.terminate = 0;
	config.readInto( cfg.netflow_listen_port, "billd_netflow_port" );//Port to listen for netflow data
	config.readInto( cfg.events_listen_port, "billd_events_port");//Port to listen for events
	config.readInto( cfg.mysql_server,	"billd_mysql_host",	string("127.0.0.1"));
	config.readInto( cfg.mysql_port,	"billd_mysql_port",	(uint16_t) 3306);
	config.readInto( cfg.mysql_database,	"billd_mysql_database",	string("billing"));
	config.readInto( cfg.mysql_username,	"billd_mysql_username",	string(""));
	config.readInto( cfg.mysql_password,	"billd_mysql_password",	string(""));
	config.readInto( cfg.mysql_reconnect_interval, "billd_mysql_reconnect_interval", (uint32_t) 3600);
	config.readInto( cfg.debug_locks,	"billd_debug_locks", 	false);
	config.readInto( cfg.debug_netflow,	"billd_debug_netflow", 	false);
	config.readInto( cfg.debug_offload,	"billd_debug_offload", 	false);	
	config.readInto( cfg.debug_events,	"billd_debug_events",	false);
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
	config.readInto( cfg.die_time_interval, "billd_user_grace_time", (uint32_t) 30);
	if (connectdb() == NULL) {
		err_func("Error. Could not connect to database.\n");
	}
//	makeDBready();

// here - fork application if configuration right
	if (cfg.do_fork){
		daemonize();
	};

	signal(SIGTERM, mysigterm);
	signal(SIGPIPE, SIG_IGN);
// here - connect to mysql, read usertables, zones
//
// here - start threads
	pthread_t threads[3];
	int rc = 0;
	uint64_t t=0;
//Create user connect/disconnect listener
	rc = pthread_create(&threads[0], NULL, userconnectlistener, (void *)t);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
//Create NETFLOW listener
	rc = pthread_create(&threads[1], NULL, netflowlistener, (void *)t);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
//Create stats update thread:
	rc = pthread_create(&threads[2], NULL, statsupdater, (void *)t);
        if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
                exit(-1);	
	};
	if (cfg.do_fork) fflush(stdout);
	while (!cfg.terminate) sleep(1);
	sleep(3);
	if (cfg.do_fork) fclose(stdout);
	pthread_exit(NULL);
}

