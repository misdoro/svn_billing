//
// Perform periodic tasks (like cron, yeah!)
//
#include "billing.h"

//Constructor:
C_periodic::C_periodic(){
    lastDailyRun = 0;
    startedDate = time(NULL);
}

C_periodic::~C_periodic(){
}

//Separate thread, running jobs sometimes:
void C_periodic::runThread(){
    logmsg(DBG_THREADS,"Started periodic tasks thread");
	MYSQL *sqllink=connectdb();
	// update values in database
	while (cfg.stayalive) {
        sleep (1);
        if(checkDailyRun()){//Run daily tasks on time!
            runDailyTasks(sqllink);
        };
    };
    logmsg(DBG_THREADS,"Stopping periodic tasks thread");

    mysql_close(sqllink);
	mysql_thread_end();

    pthread_exit(NULL);
}

//Check if daily tasks need to be started
bool C_periodic::checkDailyRun(){

    time_t nowtime = time(NULL);                //Current time
    struct tm * mytime = localtime(&nowtime);   //Local time
    uint16_t minutes = (mytime->tm_min + mytime->tm_hour*60);    //Minutes since start of day

    //Check if we started at least maxlag minutes before:
    if (nowtime<lastDailyRun+cfg.periodicDailyMaxLag*60) return false;

    if ( (minutes >= cfg.periodicDailyMinute) && (minutes < (cfg.periodicDailyMinute+cfg.periodicDailyMaxLag))){//We can start if did not do it today:
        lastDailyRun = nowtime;
        return true;
    };
    //logmsg(DBG_THREADS,"nowtime %u, minutes %u, periodic %u, periodic %u",nowtime,minutes,cfg.periodicDailyMinute,cfg.periodicDailyMinute+cfg.periodicDailyMaxLag);
    return false;

}


//Daily tasks to run:
void C_periodic::runDailyTasks(MYSQL* sqllink){
    char query[512];
    int status;
    logmsg(DBG_THREADS,"Doing our dirty daily jobs! :)");
    //Update users prices according to their selection
    sprintf(query,"UPDATE users SET active_price=request_price, request_price=0 where request_price>0;");
    status=mysql_query(sqllink,query);
    if (status){
        logmsg(DBG_ALWAYS,"can't execute query cause %s",mysql_error(sqllink));
    };
    //Set active price to one available at negative debit

    //Account money according to users active price
    //Conditional daily fees:
    //sprintf(query,"UPDATE users u, price_fees pf SET u.debit=u.debit-pf.fee WHERE u.active_price = pf.price_id AND u.last_active>DATE_ADD(CURRENT_TIMESTAMP(),INTERVAL -1 DAY) AND pf.conditional AND pf.period=1;");
    //Update parent users debit
    sprintf(query,"UPDATE users u LEFT JOIN price_fees AS pf ON pf.price_id=u.active_price LEFT JOIN users AS up ON up.id=u.parent SET up.debit=up.debit-pf.fee WHERE u.active_price=pf.id AND u.parent AND u.last_active>DATE_ADD(CURRENT_TIMESTAMP(),INTERVAL -1 DAY) AND pf.conditional AND pf.period=1;")
    status=mysql_query(sqllink,query);
    if (status){
        logmsg(DBG_ALWAYS,"can't execute query cause %s",mysql_error(sqllink));
    };
    //Update self users debit
    sprintf(query,"UPDATE users u LEFT JOIN price_fees AS pf ON pf.price_id=u.active_price SET u.debit=u.debit-pf.fee WHERE u.active_price=pf.id AND u.parent = NULL AND u.last_active>DATE_ADD(CURRENT_TIMESTAMP(),INTERVAL -1 DAY) AND pf.conditional AND pf.period=1;");
    status=mysql_query(sqllink,query);
    if (status){
        logmsg(DBG_ALWAYS,"can't execute query cause %s",mysql_error(sqllink));
    };

    //Unconditional daily fees:
    sprintf(query,"UPDATE users u, price_fees pf SET u.debit=u.debit-pf.fee WHERE u.active_price = pf.price_id AND NOT pf.conditional AND pf.period=1;");
    status=mysql_query(sqllink,query);
    if (status){
        logmsg(DBG_ALWAYS,"can't execute query cause %s",mysql_error(sqllink));
    };

}
