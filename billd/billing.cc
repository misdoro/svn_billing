//Core file, starting all threads.

#include "billing.h"

using namespace std;


//Define global variables:
//Configuration class
Config cfg;
//NAS List (top object for all NASes, users)
NASList nases;


// need to close all files, kill threads e.t.c.
void end_me (int sig) {
	logmsg(DBG_ALWAYS,"Terminating application...");
	cfg.stayalive=false;
	logmsg(DBG_ALWAYS,"Please wait some time");
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
    delete[] config_paths;
	//Init MySQL library:
	my_init();

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
	/*rc = pthread_create(&threads[2], NULL, flowstatsupdater, (void *)t);
	if (rc) {
		logmsg(DBG_ALWAYS,"ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}*/
	//Create periodic thread:
	C_periodic my_cron;
	my_cron.start();


	if (cfg.do_fork) fflush(stdout);
	while (cfg.stayalive) sleep(1);
	logmsg(DBG_THREADS,"Main thread prepares to quit");
    //for (int i=0;i<3;i++) pthread_join(threads[i],NULL);

	//sleep(5);
	//if (cfg.do_fork) fclose(stdout);
	mysql_thread_end();

	//Join threads:
	logmsg(DBG_THREADS,"Wait for periodic thread");
	my_cron.tryJoin();
	logmsg(DBG_THREADS,"Wait for events listener thread");
	pthread_join (threads[0],NULL);
	logmsg(DBG_THREADS,"Wait for netflow listener thread");
	pthread_join (threads[1],NULL);
	mysql_library_end();
	logmsg(DBG_ALWAYS,"Main thread quits");
	pthread_exit(NULL);
}

