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

	//MPD shell options:
    struct hostent *hostInfo;
    hostInfo = gethostbyname(result[4]);
    shell_addr.sin_family = hostInfo->h_addrtype;
    memcpy((char *) &shell_addr.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
    uint16_t shellport=atoi(result[5]);
    shell_addr.sin_port = htons(shellport);
    shell_login=result[6];
    shell_password=result[7];


    //Start stats offloading thread
	start();
	//logmsg(DBG_ALWAYS,"Loaded NAS id %u ip %s port %u name %s",id,inet_ntoa(flow_src_addr),flow_src_port,name.c_str());
	logmsg(DBG_ALWAYS,"Loaded NAS id %u ip %s port %u name %s",id,inet_ntoa(flow_src_addr.sin_addr),ntohs(flow_src_addr.sin_port),name.c_str());
}

C_NAS::~C_NAS () {
    tryJoin();
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

sockaddr_in C_NAS::getShellAddr(){
    return shell_addr;
}

const char* C_NAS::getShellLogin(){
    return shell_login.c_str();
}

const char* C_NAS::getShellPassword(){
    return shell_password.c_str();
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
	    
            if (myUser->checkFlowTimes(start_time,end_time)&& (myUser->getIP() == ip_addr)){
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
    newuser->setNAS(this);
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
                //Check his debit
                if (!myUser->checkDebit(su_link)){
                    //Disconnect him if debit<0
                    myUser->dropUser();
                };
                //Delete him if he is already disconnected
                if (myUser->checkDelete()){
                    logmsg(DBG_EVENTS,"Deleting expired user %u",myUser->getSID());
                    //delete from NAS maps;
                    mylock.lockWrite();
                    usersBySID.erase(myUser->getSID());
                    usersByIP.erase(usersIter);
                    mylock.unlockWrite();
                    //Delete from naslist maps:
                    nases.UserDeleted(myUser);
                    //Delete user himself:
                    delete myUser;
                };
            };

            mylock.lockRead();
            usersIter++;
        };
        mylock.unlockRead();

	}
	logmsg(DBG_THREADS,"Finishing stats offloading thread");
	mysql_close(su_link);
	mysql_thread_end();
	logmsg(DBG_THREADS,"Complete stats offloading thread");
	pthread_exit(NULL);
}
