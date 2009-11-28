#include "billing.h"


pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void logmsg ( uint8_t flags, const char* message, ...)
{
	//Check if logging category enabled:
	if ((flags & DBG_LOCKS && cfg.debug_locks) ||
		(flags & DBG_EVENTS && cfg.debug_events) ||
		(flags & DBG_NETFLOW && cfg.debug_netflow) ||
		(flags & DBG_OFFLOAD && cfg.debug_offload) ||
		(flags & DBG_DAEMON && cfg.verbose_daemonize) ||
		(flags & DBG_THREADS && cfg.debug_threads) ||
		(flags & DBG_HPSTAT && cfg.debug_hpstat) ||
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
		switch (flags) {
			case DBG_LOCKS:
				cout<< "Locks     :";
			break;
			case DBG_EVENTS:
				cout<< "Events    :";
			break;
			case DBG_NETFLOW:
				cout<< "NetFlow   :";
			break;
			case DBG_OFFLOAD:
				cout<< "Stats     :";
			break;
			case DBG_DAEMON:
				cout<< "Daemon    :";
			break;
			case DBG_THREADS:
				cout<< "Threads   :";
			break;
			case DBG_HPSTAT:
				cout<< "Det.stats :";
			break;
				cout<< "Debug     :";
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
	if (res==0)	logmsg(DBG_LOCKS,"lock and go %i",mutex);
	else{
		pthread_mutex_lock(mutex);
		logmsg(DBG_LOCKS,"lock and wait %i",mutex);
	};
	return res;
};

int verbose_mutex_unlock(pthread_mutex_t *mutex){
	int res=pthread_mutex_unlock(mutex);
	if (res==0) logmsg(DBG_LOCKS,"unlocked %i",mutex);
	else logmsg(DBG_ALWAYS,"was not locked %i",mutex);
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
	//delete all user's host-port data stats:
	fs_deleteTree(current_u->hostport_tree);
	logmsg(DBG_EVENTS,"Removing user %i, session %s.", current_u->id,current_u->verbose_session_id.c_str());
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
	/*char *addr = inet_ntoa(a);
	char *myaddr = new char[strlen(addr) + 1];
	strcpy(myaddr, addr);
	return myaddr;*///Memory leak is here, some not thread-safe solution:
	return inet_ntoa(a);
}

uint32_t ipFromStrToInt(const char *ipstr)
{
	in_addr a;
	inet_aton(ipstr,&a);
	return ntohl(a.s_addr);
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


//Class implementing locks for multiple reads && single write
rwLock::rwLock(void){
    pthread_mutex_init(&flagMutex, NULL);
    pthread_cond_init(&readDone, NULL);
    pthread_cond_init(&writeDone, NULL);
    writing = 0;
    readers = 0;
}

//Get read lock
uint32_t rwLock::lockRead(void){
    uint32_t myreaders;
    pthread_mutex_lock(&flagMutex);
    if (writing){//If writer is working, wait for it to finish.
        pthread_cond_wait(&writeDone,&flagMutex);
    };
    myreaders=++readers;
    pthread_mutex_unlock(&flagMutex);
    return myreaders;
}

//Release read lock
uint32_t rwLock::unlockRead(void){
    uint32_t myreaders;
    pthread_mutex_lock(&flagMutex);
    if (!(myreaders=--readers)){
        pthread_cond_broadcast(&readDone);
    };
    pthread_mutex_unlock(&flagMutex);
    return myreaders;
}

//Acquire write lock
void rwLock::lockWrite(void){
    pthread_mutex_lock(&flagMutex);
    while (readers>0) pthread_cond_wait(&readDone,&flagMutex);//Wait for all readers to finish
    while (writing) pthread_cond_wait(&writeDone,&flagMutex);//Wait if someone is writing
    writing = true;
    pthread_mutex_unlock(&flagMutex);
}

//Release write lock
void rwLock::unlockWrite(void){
    pthread_mutex_lock(&flagMutex);
    writing=false;
    pthread_cond_broadcast(&writeDone);
    pthread_mutex_unlock(&flagMutex);
}

//Destructor
rwLock::~rwLock(){
    pthread_cond_destroy(&writeDone);
    pthread_cond_destroy(&readDone);
    pthread_mutex_destroy(&flagMutex);

}

rwLock mylocker;
int secretval=0;
//Test for locking functions:
void testlocks(void){
    int ntr=50;//Create  threads
    pthread_t threads[ntr];
    printf("test locks\n");
    for (int i=0;i<ntr;i++){
        printf("create thread %i\n",i);
        pthread_create( &threads[ntr], NULL, &lockthread, &i );
    }
    logmsg(DBG_ALWAYS,"Main thread done,sleep some time\n");
    sleep(5);
    printf("\nSecret %i\n",secretval);
    exit(0);
};

//Test thread
void *lockthread (void *threadid){
    unsigned int thread = *(unsigned int*) threadid;
    int opcount=10;
    unsigned int rand = thread*time(NULL);
    int readers=0;
    bool written=false;
    printf("start loop\n");
    for (int i=0;i<opcount;i++){
        sched_yield();
        rand = rand_r(&rand);
        //printf("rand %i\n",rand);
        if (rand > RAND_MAX/10 || written) {
            // Read op
            readers = mylocker.lockRead();
            sched_yield();
            //std::cout << "'R" << thread << "v" << secretval << "c" << readers << "'" << endl;
            printf(":%i,%i",readers,secretval);

            mylocker.unlockRead();
        }else{
            //write op
            mylocker.lockWrite();
            sched_yield();
            secretval++;
            printf("W");
            usleep(rand/100000);
            //std::cout << "'W" << thread << "v" << ++secretval << "'" << endl;
            mylocker.unlockWrite();
            written=true;
        };
    };
    if (!written) {
        printf("Yahoo!\n");
    };
    pthread_exit(0);
};
