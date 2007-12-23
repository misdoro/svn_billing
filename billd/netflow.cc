#include "billing.h"

void fillhdr(pheader *phead,char *buf) {
    //Fill in packet struct and swap byteorder where and if needed:
    memcpy (&(phead->pver),buf,2);
    memcpy (&(phead->nflows),buf+2,2);
    memcpy (&(phead->uptime),buf+4,4);
    memcpy (&(phead->time),buf+8,4);
    memcpy (&(phead->ntime),buf+12,4);
    memcpy (&(phead->seq),buf+16,4);
    memcpy (&(phead->etype),buf+20,1);
    memcpy (&(phead->eid),buf+21,1);
    memcpy (&(phead->pad),buf+22,2);

    phead->pver=ntohs(phead->pver);
    phead->nflows=ntohs(phead->nflows);
    phead->uptime=htonl(phead->uptime);
    phead->time=htonl(phead->time);
    phead->ntime=htonl(phead->ntime);
    phead->seq=htonl(phead->seq);    
}

void fillflow(flowrecord *rec,char *buf) {
    //Fill in flow struct and swap byteorder where and if needed:
    memcpy (&(rec->srcaddr),buf,4);
    memcpy (&(rec->dstaddr),buf+4,4);
    memcpy (&(rec->nexthop),buf+8,4);
    memcpy (&(rec->in_if),buf+12,2);
    memcpy (&(rec->out_if),buf+14,2);
    memcpy (&(rec->pktcount),buf+16,4);
    memcpy (&(rec->bytecount),buf+20,4);
    memcpy (&(rec->starttime),buf+24,4);
    memcpy (&(rec->endtime),buf+28,4);
    memcpy (&(rec->srcport),buf+32,2);
    memcpy (&(rec->dstport),buf+34,2);
    memcpy (&(rec->tcpflags),buf+37,1);
    memcpy (&(rec->ipproto),buf+38,1);
    memcpy (&(rec->tos),buf+39,1);
    memcpy (&(rec->src_as),buf+40,2);
    memcpy (&(rec->dst_as),buf+42,2);
    memcpy (&(rec->src_mask),buf+44,1);
    memcpy (&(rec->dst_mask),buf+45,1);	
	
    rec->srcaddr=htonl(rec->srcaddr);
    rec->dstaddr=htonl(rec->dstaddr);
    rec->nexthop=htonl(rec->nexthop);
    rec->in_if=ntohs(rec->in_if);
    rec->out_if=ntohs(rec->out_if);
    rec->pktcount=htonl(rec->pktcount);
    rec->bytecount=htonl(rec->bytecount);
    rec->starttime=htonl(rec->starttime);
    rec->endtime=htonl(rec->endtime);
    rec->srcport=ntohs(rec->srcport);
    rec->dstport=ntohs(rec->dstport);
    rec->src_as=ntohs(rec->src_as);
    rec->dst_as=ntohs(rec->dst_as);     
}

void * netflowlistener(void *threadid) {
    int *tid;
    tid = (int*)threadid;
    
    int sock, length, n;
    struct sockaddr_in server;
    struct sockaddr_in from;
    socklen_t fromlen;
    char buf[1470];
    printf("Created thread\n");
    sock=socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) err_func("Opening socket");

    length = sizeof(server);
    bzero(&server, length);
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_port=htons(cfg.netflow_port);

