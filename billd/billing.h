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

#include "Config.h"

#include "tree.h"
#include "rwLock.h"
#include "threads.h"

#include "user.h"
#include "nas.h"
#include "naslist.h"

#include "Config.h"

#include "tree.h"
#include "rwLock.h"


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

#define DBG_LOCKS 1
#define DBG_NETFLOW 2
#define DBG_OFFLOAD 4
#define DBG_EVENTS 8
#define DBG_DAEMON 16
#define DBG_THREADS	32
#define DBG_HPSTAT 64
#define DBG_ALWAYS 128

extern Config cfg;
extern NASList nases;

//userconnect.cc
void *userconnectlistener(void *threadid);
//netflow.cc
void *netflowlistener(void *threadid);

MYSQL *connectdb();
//misc.cc
int verbose_mutex_lock(pthread_mutex_t *mutex);
int verbose_mutex_unlock(pthread_mutex_t *mutex);
char *ipFromIntToStr(uint32_t ip);
void logmsg ( uint8_t flags, const char* message, ...);



int daemonize(void);

#endif				/* _BILLING_H */
