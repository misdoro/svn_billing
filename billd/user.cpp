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

//Destructor, needed to properly clean up memory
C_user::~C_user(){
    //Delete users zonegroups, zones
    std::list<zone*>::iterator zonesiter;
    zone* ptr;
    for(zonesiter = zones.begin();zonesiter!=zones.end();zonesiter++){
        ptr=*zonesiter;
        if (ptr!=NULL){
            delete ptr;
        };

    };
    std::map<uint32_t,zone_group*>::iterator groupIt;
    zone_group* group;
    for(groupIt = this->groups.begin();groupIt!=this->groups.end();groupIt++){
        group=groupIt->second;
        if (group!=NULL){
            delete group;
        };
    };
    //Wait for disconnect thread if any
    tryJoin();
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
	pthread_mutex_init(&user_mutex,NULL);
}

void C_user::setNAS(C_NAS* caller){
    myNAS=caller;
}

uint32_t C_user::getNASId(){
	return nas_id;
}

uint32_t C_user::getIP(){
	return user_ip;
}

uint32_t C_user::getSID(){
	return session_id;
}


uint32_t C_user::mask_ip(uint32_t ip, uint8_t mask){
    uint32_t masked;
    if (mask) masked = 0xFFFFFFFF << (32-mask);
    else masked=0;
    return (ip&masked);
}

void* C_user::getFlowZone(uint32_t dst_ip,uint16_t dst_port){
    //Traverse through zones, find matching:
    std::list<zone*>::iterator zonesiter;
    zone* ptr;
    for(zonesiter = zones.begin();zonesiter!=zones.end();zonesiter++){
        ptr=*zonesiter;
        if ((ptr->zone_ip == mask_ip(dst_ip,ptr->zone_mask)) && ((ptr->zone_dstport==0)||(ptr->zone_dstport==dst_port))){
            return (void*)ptr;
        };
    };
    return NULL;
}

//Update group traffic
void C_user::updateGroupTraffic(zone_group* myzgroup, uint32_t bytecount, int8_t flow_dir ){
    zgstatlock.lockWrite();
    if (flow_dir){
        myzgroup->in_bytes+=bytecount;
        myzgroup->in_diff+=bytecount;
        myzgroup->group_changed=1;
    }else{
        myzgroup->out_bytes+=bytecount;
        myzgroup->out_diff+=bytecount;
        myzgroup->group_changed=1;
    };
    zgstatlock.unlockWrite();
}

//Update user's group according to zone:
uint32_t C_user::updateTraffic(uint32_t dst_ip,uint16_t dst_port, uint32_t bytecount,int8_t flow_direction){
    zone* myzone = (zone*) getFlowZone(dst_ip,dst_port);
    if (myzone == NULL) return 0;
    else{//Got zone!
        updateGroupTraffic(myzone->group_ref,bytecount,flow_direction);
    };
    return myzone->group_ref->id;

}

//Update user stats in MySQL:
void C_user::updateStats(MYSQL *sqllink){
    double chargedMoney=0;
    double chargedMoneyZ=0;
    char query[512];

    //Iterate through his groups:
    std::map<uint32_t,zone_group*>::iterator groupIt;
    zone_group* group;
    for(groupIt = this->groups.begin();groupIt!=this->groups.end();groupIt++){
        group=groupIt->second;
        //Check if group changed
        zgstatlock.lockRead();//Mind locks!
        if (group->group_changed){
            zgstatlock.unlockRead();
            //Clear delta stats:
            zgstatlock.lockWrite();
            uint64_t inDiff=group->in_diff;
            uint64_t outDiff=group->out_diff;
            group->in_diff=0;
            group->out_diff=0;
            group->group_changed=0;
            zgstatlock.unlockWrite();

            logmsg(DBG_OFFLOAD,"group %i: in %lu out %lu ",group->id, inDiff,outDiff);
            chargedMoneyZ = (double) inDiff * group->zone_mb_cost / (double) MB_LENGTH;

            //Try to update pack with this group ID:
            sprintf(query, "UPDATE userpacks SET units_left = units_left - %lu WHERE user_id=%u AND unittype=1 AND date_expire > CURRENT_TIMESTAMP AND unit_zone=%u AND units_left>%lu ORDER BY date_on ASC LIMIT 1",inDiff,this->bill_id,group->id,inDiff);
            mysql_query(sqllink, query);
            //If no packs updated, charge money :)
            if (mysql_affected_rows(sqllink)!=1) chargedMoney += chargedMoneyZ;

            //Add stats record:
            sprintf(query, "insert into session_statistics (zone_group_id,session_id,traf_in,traf_out,traf_in_money) values (%i,%i,%lu,%lu,%f);",group->id,this->session_id,inDiff,outDiff,chargedMoneyZ);
            mysql_query(sqllink, query);

        }else zgstatlock.unlockRead();//Mind locks!
    };
    //Update user's debit:
    if (chargedMoney>0){
        sprintf(query, "UPDATE users SET debit=debit-%f WHERE id=%u", chargedMoney,this->bill_id);
		mysql_query(sqllink, query);
    };

    //Update user's 'last active' field:
    sprintf(query, "UPDATE users SET last_active = CURRENT_TIMESTAMP() where id=%u",this->id);
    mysql_query(sqllink, query);

}

//Check user's debit, return true if he has enough money
bool C_user::checkDebit(MYSQL *sqllink){
    MYSQL_RES *result;
    char query[200];
    sprintf(query,"SELECT users.debit+users.credit from users where users.id=%i", this->bill_id);
    mysql_query(sqllink,query);
    result = mysql_store_result(sqllink);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (row[0]==NULL){//If can't get debit, drop user.
        logmsg(DBG_EVENTS,"Cant get user's debit for sid %u, drop him!",this->session_id);
        this->start();
        return false;
    };
    double money=atof(row[0]);

    mysql_free_result(result);
    return (money>0);
}

