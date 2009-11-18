//
// NAS class constructor and routines
//
#include "billing.h"

C_NAS::C_NAS (MYSQL_ROW result){
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	listByIP_mutex = mutex;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
	listBySID_mutex = mutex1;
	flow_src_port = atoi(result[0]);
	flow_port = flow_src_port;
	id=atoi(result[0]);
	name=result[3];

	logmsg(DBG_ALWAYS,"Loaded NAS id %s ip %s port %s identifier %s",result[0],result[1],result[2],result[3]);
}

uint16_t C_NAS::getFlowPort(void){
	return flow_port;
}

uint16_t C_NAS::getFlowSrcPort(void){
	return flow_src_port;
}

uint32_t C_NAS::getId(void){
	return id;
}

C_user* C_NAS::getUserByIP(uint32_t ip_addr,uint32_t start_time,uint32_t end_time){
	verbose_mutex_lock(&listByIP_mutex);
	C_user* myuser = usersByIP[ip_addr];
	verbose_mutex_unlock(&listByIP_mutex);
	return myuser;
}

//Add new user to my maps
void C_NAS::add_user(C_user* newuser){
	verbose_mutex_lock(&listByIP_mutex);
	usersByIP[newuser->getIP()]=newuser;
	verbose_mutex_unlock(&listByIP_mutex);
	verbose_mutex_lock(&listBySID_mutex);
	usersBySID[newuser->getSID()]=newuser;
	verbose_mutex_unlock(&listBySID_mutex);
};
