//
//	Class for loading and manipulating NASes
//

#include "billing.h"


//Constructor. Load NASes from MYSQL
NASList::NASList (){
	pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;

	list_mutex = mutex;
	pthread_mutex_t	mutex1 = PTHREAD_MUTEX_INITIALIZER;
	id_mutex = mutex1;
	pthread_mutex_t	mutex2 = PTHREAD_MUTEX_INITIALIZER;
	userSidMutex = mutex2;
}

uint32_t NASList::load(){
	//Connect to MySQL
	MYSQL *sqllink = connectdb();
	MYSQL_ROW row;

	C_NAS* thisnas;

	logmsg(DBG_ALWAYS,"Loading NASes");

	//Load all NASes:
	std::string query;
	query = "SELECT id,ip,port,identifier FROM naslist;";
	MYSQL_RES *result;
	mysql_query(sqllink, query.c_str());
	result = mysql_store_result(sqllink);
	if (mysql_num_rows(result) == 0) {
		logmsg(DBG_ALWAYS,"Warning! No NASes found.");
		cfg.stayalive=false;
	}else{
		//Load each NAS
		while ((row = mysql_fetch_row(result)) != NULL) {
			thisnas = new C_NAS(row);
			verbose_mutex_lock(&list_mutex);
			naslist[thisnas->getFlowSrcPort()]=thisnas;
			verbose_mutex_unlock(&list_mutex);
			verbose_mutex_lock(&id_mutex);
			listById[thisnas->getId()]=thisnas;
			verbose_mutex_unlock(&id_mutex);
		};
	};
	mysql_free_result(result);
	mysql_close(sqllink);
	return naslist.size();
}

C_NAS* NASList::getbyport (uint16_t port){
    std::map<uint16_t,C_NAS*>::iterator nasit;
	nasit=naslist.find(port);
	logmsg(DBG_NETFLOW,"got iterator");
	C_NAS* mynas=nasit->second;
	if (mynas==NULL){
        logmsg(DBG_NETFLOW,"NULL pointer");
	    return NULL;
	};
	if (nasit->second->getFlowSrcPort()==port){
	    return mynas;
    }else{
        return NULL;
    };
}

C_NAS* NASList::getById (uint32_t id){
    std::map<uint32_t,C_NAS*>::iterator listByIdIt;
	listByIdIt=listById.find(id);
	if (listByIdIt->second->getId()==id){
	    return listByIdIt->second;
    }else{
        return NULL;
    };
}


void NASList::UserConnected(char* sessionid){
	logmsg(DBG_EVENTS,"Called connect function with session %s.",sessionid);
	//Connect to MySQL:
	MYSQL *sqllink = connectdb();
	//Create user, fill him with session data
	C_user* newuser = new C_user(sessionid,sqllink);
	//Load zones and groups
	newuser->load(sqllink);
	//Disconnect MySQL
	mysql_close(sqllink);
	//add user to NAS lists
	nases.getById(newuser->getNASId())->add_user(newuser);
	//add user to SID map
	verbose_mutex_lock(&userSidMutex);
	usersBySID[newuser->getSID()]=newuser;
	verbose_mutex_unlock(&userSidMutex);
};

void NASList::UserDisconnected(char* sessionid){
	logmsg(DBG_ALWAYS,"Called disconnect function with session %s.",sessionid);
	//Create user, fill him with session data:
	C_user* newuser = new C_user(sessionid);
	verbose_mutex_lock(&userSidMutex);
	C_user* myuser = usersBySID[newuser->getSID()];
	verbose_mutex_unlock(&userSidMutex);
	delete newuser;
	if (myuser!=NULL){
		myuser->userDisconnected();
	}


};