//User disconnect method, called when debit < 0;
void C_user::dropUser(){
    this->start();//Start runThread to drop user
}
void C_user::runThread(){
    logmsg(DBG_EVENTS,"Going to disconnect link %s",this->nasLinkName.c_str());
    if (this->session_end_time == 0){
        //If user is still alive, connect to mpd and drop him:
        int socketDescriptor;
        sockaddr_in mpd_shell_addr = myNAS->getShellAddr();
        socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
        if (socketDescriptor < 0) {
            logmsg(DBG_EVENTS,"Error creating disconnect socket!");
        }else{
			if (connect(socketDescriptor, (struct sockaddr *) &mpd_shell_addr, sizeof(mpd_shell_addr)) < 0) {
				logmsg(DBG_EVENTS,"cannot connect to MPD console");
            }else{
				stringstream buffer;
				sleep(1);
				buffer.str("");
				buffer << myNAS->getShellLogin() << endl;
				send(socketDescriptor, buffer.str().c_str(), strlen(buffer.str().c_str()) + 1, 0);
				sleep(1);
				buffer.str("");
                buffer << myNAS->getShellPassword() << endl;
				send(socketDescriptor, buffer.str().c_str(), strlen(buffer.str().c_str()) + 1, 0);
				sleep(1);
				buffer.str("");
				buffer << "link " <<this->nasLinkName << endl;
				send(socketDescriptor, buffer.str().c_str(), strlen(buffer.str().c_str()) + 1, 0);
                sleep(1);
				buffer.str("");
				buffer << "close" << endl;
				send(socketDescriptor, buffer.str().c_str(), strlen(buffer.str().c_str()) + 1, 0);
				sleep(1);
				logmsg(DBG_EVENTS,"link %s disconnected", this->nasLinkName.c_str());
			};
			close(socketDescriptor);
		};
	};
	pthread_exit(NULL);
};

//Get users's data from stored session:
void C_user::getsession(char* sessionid,MYSQL *sqllink){

	MYSQL_ROW row;
	MYSQL_RES *result;

	std::string query;
	char u_id [17];
	unique_session_id = strtoull(sessionid,NULL,16);
	sprintf(u_id,"%.16lx",unique_session_id);
	query = "SELECT s.id,s.user_id,s.user_name,s.nas_linkname,s.ppp_ip,UNIX_TIMESTAMP(s.sess_start),s.nas_id,u.parent FROM sessions as s, users as u WHERE u.id=s.user_id AND session_id='";
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
		session_start_time=atol(row[5]);
		session_end_time=0;
		nas_id=atol(row[6]);
        if (row[7]==NULL) bill_id=id;
		else bill_id=atol(row[7]);
		if (bill_id==0) bill_id=id;
		logmsg(DBG_EVENTS,"Connected user %s",row[2]);
	};
	mysql_free_result(result);


}

//Get user's groups
void C_user::loadgroups(MYSQL *sqllink){
	MYSQL_ROW row;
	MYSQL_RES *result;

	char sql[200];
	//sprintf(sql, "SELECT usergroups.group_id,groupnames.mb_cost FROM usergroups,groupnames WHERE user_id=%i and usergroups.group_id=groupnames.id", id);
	sprintf(sql,"SELECT price_groups.group_id,price_groups.mb_cost FROM price_groups,users WHERE price_groups.price_id=users.active_price AND users.id=%u",this->id);
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
	//sprintf(sql, "select allzones.id, zone_groups.group_id, allzones.ip, allzones.mask, allzones.dstport, zone_groups.priority from allzones,zone_groups where allzones.id = zone_groups.zone_id and zone_groups.group_id in (select group_id from usergroups where user_id=%i) order by zone_groups.priority DESC;",id);
	sprintf(sql, "SELECT az.id, zg.group_id, az.ip, az.mask, az.dstport, zg.priority FROM allzones AS az,zone_groups AS zg WHERE az.id = zg.zone_id AND zg.group_id IN (SELECT pg.group_id FROM price_groups AS pg,users AS u WHERE pg.price_id=u.active_price AND u.id=%u) ORDER BY zg.priority DESC;",this->id);
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
		logmsg(DBG_EVENTS,"Loaded zone. id=%i, zone_group_id=%i, ip %s mask %i", newzone->id, (newzone->group_ref)->id, ipFromIntToStr(newzone->zone_ip), newzone->zone_mask);
	}
	mysql_free_result(result);
	logmsg(DBG_EVENTS,"Zones loaded.");

}

//Check if flow time matches session interval:
bool C_user::checkFlowTimes(uint32_t flow_start, uint32_t flow_end){
    //logmsg(DBG_NETFLOW,"s_s:%i, s_e %i, f_s%i, f_e%i",session_start_time,session_end_time,flow_start,flow_end);
    bool result=(session_start_time < flow_start
        && ((session_end_time == 0)
            || (session_end_time > flow_end))
        );
    return result;
}

void C_user::userDisconnected(){
	verbose_mutex_lock(&user_mutex);
	session_end_time=time(NULL);
	verbose_mutex_unlock(&user_mutex);
}

//Check if user is ready to be deleted.
bool C_user::checkDelete(void){
    if (((time(NULL) - this->session_end_time ) > cfg.die_time_interval)&& this->session_end_time !=0) {
        return true;
    }else return false;
}
