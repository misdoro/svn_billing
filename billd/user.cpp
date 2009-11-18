#include "billing.h"

C_user::C_user(char* sessionid){
	//Connect to MySQL
	MYSQL *sqllink = connectdb();
	//Get session data
	getsession(sessionid,sqllink);
	//Disconnect MySQL
	mysql_close(sqllink);
}

C_user::C_user(char* sessionid,MYSQL *sqllink){
	//Get session data
	getsession(sessionid,sqllink);
}

void C_user::load(){
	//Connect to MySQL
	MYSQL *sqllink = connectdb();
	//Load zone groups
	loadgroups(sqllink);
	//Load zones
	loadzones(sqllink);
	//Disconnect MySQL
	mysql_close(sqllink);
	//Init mutexes;
	init_mutex();
}

void C_user::load(MYSQL *sqllink){
	//Load zone groups
	loadgroups(sqllink);
	//Load zones
	loadzones(sqllink);
	//Init mutexes
	init_mutex();
}

void C_user::init_mutex(){
	pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
	user_mutex=mutex;
};


uint32_t C_user::getNASId(){
	return nas_id;
}

uint32_t C_user::getIP(){
	return user_ip;
}

uint32_t C_user::getSID(){
	return session_id;
}

//Update user's group according to zone:
bool C_user::updateTraffic(uint32_t dst_ip,uint16_t dst_port, uint32_t bytecount,int8_t flow_direction){

	return false;
};

//Get users's data from stored session:
void C_user::getsession(char* sessionid,MYSQL *sqllink){

	MYSQL_ROW row;
	MYSQL_RES *result;

	std::string query;
	char u_id [17];
	unique_session_id = strtoull(sessionid,NULL,16);
	sprintf(u_id,"%.16lx",unique_session_id);
	query = "SELECT id,user_id,user_name,nas_linkname,ppp_ip,sess_start,nas_id FROM sessions where session_id='";
	query.append(u_id);
	query.append("'");
	logmsg(DBG_EVENTS,query.c_str());
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

}

//Get user's groups
void C_user::loadgroups(MYSQL *sqllink){
	MYSQL_ROW row;
	MYSQL_RES *result;

	char sql[200];
	sprintf(sql, "SELECT usergroups.group_id,groupnames.mb_cost FROM usergroups,groupnames WHERE user_id=%i and usergroups.group_id=groupnames.id", id);
	mysql_query(sqllink,sql);
	result = mysql_store_result(sqllink);
	if (mysql_num_rows(result) == 0) {
		logmsg(DBG_EVENTS,"Warning! No groups for user %i found.", id);
		//drop user here!

	}
	while ((row = mysql_fetch_row(result)) != NULL) {
		//add zone groups
		zone_group * newgroup = new zone_group;
		newgroup->id = atoi(row[0]);
		newgroup->in_bytes = 0;
		newgroup->out_bytes = 0;
		newgroup->zone_mb_cost = atof(row[1]);
		newgroup->in_diff = 0;
		newgroup->out_diff = 0;
		newgroup->group_changed = 0;
		//groups.insert(pair<uint>)
		groups[newgroup->id] = newgroup;
		logmsg(DBG_EVENTS,"Group %i", newgroup->id);
	}
	mysql_free_result(result);
}
//Get user's zones
void C_user::loadzones(MYSQL *sqllink){
	MYSQL_ROW row;
	MYSQL_RES *result;

	char sql[400];
	sprintf(sql, "select allzones.id, zone_groups.group_id, allzones.ip, allzones.mask, allzones.dstport, zone_groups.priority from allzones,zone_groups where allzones.id = zone_groups.zone_id and zone_groups.group_id in (select group_id from usergroups where user_id=%i) order by zone_groups.priority DESC;",id);
	mysql_query(sqllink,sql);
	result = mysql_store_result(sqllink);
	while ((row = mysql_fetch_row(result)) != NULL) {
		//create new zone
		zone * newzone = new zone;
		newzone->id = atoi(row[0]);
		//Create link to parent zone group
		uint32_t zone_group_id = atoi(row[1]);
		newzone->group_ref = groups[zone_group_id];
		newzone->zone_ip = strtoul(row[2],NULL,10);
		newzone->zone_mask = atoi(row[3]);
		newzone->zone_dstport = atoi(row[4]);
		zones.push_back(newzone);
		logmsg(DBG_EVENTS,"Loaded zone. id=%i, zone_group_id=%i, ip=%s", newzone->id, (newzone->group_ref)->id, ipFromIntToStr(newzone->zone_ip));
	}
	mysql_free_result(result);
	logmsg(DBG_EVENTS,"Zones loaded.");

}

void C_user::userDisconnected(){
	verbose_mutex_lock(&user_mutex);
	die_time = time(NULL);
	session_end_time=time(NULL);
	verbose_mutex_unlock(&user_mutex);
}
