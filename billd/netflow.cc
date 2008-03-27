#include "billing.h"

void fillhdr(pheader * phead, char *buf)
{
	//Fill in packet struct and swap byteorder where and if needed:
	memcpy(&(phead->pver), buf, 2);
	memcpy(&(phead->nflows), buf + 2, 2);
	memcpy(&(phead->uptime), buf + 4, 4);
	memcpy(&(phead->time), buf + 8, 4);
	memcpy(&(phead->ntime), buf + 12, 4);
	memcpy(&(phead->seq), buf + 16, 4);
	memcpy(&(phead->etype), buf + 20, 1);
	memcpy(&(phead->eid), buf + 21, 1);
	memcpy(&(phead->pad), buf + 22, 2);

	phead->pver = ntohs(phead->pver);
	phead->nflows = ntohs(phead->nflows);
	phead->uptime = ntohl(phead->uptime);
	phead->time = ntohl(phead->time);
	phead->ntime = ntohl(phead->ntime);
	phead->seq = ntohl(phead->seq);
}

void fillflow(flowrecord * rec, char *buf)
{
	//Fill in flow struct and swap byteorder where and if needed:
	memcpy(&(rec->srcaddr), buf, 4);
	memcpy(&(rec->dstaddr), buf + 4, 4);
	memcpy(&(rec->nexthop), buf + 8, 4);
	memcpy(&(rec->in_if), buf + 12, 2);
	memcpy(&(rec->out_if), buf + 14, 2);
	memcpy(&(rec->pktcount), buf + 16, 4);
	memcpy(&(rec->bytecount), buf + 20, 4);
	memcpy(&(rec->starttime), buf + 24, 4);
	memcpy(&(rec->endtime), buf + 28, 4);
	memcpy(&(rec->srcport), buf + 32, 2);
	memcpy(&(rec->dstport), buf + 34, 2);
	memcpy(&(rec->tcpflags), buf + 37, 1);
	memcpy(&(rec->ipproto), buf + 38, 1);
	memcpy(&(rec->tos), buf + 39, 1);
	memcpy(&(rec->src_as), buf + 40, 2);
	memcpy(&(rec->dst_as), buf + 42, 2);
	memcpy(&(rec->src_mask), buf + 44, 1);
	memcpy(&(rec->dst_mask), buf + 45, 1);
	rec->srcaddr = ntohl(rec->srcaddr);
	rec->dstaddr = ntohl(rec->dstaddr);
	rec->nexthop = ntohl(rec->nexthop);
	rec->in_if = ntohs(rec->in_if);
	rec->out_if = ntohs(rec->out_if);
	rec->pktcount = ntohl(rec->pktcount);
	rec->bytecount = ntohl(rec->bytecount);
	rec->starttime = ntohl(rec->starttime);
	rec->endtime = ntohl(rec->endtime);
	rec->srcport = ntohs(rec->srcport);
	rec->dstport = ntohs(rec->dstport);
	rec->src_as = ntohs(rec->src_as);
	rec->dst_as = ntohs(rec->dst_as);
}

void * netflowlistener(void *threadid)
{
	int *tid;
	tid = (int *)threadid;
	int sock, length, n;
	struct sockaddr_in server;
	struct sockaddr_in from;
	socklen_t fromlen;
	char buf[1470];
	printf("Created netflow thread\n");
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) err_func("Opening socket");
	length = sizeof(server);
	bzero(&server, length);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(cfg.netflow_listen_port);
	//need free ! !!!!
	pheader * packet = new pheader;
	flowrecord *records = new flowrecord[30];
	user *currentuser;
	uint8_t flow_direction;
	uint32_t dst_ip;
	user_zone *currentzone;
	if (bind(sock, (struct sockaddr *)&server, length) < 0)
	err_func("binding");
	fromlen = sizeof(struct sockaddr_in);
	while (1) {
		n = recvfrom(sock, buf, 1470, 0, (struct sockaddr *)&from, &fromlen);
		if (n < 0) err_func("recvfrom");
		fillhdr(packet, buf);
		if (cfg.debug_netflow) printf("received datagram!\n");
		//Fill in packet data
		// Protocol version
		if (cfg.debug_netflow) printf("Protocol version: %i\n", packet->pver);
		//Number of flows:
		if (cfg.debug_netflow) printf("Number of flows: %i\n", packet->nflows);
		//NAS uptime:
		if (cfg.debug_netflow) printf("NAS uptime: %i\n", packet->uptime);
		//flow_sequence
		if (cfg.debug_netflow) printf("First flow sequence: %i\n", packet->seq);

		for (n = 0; n < packet->nflows; n++) {
			fillflow(&(records[n]), buf + 24 + n * 48);
			verbose_mutex_lock(&users_table_m);//Lock all users while searching
			currentuser = getuserbyip(records[n].srcaddr, records[n].dstaddr,records[n].starttime,records[n].endtime);
			verbose_mutex_unlock(&users_table_m);//Unlock them when done
			
			if (currentuser != NULL) {
				if (cfg.debug_netflow) printf("Got user %i\n",currentuser->session_id);
				verbose_mutex_lock(&(currentuser->user_mutex));
				//Get flow direction(0->out, 1->in)
				dst_ip = (records[n].srcaddr == currentuser->user_ip ? records[n].dstaddr : records[n].srcaddr);
				flow_direction = (records[n].srcaddr == currentuser->user_ip ? 0 : 1);
				currentzone = getflowzone(currentuser, dst_ip);
				if (currentzone != NULL) {
					if (flow_direction == 0) 
					{
						currentzone->group_ref->out_bytes += records[n].bytecount;
						currentzone->group_ref->out_diff += records[n].bytecount;
						currentzone->group_ref->group_changed = 1;
						currentzone->zone_out_bytes += records[n].bytecount;
					}
					else if (flow_direction == 1) 
					{
						currentzone->group_ref->in_bytes += records[n].bytecount;
						currentzone->group_ref->in_diff += records[n].bytecount;
						currentzone->group_ref->group_changed = 1;
						currentzone->zone_in_bytes += records[n].bytecount;
					}
					if (cfg.debug_netflow) printf("Record %i: session %i, in: %lu, out: %lu\n",packet->seq+n,currentuser->session_id,currentzone->group_ref->in_bytes,currentzone->group_ref->out_bytes);
				} else {
					if (cfg.debug_netflow) printf("Warning! Zone not found! (uid: %u, srcaddr: %u, dscaddr %u)\n", currentuser->id, records[n].srcaddr, records[n].dstaddr);
				};
				verbose_mutex_unlock(&(currentuser->user_mutex));
			} else {
				char *src = ipFromIntToStr(records[n].srcaddr);
				char *dst = ipFromIntToStr(records[n].dstaddr);
				if (cfg.debug_netflow) printf("Warning! User not found (srcaddr: %s, dstaddr %s)\n", src, dst);
				delete src;
				delete dst;
			};
		};
	};
	pthread_exit(NULL);
}
