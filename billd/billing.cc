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
FILE * mystdout;

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
	mystdout = stdout;
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
	config.readInto( cfg.debug_locks,	"billd_debug_locks", 	false);
	config.readInto( cfg.debug_netflow,	"billd_debug_netflow", 	false);
	config.readInto( cfg.debug_offload,	"billd_debug_offload", 	false);	
	config.readInto( cfg.debug_events,	"billd_debug_events",	false);
	config.readInto( cfg.do_fork,		"billd_daemon_mode",	true);	
	config.readInto( cfg.mpd_shell_port,	"billd_mpd_shell_port");
	config.readInto( cfg.mpd_shell_addr,	"billd_mpd_shell_addr");
	config.readInto( cfg.mpd_shell_user,	"billd_mpd_shell_user");
	config.readInto( cfg.mpd_shell_pass,	"billd_mpd_shell_pass");
	
		
	cfg.die_time_interval = 30;
	// minimum time (in sec) before save stats to database
	cfg.stats_update_interval = 25;
	if (connectdb() == NULL) {
		err_func("Error. Could not connect to database.\n");
	}
//	makeDBready();

// here - fork application if configuration right
	if (cfg.do_fork){
		pid_t pid;
		if ((pid = fork()) < 0) {
			printf("Fork error. Daemon not started.\n");
			exit(0);
		} else if (pid != 0) {
			printf("Daemon successfully forked.\n");
			exit(0);
		};
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
	if (mystdout != stdout) fflush(mystdout);
	while (!cfg.terminate) sleep(1);
	sleep(3);
	if (mystdout!=stdout) fclose(mystdout);
	return (EXIT_SUCCESS);
}

