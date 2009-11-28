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
Config cfg;

//NAS List (top object to all users)
NASList nases;

//all mutexes are inside their classes

// need to close all files, kill threads e.t.c.
void end_me (int sig) {
	logmsg(DBG_ALWAYS,"Terminating application...");
	cfg.stayalive=false;
	logmsg(DBG_ALWAYS,"Please wait 30 seconds");
}

int main(int argc, char** argv) {
	//Read configuration:
	int i=0;
	char **config_paths=new char*[3];
	config_paths[0]=(char*)"/etc/billd.conf";
	config_paths[1]=(char*)"/usr/local/etc/billd.conf";
	config_paths[2]=(char*)"/usr/local/billing/conf/billd.conf";
	while (!cfg.readconfig(config_paths[i++])){
	     if (i>3) {
	        logmsg(DBG_ALWAYS,"ERROR: no config files found!\n");
            exit(-1);
	     };
	    }

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

	//Load NAS list from MySQL server

	logmsg(DBG_ALWAYS,"loaded %i nases",nases.load());
	//Start NAS connectlistener thread


// here - start threads
	pthread_t threads[3];
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
//Create hostport stats update thread:
	rc = pthread_create(&threads[2], NULL, flowstatsupdater, (void *)t);
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

