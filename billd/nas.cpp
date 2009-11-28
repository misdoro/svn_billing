//
// NAS class constructor and routines
//
#include "billing.h"

C_NAS::C_NAS (MYSQL_ROW result){
	flow_src_port = atoi(result[2]);
	id=atoi(result[0]);
	name=result[3];

	logmsg(DBG_ALWAYS,"Loaded NAS id %u ip %s port %u name %",id,result[1],flow_src_port,result[3]);
}

uint16_t C_NAS::getFlowSrcPort(void){
	return flow_src_port;
}

uint32_t C_NAS::getId(void){
	return id;
}

const char* C_NAS::getName(void){
    return name.c_str();
}

C_user* C_NAS::getUserByIP(uint32_t ip_addr,uint32_t start_time,uint32_t end_time){
	C_user* myuser;
	mylock.lockRead();
	if (usersByIP.count(ip_addr)>1){
        myuser = usersByIP[ip_addr];
	}else {
	    myuser = usersByIP[ip_addr];
	};
	mylock.unlockRead();
	return myuser;
}

//Add new user to my maps
void C_NAS::add_user(C_user* newuser){
    mylock.lockWrite();
	usersByIP[newuser->getIP()]=newuser;
	usersBySID[newuser->getSID()]=newuser;
    mylock.unlockWrite();
};
