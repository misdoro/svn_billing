#include "billing.h"

int fillhdr(pheader * phead, char *buf)
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
	if (phead->nflows>30) return 1;	//Warning! Malformed packet! Discard it! It can't contain more than 30 flow records!
	else return 0;
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
	logmsg(DBG_THREADS,"netflow thread started");
	int *tid;
	tid = (int *)threadid;
	int sock, length, n;
	struct sockaddr_in server;
	struct sockaddr_in from;
	socklen_t fromlen;
	char buf[1470];
	logmsg(DBG_NETFLOW,"Created netflow thread");
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) logmsg(DBG_NETFLOW,"Error opening netflow socket");
	length = sizeof(server);
	bzero(&server, length);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(cfg.netflow_listen_addr);
	server.sin_port = htons(cfg.netflow_listen_port);
	//Define timeouts for rcvfrom
	timeval tv;
	tv.tv_sec = cfg.netflow_timeout;
	tv.tv_usec = 0;
	setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(timeval));
	//Create vars for packet data
	pheader * packet = new pheader;
	flowrecord *records = new flowrecord[30];
	user *currentuser;
	uint8_t flow_direction;
	uint32_t dst_ip;
	uint16_t dst_port;
	user_zone *currentzone;
	if (bind(sock, (struct sockaddr *)&server, length) < 0)
	logmsg(DBG_NETFLOW,"Error binding netflow socket");
	fromlen = sizeof(struct sockaddr_in);
	while (cfg.stayalive) {
		n = recvfrom(sock, buf, 1470, 0, (struct sockaddr *)&from, &fromlen);
		if (n < 0 && errno!=EAGAIN) logmsg(DBG_NETFLOW,"Error receiving netflow datagram");
		else if (n<0 &&	errno==EAGAIN ){	//Recvfrom exited by timeout, necessary to exit properly
			logmsg(DBG_NETFLOW,"Netflow listener timeout");
			continue;
		};
		if (fillhdr(packet, buf)) {
			continue;
			//message("recieved malformed datagram from")
		}
		else
		{

			logmsg(DBG_NETFLOW,"received datagram!");
			//Fill in packet data
			// Protocol version
			logmsg(DBG_NETFLOW,"Protocol version: %u", packet->pver);
			//Number of flows:
			logmsg(DBG_NETFLOW,"Number of flows: %u", packet->nflows);
			//NAS uptime:
			logmsg(DBG_NETFLOW,"NAS uptime: %u", packet->uptime);
			//flow_sequence
			logmsg(DBG_NETFLOW,"First flow sequence: %u", packet->seq);
			//Fill in packet data
			for (n = 0; n < packet->nflows; n++) {
				fillflow(&(records[n]), buf + 24 + n * 48);
				verbose_mutex_lock(&users_table_m);//Lock all users while searching
				currentuser = getuserbyip(records[n].srcaddr, records[n].dstaddr,records[n].starttime,records[n].endtime);
				verbose_mutex_unlock(&users_table_m);//Unlock them when done

				if (currentuser != NULL) {
					logmsg(DBG_NETFLOW,"Got user %i",currentuser->session_id);
					verbose_mutex_lock(&(currentuser->user_mutex));
					//Get flow direction(0->out, 1->in)
					dst_ip = (records[n].srcaddr == currentuser->user_ip ? records[n].dstaddr : records[n].srcaddr);
					dst_port = (records[n].srcaddr == currentuser->user_ip ? records[n].dstport : records[n].srcport);
					flow_direction = (records[n].srcaddr == currentuser->user_ip ? 0 : 1);
					currentzone = getflowzone(currentuser, dst_ip, dst_port);
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
						logmsg(DBG_NETFLOW,"Record %i: session %i, in: %lu, out: %lu",packet->seq+n,currentuser->session_id,currentzone->group_ref->in_bytes,currentzone->group_ref->out_bytes);
					} else {
						logmsg(DBG_NETFLOW,"Warning! Zone not found! (uid: %u, srcaddr: %s, dstaddr %s)", currentuser->id, ipFromIntToStr(records[n].srcaddr), ipFromIntToStr(records[n].dstaddr));
					};
					verbose_mutex_unlock(&(currentuser->user_mutex));
				} else {
					logmsg(DBG_NETFLOW,"Warning! User not found (srcaddr: %s, dstaddr %s)", ipFromIntToStr(records[n].srcaddr), ipFromIntToStr(records[n].dstaddr));
				};
			};
		};
	};
	delete packet;
	delete records;
	logmsg(DBG_THREADS,"netflow thread finished");
	pthread_exit(NULL);
}
