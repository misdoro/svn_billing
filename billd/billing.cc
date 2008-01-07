// 
// File:   billing.cc
// Author: flexx
//
// Created on 18 Декабрь 2007 г., 20:19
//

#include "billing.h"

using namespace std;

//Define mutexes:
pthread_mutex_t users_table_m = PTHREAD_MUTEX_INITIALIZER;

//Define global variables:
user * firstuser;

// configuration
struct configuration cfg;
FILE * mystdout;

void err_func(char *msg) {
    perror(msg);
    exit(0);
}

// need close all files, kill threads e.t.c.
void mysigterm (int sig) {
	printf("Terminating application...\n");
	cfg.terminate = 1;
}

int main(int argc, char** argv) {
    mystdout = stdout;
// here - read configuration
    cfg.terminate = 0;
    cfg.netflow_port = 12345;
    cfg.connectmsgport = 10203;
    cfg.mysql_server = "127.0.0.1";
    cfg.mysql_port = 3306;    
    cfg.mysql_username = "root";
    cfg.mysql_database = "billing";
    cfg.mysql_password = "";
    cfg.die_time_interval = 30;
    // minimum time (in sec) before save stats to database
    cfg.stats_update_interval = 25;
    if (connectdb() == NULL) {
        err_func("Error. Could not connect to database.\n");
    }
    makeDBready();
   /* onUserConnected(-1062729719, 0);
    onUserConnected(-1062729720, 0);
    onUserConnected(-1062729718, 0);
    onUserConnected(-1062729717, 0);
    onUserConnected(-1062729721, 0);
    onUserConnected(-1062729722, 0);
    onUserConnected(-1062729723, 0);
    onUserConnected(-1062729724, 0);
    onUserConnected(-1062729725, 0);
    onUserConnected(-1062729726, 0);
    */
// here - fork application - if configuration right
/*    pid_t pid;
    if ((pid = fork()) < 0) {
    	printf("Fork error. Daemon not started.\n");
	exit(0);
    } else if (pid != 0) {
        printf("Daemon successfully forked.\n");
    	exit(0);
    }*/
    signal(SIGTERM, mysigterm);
    signal(SIGPIPE, SIG_IGN);
// here - connect to mysql, read usertables, zones
//............

// here - start threads
    pthread_t threads[1];
    int rc, t = 0;
    rc = pthread_create(&threads[0], NULL, userconnectlistener, (void *)t);
    if (rc) {
	printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
//Create NETFLOW listener
    rc = pthread_create(&threads[0], NULL, netflowlistener, (void *)t);
    if (rc) {
    	printf("ERROR; return code from pthread_create() is %d\n", rc);
    	exit(-1);
    }
    if (mystdout != stdout) fflush(mystdout);
    while (!cfg.terminate) sleep(1);
    sleep(3);
    if (mystdout!=stdout) fclose(mystdout);
    return (EXIT_SUCCESS);
}

