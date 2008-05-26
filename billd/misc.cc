#include "billing.h"


pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void logmsg ( uint8_t flags, char* message, ...)
{
	if ((flags & DBG_LOCKS && cfg.debug_locks) ||
		(flags & DBG_EVENTS && cfg.debug_events) ||
		(flags & DBG_NETFLOW && cfg.debug_netflow) ||
		(flags & DBG_OFFLOAD && cfg.debug_offload) ||
		(flags & DBG_DAEMON && cfg.verbose_daemonize) ||
		(flags & DBG_THREADS && cfg.debug_threads) ||
		(flags & DBG_ALWAYS))
	{
		pthread_mutex_lock(&log_mutex);
		if (cfg.log_date){
			time_t rawtime;
			time(&rawtime);
			char buf[50];
			strftime(buf,50, "%c", localtime ( &rawtime ));
			cout << buf << ": " ;
		};
		va_list args;
		va_start (args, message);
		vprintf (message, args);
		if (errno && errno!= EAGAIN){
			cout << " " << strerror(errno);
			errno=0;
		};
		cout << endl;
		va_end (args);
		pthread_mutex_unlock(&log_mutex);
		errno=0;
	};
}


// Get pointer to packet owner user:
user * getuserbyip(uint32_t psrcaddr, uint32_t pdstaddr, uint32_t pstarttime, uint32_t pendtime)
{
	for (user * p = firstuser; (p != NULL); p = p->next) {
		if (((psrcaddr == p->user_ip) || (pdstaddr == p->user_ip))
		 && pstarttime >= p->session_start_time
		 && (p->die_time==0 || pendtime <= p->die_time)) {
			return p;
		}
		logmsg(DBG_NETFLOW,"I%s sst%u pst%u set%u pet%u", ipFromIntToStr(p->user_ip),p->session_start_time,pstarttime,p->die_time,pendtime);
	}
	return NULL;
}

int verbose_mutex_lock(pthread_mutex_t *mutex){
	int res = pthread_mutex_trylock(mutex);
	if (res==0)	logmsg(DBG_LOCKS,"%s %i","lock and go ",mutex);
	else{
		pthread_mutex_lock(mutex);
		logmsg(DBG_LOCKS,"%s %i","lock and wait ",mutex);
	};
	return res;
};

int verbose_mutex_unlock(pthread_mutex_t *mutex){
	int res=pthread_mutex_unlock(mutex);
	if (res==0) logmsg(DBG_LOCKS,"%s %i","unlocked ",mutex);
	else logmsg(DBG_LOCKS,"%s %i","was not locked ",mutex);
	return res;
};

//shift given ip number by mask bits:
uint32_t mask_ip(uint32_t unmasked_ip, uint8_t mask)
{
	if (mask == 0) return 0;
	return unmasked_ip >> (32 - mask);
}

//Get pointer to matching zone:
user_zone * getflowzone(user * curr_user, uint32_t dst_ip,uint16_t dst_port)
{
	uint32_t dst_ip_masked;
	uint32_t zone_ip_masked;
	uint32_t sc=0;
	for (user_zone * p = curr_user->first_user_zone; (p != NULL); p = p->next) {
		dst_ip_masked = mask_ip(dst_ip, p->zone_mask);
		zone_ip_masked = mask_ip(p->zone_ip, p->zone_mask);
		if ((dst_ip_masked == zone_ip_masked) && ((p->zone_dstport==0)||(p->zone_dstport==dst_port))) {
			logmsg(DBG_NETFLOW,"Found zone (%i) for user %i (ip %s, port %i) in %i steps :( ",
			p->zone_dstport,curr_user->id,ipFromIntToStr(dst_ip),dst_port,sc);
			return p;
		}
		sc++;
	}
	logmsg(DBG_NETFLOW," zone not found for user %i (ip %s) in %i steps :( ",curr_user->id,ipFromIntToStr(dst_ip),sc);
	return NULL;
}

void addUser(user * u)
{
	if (firstuser == NULL) {
		firstuser = u;
		firstuser->next = NULL;
		return;
	}
	user *p;
	for (p = firstuser; (p->next != NULL); p = p->next);
	u->next = NULL;
	p->next = u;
}

