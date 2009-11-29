//
// NAS class constructor and routines
//
#include "billing.h"


//Constructor
C_NAS::C_NAS (MYSQL_ROW result){
	//flow_src_port = atoi(result[2]);
	id=atoi(result[0]);
	flow_src_addr.sin_addr.s_addr = htonl(atol(result[1]));
	flow_src_addr.sin_port = htons(atoi(result[2]));
	name=result[3];
    //Start stats offloading thread
	start();
	//logmsg(DBG_ALWAYS,"Loaded NAS id %u ip %s port %u name %s",id,inet_ntoa(flow_src_addr),flow_src_port,name.c_str());
	logmsg(DBG_ALWAYS,"Loaded NAS id %u ip %s port %u name %s",id,inet_ntoa(flow_src_addr.sin_addr),ntohs(flow_src_addr.sin_port),name.c_str());
}

uint16_t C_NAS::getFlowSrcPort(void){
	return ntohs(flow_src_addr.sin_port);
	//return flow_src_port;
}

uint32_t C_NAS::getId(void){
	return id;
}

const char* C_NAS::getName(void){
    return name.c_str();
}

//Get user having this IP and session start&end time out of flow start&end time
C_user* C_NAS::getUserByIP(uint32_t ip_addr,uint32_t start_time,uint32_t end_time){
	C_user* myUser;

    //Iterators for lookup:
    pair<std::multimap<uint32_t,C_user*>::iterator,
        std::multimap<uint32_t,C_user*>::iterator> searchRes;
    std::multimap<uint32_t,C_user*>::iterator userIt;

    mylock.lockRead();

    searchRes = usersByIP.equal_range(ip_addr);
    userIt=searchRes.first;
    //Check all users with this IP, try to find one with matching time:
    while ((userIt!=searchRes.second) ||userIt!=usersByIP.end() ){
        myUser = userIt->second;
        mylock.unlockRead();
        if (myUser!=NULL){
            if (myUser->checkFlowTimes(start_time,end_time)){
                return myUser;
            };
        }else{
            return NULL;
        }
        mylock.lockRead();
        userIt++;
    };
	mylock.unlockRead();
	return NULL;
}

//Add new user to my maps
void C_NAS::add_user(C_user* newuser){
    mylock.lockWrite();
    usersByIP.insert(pair<uint32_t,C_user*>(newuser->getIP(),newuser));
	usersBySID[newuser->getSID()]=newuser;
    mylock.unlockWrite();
}

//Create stats update thread
void C_NAS::runThread() {
	logmsg(DBG_THREADS,"Started stats offloading thread for NAS %s", this->getName());
	MYSQL *su_link=connectdb();
	// update values in database
	while (cfg.stayalive) {
	    sleep(cfg.stats_update_interval);
	    logmsg(DBG_OFFLOAD,"Updating stats in MySQL for NAS %s", this->getName());

        //Iterate over users map, keep locking in mind!
        std::map<uint32_t,C_user*>::iterator usersIter;
        C_user* myUser;
        mylock.lockRead();
        usersIter = usersByIP.begin();
        while (usersIter!=usersByIP.end()){
            myUser = usersIter->second;
            mylock.unlockRead();

            if (myUser!=NULL){
                //Got user, save his stats.
                myUser->updateStats(su_link);
            };

            mylock.lockRead();
            usersIter++;
        };
        mylock.unlockRead();
/*
		// for remove disconnected users from table
		user * next_u;
		verbose_mutex_lock (&users_table_m);
		for (user * u = firstuser; u != NULL;) {
			if (((time(NULL) - u->die_time) > cfg.die_time_interval)&& u->die_time !=0) {
				// remove dead user
				next_u = u->next;
				removeUser(u);
				u = next_u;
				continue;
			}else{
				u = u->next;
			};
		}
		verbose_mutex_unlock (&users_table_m);*/

	}
	logmsg(DBG_THREADS,"Finishing stats offloading thread");
	mysql_close(su_link);
	mysql_thread_end();
	logmsg(DBG_THREADS,"Complete stats offloading thread");
	pthread_exit(NULL);
}
