/*
 * File:   billing.h authors: flexx, misdoro
 *
 */

#ifndef _BILLING_H
#define	_BILLING_H

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <list>

#include <math.h>
#include <mysql/mysql.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>	//uid_t etc
#include <set>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef linux
#include <endian.h>
#elif defined(__FreeBSD__)
#include <machine/endian.h>
#endif
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include <limits.h> //strtoull

#include "user.h"
#include "nas.h"
#include "naslist.h"

#include "Config.h"

#include "tree.h"

//#include "user.h"
#include "nas.h"
#include "naslist.h"

#define MB_LENGTH 1048576

using namespace std;

//netflow structures
struct pheader {
	uint16_t pver;
	uint16_t nflows;
	uint32_t uptime;
	uint32_t time;
	uint32_t ntime;
	uint32_t seq;
	uint8_t etype;
	uint8_t eid;
	uint16_t pad;
};

struct flowrecord {
	uint32_t srcaddr;
	uint32_t dstaddr;
	uint32_t nexthop;
	uint16_t in_if;
	uint16_t out_if;
	uint32_t pktcount;
	uint32_t bytecount;
	uint32_t starttime;
	uint32_t endtime;
	uint16_t srcport;
	uint16_t dstport;
	uint8_t pad1;
	uint8_t tcpflags;
	uint8_t ipproto;
	uint8_t tos;
	uint16_t src_as;
	uint16_t dst_as;
	uint8_t src_mask;
	uint8_t dst_mask;
	uint16_t pad2;
};

//billing structures
struct zone_group {
	zone_group *next;
	uint32_t id;
	uint64_t in_bytes;
	uint64_t out_bytes;
	bool group_changed;
	uint64_t in_diff;
	uint64_t out_diff;
	double zone_mb_cost;
};

struct user_zone {
	user_zone *next;
	zone_group *group_ref;
	uint32_t id;
//	uint32_t zone_group_id;
	uint32_t zone_ip;
	uint8_t zone_mask;
	uint16_t zone_dstport;
	uint64_t zone_in_bytes;
	uint64_t zone_out_bytes;
};

struct user {
	user *next;
	uint32_t id;
	uint32_t bill_id;
	uint32_t user_ip;
	double user_debit;
	double user_debit_diff;
	double user_credit;
	uint32_t session_id;
	string verbose_session_id;
	string verbose_link_name;
	uint32_t session_start_time;
	uint32_t session_end_time;
	uint32_t die_time;
	bool debit_changed;
	user_zone *first_user_zone;
	zone_group *first_zone_group;
	pthread_mutex_t user_mutex;
	pthread_t user_drop_thread;
	host_node *hostport_tree;
};

class rwLock {
    private:

    uint32_t readers;
    uint8_t writing;
    pthread_mutex_t flagMutex;
    pthread_cond_t writeDone;
    pthread_cond_t readDone;

    public:
    rwLock(void);
    ~rwLock(void);
    uint32_t lockRead(void);
    uint32_t unlockRead(void);
    void lockWrite(void);
    void unlockWrite(void);

};

#define DBG_LOCKS 1
#define DBG_NETFLOW 2
#define DBG_OFFLOAD 4
#define DBG_EVENTS 8
#define DBG_DAEMON 16
#define DBG_THREADS	32
#define DBG_HPSTAT 64
#define DBG_ALWAYS 128

extern user *firstuser;
extern Config cfg;
extern NASList nases;
extern pthread_mutex_t users_table_m;
extern pthread_mutex_t mysql_mutex;


//userconnect.cc
void *userconnectlistener(void *threadid);
//netflow.cc
void *netflowlistener(void *threadid);
//mysql.cc
void *statsupdater(void *threadid);
MYSQL *connectdb();
//misc.cc
void *dropUser(void * userstr);
int disconnect_user (user * drophim);
int verbose_mutex_lock(pthread_mutex_t *mutex);
int verbose_mutex_unlock(pthread_mutex_t *mutex);
char *ipFromIntToStr(uint32_t ip);
uint32_t ipFromStrToInt(const char *ipstr);
user *onUserConnected(char *session_id, MYSQL *link);
void onUserDisconnected(char *session_id);
void removeUser(user * current_u);
void logmsg ( uint8_t flags, const char* message, ...);
user * getuserbyip(uint32_t psrcaddr, uint32_t pdstaddr , uint32_t pstarttime, uint32_t pendtime);
uint32_t mask_ip(uint32_t unmasked_ip, uint8_t mask);
user_zone * getflowzone(user * curr_user, uint32_t dst_ip,uint16_t dst_port);
//daemonize.cc
int daemonize(void);
void *lockthread (void *threadid);
void testlocks(void);


#endif				/* _BILLING_H */
