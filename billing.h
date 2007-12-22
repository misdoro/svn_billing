/* 
 * File:   billing.h
 * Author: flexx
 *
 * Created on 18 Декабрь 2007 г., 20:08
 */

#ifndef _BILLING_H
#define	_BILLING_H

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
//#include <sys/endian.h>

#include <signal.h>
#include <sys/time.h>

#include <math.h>
#include <mysql/mysql.h>

#define MB_LENGTH 1048576

// netflow structures
typedef struct pheader {
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

typedef struct flowrecord {
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

// billing structures
typedef struct user_zone {
        user_zone * next;
        uint32_t id;
        uint32_t zone_group_id;
        
        uint32_t zone_ip;
        uint8_t zone_mask;
        uint16_t zone_dstport;
	double zone_mb_cost;
	uint64_t zone_in_bytes;
	uint64_t zone_out_bytes;
};

typedef struct zone_group {
        zone_group * next;
        uint32_t id;
	uint64_t in_bytes;
	uint64_t out_bytes;
        bool group_changed;
        double in_mb_cost_total;
};

typedef struct user {
	user *next;
	uint32_t id;
        uint32_t user_ip;        
//        uint32_t user_ip_digit;
	double user_debit;
        double user_kredit;
	uint32_t session_id;
	uint32_t session_start_time;
	uint32_t session_end_time;
        bool debit_changed;
	user_zone * first_user_zone;
        zone_group * first_zone_group;
};

typedef struct configuration {
	uint32_t netflow_port;
        char * mysql_server;
        uint32_t mysql_port;
        char * mysql_username;
        char * mysql_password;
        char * mysql_database;
        MYSQL * myconn;
	bool terminate;
        uint32_t stats_updated_time;
        uint32_t stats_update_interval;
};
    
extern user * firstuser;
extern configuration cfg;
extern pthread_mutex_t users_table_m;

void err_func(char *msg); // billing.cc
void * userconnectlistener (void *threadid); 
void * netflowlistener(void *threadid); // netflow.cc

user * getuserbyip(uint32_t psrcaddr, uint32_t pdstaddr);
uint32_t mask_ip(uint32_t unmasked_ip, uint8_t mask);
user_zone * getflowzone(user * curr_user, uint32_t dst_ip);
MYSQL * connectdb ();

char * ipFromIntToStr(uint32_t ip);
user * onUserConnected(uint32_t dst_ip, uint32_t real_ip);
void makeDBready();
#endif	/* _BILLING_H */
