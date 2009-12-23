//
//	Class for loading and manipulating NASes
//

#include "billing.h"


//Constructor. Load NASes from MYSQL
NASList::NASList (){
    pthread_mutex_init(&list_mutex,NULL);
    pthread_mutex_init(&id_mutex,NULL);
	pthread_mutex_init(&userSidMutex,NULL);
}

NASList::~NASList(){
    std::map<uint16_t,C_NAS*>::iterator nasit;
    C_NAS* thisnas;
    for(nasit=naslist.begin();nasit!=naslist.end();nasit++){
        thisnas = nasit->second;
        if (thisnas!=NULL) delete thisnas;
    };
    pthread_mutex_destroy(&list_mutex);
    pthread_mutex_destroy(&id_mutex);
    pthread_mutex_destroy(&userSidMutex);
}

uint32_t NASList::load(){
	//Connect to MySQL
	MYSQL *sqllink = connectdb();
	MYSQL_ROW row;

	C_NAS* thisnas;

	logmsg(DBG_ALWAYS,"Loading NASes");

	//Load all NASes:
	std::string query;
	query = "SELECT id,ip,port,identifier,shell_host,shell_port,shell_user,shell_password FROM naslist;";
	MYSQL_RES *result;
	mysql_query(sqllink, query.c_str());
	result = mysql_store_result(sqllink);
	if (mysql_num_rows(result) == 0) {
		logmsg(DBG_ALWAYS,"ERROR: No NASes found!");
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
	C_NAS* mynas=nasit->second;
	if (mynas==NULL){
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
}

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
}

//Delete disconnected user from SID map.
void NASList::UserDeleted(C_user* myUser){
    verbose_mutex_lock(&userSidMutex);
	usersBySID.erase(myUser->getSID());
	verbose_mutex_unlock(&userSidMutex);
}