// need free!!!!!
    pheader * packet = new pheader;
    flowrecord * records = new flowrecord[30];

    user * currentuser;
    uint8_t flow_direction;
    uint32_t dst_ip;
    user_zone * currentzone;
   
    if (bind(sock,(struct sockaddr *)&server,length) < 0) 
        err_func("binding");
   
    fromlen =  sizeof(struct sockaddr_in);
    while (1) {
        n = recvfrom(sock, buf, 1470, 0, (struct sockaddr *)&from, &fromlen);
        if (n < 0) err_func("recvfrom");
	fillhdr(packet,buf);
        printf("received datagram!\n");
      /*  printf("received datagram! length is:%i\nDecoding data:\n",n);
	//Fill in packet data
	//Protocol version
            printf("Protocol version: %i\n",packet->pver);
	//Number of flows:
            printf("Number of flows: %i\n",packet->nflows);
	//NAS uptime:
            printf("NAS uptime: %i\n",packet->uptime);
	//flow_sequence
            printf("First flow sequence: %i\n",packet->seq);*/
        pthread_mutex_lock (&users_table_m);
        for (n = 0; n < packet->nflows; n++) {
            fillflow(&(records[n]),buf+24+n*48);
            //printf("\n\nflow sequence: %i\n",packet->seq+n);
            //printf(" srcport %6i: dstport %6i: packets %6i : bytes %6i : src_addr %u : dcs_addr %u \n",records[n].srcport,records[n].dstport,records[n].pktcount,records[n].bytecount, records[n].srcaddr, records[n].dstaddr);
            currentuser = getuserbyip(records[n].srcaddr, records[n].dstaddr);
            if (currentuser != NULL) {
                // Get flow direction (0 -> out, 1 -> in)
                dst_ip=( records[n].srcaddr==currentuser->user_ip ? records[n].dstaddr : records[n].srcaddr);
                flow_direction=( records[n].srcaddr==currentuser->user_ip ? 0 : 1);
                //printf("current user id:%u, dstaddr %u, direction %i\n",currentuser->id, dst_ip, flow_direction);
                
		currentzone = getflowzone(currentuser, dst_ip);
		if (currentzone != NULL) {
                    for (zone_group * p = currentuser->first_zone_group; p != NULL; p = p->next) {
                        if (p->id == currentzone->zone_group_id) {
                            if (flow_direction == 0) {
                                currentzone->zone_out_bytes += records[n].bytecount;
                                p->out_bytes += records[n].bytecount;
                                p->group_changed = 1;                                
                            }
                            if (flow_direction == 1) {
                                currentzone->zone_in_bytes += records[n].bytecount;
                                p->in_bytes += records[n].bytecount;
                                p->in_mb_cost_total += (records[n].bytecount * currentzone->zone_mb_cost)/MB_LENGTH;
                                p->group_changed = 1;
                                float old_debit = currentuser->user_debit;
                                currentuser->user_debit -= (records[n].bytecount * currentzone->zone_mb_cost)/MB_LENGTH;
                                if (old_debit != currentuser->user_debit) currentuser->debit_changed = 1;
                            }
                        }
                    }
                } else {
                    printf("Warning! Zone not found! (uid: %u, srcaddr: %u, dscaddr %u)\n",currentuser->id, records[n].srcaddr, records[n].dstaddr);
                };
            } else {
                /*char * src = ipFromIntToStr(records[n].srcaddr);
                char * dst = ipFromIntToStr(records[n].dstaddr);
                printf("Warning! User not found (srcaddr: %s, dscaddr %s)\n", src, dst);
                delete src; delete dst;*/
            };
	};
        // update values in database
      if ((time(NULL) - cfg.stats_updated_time) > cfg.stats_update_interval) {
      //  printf("t:%u\n", time(NULL));
        cfg.stats_updated_time = time(NULL);
        for (user * u = firstuser; u != NULL; u = u->next) {
            for (zone_group * p = u->first_zone_group; p != NULL; p = p->next) {
                if (p->group_changed == 1) {
                    char * sql = new char[1024];
                    uint64_t in_bytes = p->in_bytes;
                    uint64_t out_bytes = p->out_bytes;
                    uint32_t pid = p->id;
                    uint32_t uid = u->id;
                    float traf_in_money = p->in_mb_cost_total;
                    sprintf(sql, "UPDATE statistics SET traf_in=%llu, traf_out=%llu, traf_in_money=%f WHERE (user_id=%lu) AND (zone_group_id=%lu) AND (connected=1)", in_bytes, out_bytes, traf_in_money, uid, pid);
                    mysql_query(cfg.myconn, sql);
                    printf("%s\n",sql);
                    delete sql;
                    p->group_changed = 0;                    
                }
            }
            if (u->debit_changed == 1) {
                 u->debit_changed = 0;
                 char * sql = new char[1024];
                 sprintf(sql, "UPDATE users SET debit=%f WHERE id=%u", u->user_debit,u->id);
                 mysql_query(cfg.myconn, sql);
                 printf("%s\n",sql);
                 delete sql;                
            }
        }
      }
        
        pthread_mutex_unlock (&users_table_m);      
   };
    pthread_exit(NULL);
 }

