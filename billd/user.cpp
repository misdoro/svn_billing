#include "billing.h"

void C_user::getsession(char* sessionid){
	//Connect to MySQL
	MYSQL *sqllink = connectdb();
	MYSQL_ROW row;
	std::string query;
	char u_id [17];
	unique_session_id = strtoull(sessionid,NULL,16);
	sprintf(u_id,"%lx",unique_session_id);
	query = "SELECT id,user_id,user_name,nas_linkname,ppp_ip,sess_start,nas_id FROM sessions where session_id='";
	query.append(u_id);
	query.append("'");
	logmsg(DBG_EVENTS,query.c_str());
	MYSQL_RES *result;
	mysql_query(sqllink, query.c_str());
	result = mysql_store_result(sqllink);
	if (mysql_num_rows(result) == 0) {
		logmsg(DBG_ALWAYS,"Session %s not found",sessionid);
	}else if ((row = mysql_fetch_row(result)) != NULL) {
		session_id=atol(row[0]);
		id=atol(row[1]);
		nasLinkName=row[3];
		user_ip=atol(row[4]);
		nas_id=atol(row[6]);
		logmsg(DBG_EVENTS,"Connected user %s",row[2]);
	};
	mysql_free_result(result);
	mysql_close(sqllink);
}

void C_user::loadzones(){

}

C_user::C_user(char* sessionid){
	//Get session data
	getsession(sessionid);
	//Load zones

}

uint32_t C_user::getNASId(){
	return nas_id;
}

uint32_t C_user::getIP(){
	return user_ip;
}