void removeUser(user * current_u)
{
	pthread_mutex_t *this_user_mutex=&(current_u->user_mutex);
	verbose_mutex_lock (this_user_mutex);
	//free memory stored for zone_groups
	zone_group * previous_zone_group;
	for (zone_group * p = current_u->first_zone_group; p != NULL;) {
		previous_zone_group = p;
		p = p->next;
		delete previous_zone_group;
	}
	//free memory stored for user_zones
	user_zone * previous_zone;
	for (user_zone * p = current_u->first_user_zone; p != NULL;) {
		previous_zone = p;
		p = p->next;
		delete previous_zone;
	}
	logmsg(DBG_EVENTS,"Removing user %i...", current_u->id);
	//remove user
	if (firstuser == current_u) {
		user *previous_u = firstuser;
		firstuser = firstuser->next;
		delete previous_u;
		logmsg(DBG_EVENTS,"Removed first user");
	} else {
		for (user * u = firstuser; u != NULL;) {
			if (u->next == current_u){
				user *todelete_u = u->next;
				u->next = todelete_u->next;
				delete todelete_u;
				logmsg(DBG_EVENTS,"Removed subsequent user");
				break;
			}else{
				u = u->next;
			};
		}
	}
	verbose_mutex_unlock (this_user_mutex);
	pthread_mutex_destroy (this_user_mutex);
}

char *ipFromIntToStr(uint32_t ip)
{
	in_addr a;
	a.s_addr = htonl(ip);
	char *addr = inet_ntoa(a);
	char *myaddr = new char[strlen(addr) + 1];
	strcpy(myaddr, addr);
	return myaddr;
}

uint32_t ipFromStrToInt(const char *ipstr)
{
	in_addr a;
	inet_aton(ipstr,&a);
	return ntohl(a.s_addr);
}

user *onUserConnected(char *session_id, MYSQL * link)
{
	char sql[32768];
	MYSQL_RES *result;
	//get user info from session:
	sprintf(sql, "SELECT sessions.user_id, users.debit, users.credit, sessions.ppp_ip, sessions.id, sessions.nas_linkname from sessions,users where sessions.session_id='%s' and sessions.state=2 and users.id=sessions.user_id", session_id);
	mysql_query(link,sql);
	result = mysql_store_result(link);
	if (mysql_num_rows(result) == 0) {
		logmsg(DBG_EVENTS,"Warning! Session %s not found or is wrong", session_id);
		//can't drop user here, sorry. Don't know his NAS and link num
		return NULL;
	}
	MYSQL_ROW row = mysql_fetch_row(result);

	//create user structure
	user * newuser = new user;
	newuser->next = NULL;
	newuser->id = atoi(row[0]);
	newuser->user_debit = atof(row[1]);
	newuser->user_credit = atof(row[2]);
	newuser->user_ip = htonl(inet_addr(row[3]));
	newuser->session_id = atoi(row[4]);
	newuser->verbose_session_id = session_id;
	newuser->verbose_link_name = row[5];
	newuser->first_user_zone = NULL;
	newuser->first_zone_group = NULL;
	newuser->debit_changed = 0;
	newuser->session_start_time = time(NULL);
	newuser->session_end_time = 0;
	newuser->die_time = 0;
	pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
	newuser->user_mutex = mutex;
	newuser->user_drop_thread = 0;
	logmsg(DBG_EVENTS,"User info - id:%s, debit:%s, credit:%s", row[0], row[1], row[2]);
	mysql_free_result(result);
	//get user groups
	sprintf(sql, "SELECT usergroups.group_id,groupnames.mb_cost FROM usergroups,groupnames WHERE user_id=%i and usergroups.group_id=groupnames.id", newuser->id);
	mysql_query(link,sql);
	result = mysql_store_result(link);
	if (mysql_num_rows(result) == 0) {
		logmsg(DBG_EVENTS,"Warning! No groups for user %i found.", newuser->id);
		//drop user
		disconnect_user(newuser);
		delete newuser;
		return NULL;
	}
	while ((row = mysql_fetch_row(result)) != NULL) {
		//add zone groups
		zone_group * newgroup = new zone_group;
		newgroup->next = NULL;
		newgroup->id = atoi(row[0]);
		newgroup->in_bytes = 0;
		newgroup->out_bytes = 0;
		newgroup->zone_mb_cost = atof(row[1]);
		newgroup->in_diff = 0;
		newgroup->out_diff = 0;
		newgroup->group_changed = 0;
		if (newuser->first_zone_group == NULL) {
			newuser->first_zone_group = newgroup;
		} else {
			zone_group *p;
			for (p = newuser->first_zone_group; (p->next != NULL); p = p->next);
			p->next = newgroup;
		}

		logmsg(DBG_EVENTS,"User %i. Group %i", newuser->id, newgroup->id);
	}
	mysql_free_result(result);

	//load zones
	sprintf(sql, "select allzones.id, zone_groups.group_id, allzones.ip, allzones.mask, allzones.dstport, zone_groups.priority from allzones,zone_groups where allzones.id = zone_groups.zone_id and zone_groups.group_id in (select group_id from usergroups where user_id=%i) order by zone_groups.priority DESC;",newuser->id);
	mysql_query(link,sql);
	result = mysql_store_result(link);
	while ((row = mysql_fetch_row(result)) != NULL) {
		//create new zone
		user_zone * newzone = new user_zone;
		newzone->next = NULL;
		newzone->id = atoi(row[0]);
		//Create link to parent zone group
		uint32_t zone_group_id = atoi(row[1]);
		zone_group *p;
		for (p = newuser->first_zone_group; (p->next != NULL && p->id != zone_group_id); p = p->next);
		newzone->group_ref = p;

		newzone->zone_ip = atoll(row[2]);
		newzone->zone_mask = atoi(row[3]);
		newzone->zone_dstport = atoi(row[4]);
		newzone->zone_in_bytes = 0;
		newzone->zone_out_bytes = 0;
		logmsg(DBG_EVENTS,"Loaded zone. id=%i, zone_group_id=%i, ip=%s", newzone->id, (newzone->group_ref)->id, ipFromIntToStr(newzone->zone_ip));
		if (newuser->first_user_zone == NULL) {
			newuser->first_user_zone = newzone;
		} else {
			user_zone *p;
			for (p = newuser->first_user_zone; (p->next != NULL); p = p->next);
			p->next = newzone;
		}
	}
	mysql_free_result(result);
	logmsg(DBG_EVENTS,"Zones loaded.");
	//add user to list
	verbose_mutex_lock(&users_table_m);
	addUser(newuser);
	verbose_mutex_unlock(&users_table_m);
	logmsg(DBG_EVENTS,"User added.");
	return newuser;
}


