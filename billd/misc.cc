#include "billing.h"

// Get pointer to packet owner user:
user * getuserbyip(uint32_t psrcaddr, uint32_t pdstaddr)
{
	for (user * p = firstuser; (p != NULL); p = p->next) {
		if ((psrcaddr == p->user_ip) || (pdstaddr == p->user_ip)) {
			return p;
		}
	}
	return NULL;
}

int verbose_mutex_lock(pthread_mutex_t *mutex){
	int res = pthread_mutex_trylock(mutex);
	if (res==0){
		if (cfg.debug_locks) printf("lock and go!\n");
	}else{	
		pthread_mutex_lock(mutex);
		if (cfg.debug_locks) printf("lock and wait!\n");
	};
	return res;
};

int verbose_mutex_unlock(pthread_mutex_t *mutex){
	int res=pthread_mutex_unlock(mutex);
	if (res==0){
		if (cfg.debug_locks) printf("unlocked!\n");
	}else {
		if (cfg.debug_locks) printf("was not locked!\n");
	};
	return res;
};

//shift given ip number by mask bits:
uint32_t mask_ip(uint32_t unmasked_ip, uint8_t mask)
{
	if (mask == 0)
		return 0;
	return unmasked_ip >> (32 - mask);
}

//Get pointer to matching zone:
user_zone * getflowzone(user * curr_user, uint32_t dst_ip)
{
	uint32_t dst_ip_masked;
	uint32_t zone_ip_masked;
	for (user_zone * p = curr_user->first_user_zone; (p != NULL); p = p->next) {
		dst_ip_masked = mask_ip(dst_ip, p->zone_mask);
		zone_ip_masked = mask_ip(p->zone_ip, p->zone_mask);
		if (dst_ip_masked == zone_ip_masked) {
			return p;
		}
	}
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
	printf("Removing user %i...", current_u->id);
	//remove user
	if (firstuser == current_u) {
		user *previous_u = firstuser;
		firstuser = firstuser->next;
		delete previous_u;
		printf("OK\n");
	} else {
		for (user * u = firstuser; u != NULL;) {
			if (u->next == current_u){
				user *todelete_u = u->next;
				u->next = todelete_u->next;
				delete todelete_u;
                                printf("OK\n");
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

void makeDBready()
{
	/*
	char *sql;
	sql = new char[1024];
	sprintf(sql, "UPDATE statistics SET connected = 0");
	verbose_mutex_lock (&mysql_mutex);
	mysql_query(cfg.myconn, sql);
	verbose_mutex_unlock (&mysql_mutex);
	delete sql;
	*/
}

user *onUserConnected(char *session_id)
{
	char sql[32768];
	MYSQL_RES *result;
//get user info from session:
	sprintf(sql, "SELECT sessions.user_id, users.debit, users.credit, sessions.ppp_ip, sessions.id, sessions.nas_linkname from sessions,users where sessions.session_id='%s' and sessions.state=2 and users.id=sessions.user_id", session_id);
	//sprintf(sql, "SELECT id, debit, credit, user_ip FROM users WHERE login='%s' LIMIT 1", username);
	verbose_mutex_lock (&mysql_mutex);
	mysql_query(cfg.myconn, sql);
	result = mysql_store_result(cfg.myconn);
	if (mysql_num_rows(result) == 0) {
		printf("Warning! Session %s not found or is wrong\n", session_id);
		//!!!-drop user here
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
	newuser->die_time = 0;
	newuser->user_mutex = PTHREAD_MUTEX_INITIALIZER;
	newuser->user_drop_thread = 0;
	printf("User info - id:%s, debit:%s, credit:%s\n", row[0], row[1], row[2]);
	mysql_free_result(result);
	verbose_mutex_unlock (&mysql_mutex);
	//get user groups
	sprintf(sql, "SELECT usergroups.group_id,groupnames.mb_cost FROM usergroups,groupnames WHERE user_id=%i and usergroups.group_id=groupnames.id LIMIT 128", newuser->id);
	verbose_mutex_lock (&mysql_mutex);
	mysql_query(cfg.myconn, sql);
	result = mysql_store_result(cfg.myconn);
	if (mysql_num_rows(result) == 0) {
		printf("Warning! No groups for user %i found.\n", newuser->id);
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

		printf("User %i. Group %i\n", newuser->id, newgroup->id);
	}
	mysql_free_result(result);
	verbose_mutex_unlock (&mysql_mutex);

	//load zones - create sql query
	/*sprintf(sql, "SELECT allzones.id, zone_groups.group_id  AS group_id, allzones.ip, allzones.mask, allzones.dstport, (SELECT mb_cost FROM groupnames WHERE group_id = groupnames.id) AS mb_cost FROM allzones INNER JOIN zone_groups ON zone_groups.zone_id = allzones.id WHERE ");
	for (zone_group * p = newuser->first_zone_group; (p != NULL); p = p->next) {
		sprintf(sql, "%s (zone_groups.group_id = %i)", sql, p->id);
		if (p->next != NULL)
			sprintf(sql, "%s OR ", sql);
	}
	sprintf(sql, "%s ORDER BY zone_groups.priority DESC", sql);
	*/
	sprintf(sql, "select allzones.id, zone_groups.group_id, allzones.ip, allzones.mask, allzones.dstport, zone_groups.priority from allzones,zone_groups where allzones.id = zone_groups.zone_id and zone_groups.group_id in (select group_id from usergroups where user_id=%i) order by zone_groups.priority DESC;",newuser->id);
	verbose_mutex_lock (&mysql_mutex);
	mysql_query(cfg.myconn, sql);
	result = mysql_store_result(cfg.myconn);
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
		printf("Loaded zone. id=%i, zone_group_id=%i, ip=%s\n", newzone->id, (newzone->group_ref)->id, ipFromIntToStr(newzone->zone_ip));
		if (newuser->first_user_zone == NULL) {
			newuser->first_user_zone = newzone;
		} else {
			user_zone *p;
			for (p = newuser->first_user_zone; (p->next != NULL); p = p->next);
			p->next = newzone;
		}
	}
	mysql_free_result(result);
	verbose_mutex_unlock (&mysql_mutex);
	printf("Zones loaded.\n");
	//add user to list
	verbose_mutex_lock(&users_table_m);
	addUser(newuser);
	verbose_mutex_unlock(&users_table_m);
	printf("User added.\n");
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
		printf("Warning! Session %s not found!\n", session_id);
		return;
	}

	verbose_mutex_lock(&(current_u->user_mutex));
	current_u->die_time = time(NULL);
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
			printf("Error creating socket!\n");
		}else{
			hostInfo = gethostbyname(cfg.mpd_shell_addr.c_str());
			if (hostInfo == NULL) {
				printf("Something is wrong with address!\n");
			}else{
				serverAddress.sin_family = hostInfo->h_addrtype;
				memcpy((char *) &serverAddress.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
				serverAddress.sin_port = htons(cfg.mpd_shell_port);
				if (connect(socketDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
					printf("cannot connect\n");
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
					cout << "link " << link_name << " disconnected" << endl;
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
