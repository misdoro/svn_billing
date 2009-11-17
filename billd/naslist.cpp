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
	nasit=naslist.find(port);
	return nasit->second;
}

C_NAS* NASList::getById (uint32_t id){
	listByIdIt=listById.find(id);
	return listByIdIt->second;
}


void NASList::UserConnected(char* sessionid){
	logmsg(DBG_EVENTS,"Called connect function with session %s.",sessionid);
	//Get session data
	C_user* newuser = new C_user(sessionid);
	//add user to NAS lists
	C_NAS* hisnas = nases.getById(newuser->getNASId());
	hisnas->add_user(newuser);

};

void NASList::UserDisconnected(char* sessionid){
	logmsg(DBG_ALWAYS,"Called disconnect function with session %s.",sessionid);
};