void onUserDisconnected(char *session_id)
{
	user * current_u = NULL;
	uint8_t user_found = 0;

	verbose_mutex_lock(&users_table_m);
	for (current_u = firstuser; current_u != NULL; current_u = current_u->next) {
		if (strcmp(current_u->verbose_session_id.c_str(), session_id)==0) {
			user_found = 1;
			break;
		}
	}
	verbose_mutex_unlock(&users_table_m);

	if (user_found == 0) {
		logmsg(DBG_EVENTS,"Warning! Session %s not found!", session_id);
		return;
	}

	verbose_mutex_lock(&(current_u->user_mutex));
	current_u->die_time = time(NULL);
	current_u->session_end_time=time(NULL);
	verbose_mutex_unlock(&(current_u->user_mutex));
}

//Function to disconnect user from server, runs in another thread:
void * dropUser(void * userstr){
	user * drop_user;
	drop_user=(user *) userstr;
	verbose_mutex_lock(&(drop_user->user_mutex));
	uint32_t die_time = drop_user->die_time;
	string link_name = drop_user->verbose_link_name;
	verbose_mutex_unlock(&(drop_user->user_mutex));
	if (die_time == 0){
		//If user is still alive, connect to mpd and drop him:
		int socketDescriptor;
		struct sockaddr_in serverAddress;
		struct hostent *hostInfo;
		socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
		if (socketDescriptor < 0) {
			logmsg(DBG_EVENTS,"Error creating disconnect socket!");
		}else{
			hostInfo = gethostbyname(cfg.mpd_shell_addr.c_str());
			if (hostInfo == NULL) {
				logmsg(DBG_EVENTS,"Something is wrong with MPD address!");
			}else{
				serverAddress.sin_family = hostInfo->h_addrtype;
				memcpy((char *) &serverAddress.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
				serverAddress.sin_port = htons(cfg.mpd_shell_port);
				if (connect(socketDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
					logmsg(DBG_EVENTS,"cannot connect to MPD console");
				}else{
					stringstream buffer;
					sleep(1);
					buffer.str("");
					buffer << cfg.mpd_shell_user << endl;
					send(socketDescriptor, buffer.str().c_str(), strlen(buffer.str().c_str()) + 1, 0);
					sleep(1);
					buffer.str("");
                                        buffer << cfg.mpd_shell_pass << endl;
					send(socketDescriptor, buffer.str().c_str(), strlen(buffer.str().c_str()) + 1, 0);
					sleep(1);
					buffer.str("");
					buffer << "link " << link_name << endl;
					send(socketDescriptor, buffer.str().c_str(), strlen(buffer.str().c_str()) + 1, 0);
                                        sleep(1);
					buffer.str("");
                                        buffer << "close" << endl;
					send(socketDescriptor, buffer.str().c_str(), strlen(buffer.str().c_str()) + 1, 0);
					sleep(1);
					logmsg(DBG_EVENTS,"link %s disconnected",link_name.c_str());
				};
			};
			close(socketDescriptor);
		};
	};
	pthread_exit(NULL);
}

int disconnect_user (user * drophim){
        verbose_mutex_lock(&(drophim->user_mutex));
        int state=0;
	if (drophim->user_drop_thread==0 && drophim->die_time==0){
                pthread_create(&(drophim->user_drop_thread), NULL, dropUser, (void *) drophim);
        }else {
		state=1;
	};
        verbose_mutex_unlock(&(drophim->user_mutex));
	return state;
};
