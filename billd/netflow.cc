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
	int sock, length, n,recieved;
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
	server.sin_addr.s_addr = cfg.netflow_listen_addr.s_addr;
	server.sin_port = htons(cfg.netflow_listen_port);
	//Define timeouts for rcvfrom
	timeval tv;
	tv.tv_sec = cfg.netflow_timeout;
	tv.tv_usec = 0;
	setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(timeval));
	//Create vars for packet data
	pheader * packet = new pheader;
	flowrecord *records = new flowrecord[30];

	int8_t flow_direction=-1;
	uint32_t dst_ip;
	uint16_t dst_port;
	if (bind(sock, (struct sockaddr *)&server, length) < 0)
	logmsg(DBG_NETFLOW,"Error binding netflow socket");
	fromlen = sizeof(struct sockaddr_in);
	while (cfg.stayalive)
	{
		recieved = recvfrom(sock, buf, 1470, 0, (struct sockaddr *)&from, &fromlen);
		if (recieved < 0 && errno!=EAGAIN) logmsg(DBG_NETFLOW,"Error receiving netflow datagram");
		else if (recieved<0 &&	errno==EAGAIN )
		{	//Recvfrom exited by timeout, necessary to exit properly
			logmsg(DBG_NETFLOW,"listener timeout");
			continue;
		};
		if (fillhdr(packet, buf)) {
		    logmsg(DBG_NETFLOW,"recieved malformed datagram");
			continue;
		}else
		{

			logmsg(DBG_NETFLOW,"recieved datagram!");
			//Fill in packet data
			// Protocol version
			logmsg(DBG_NETFLOW,"Protocol version: %u", packet->pver);
			//Number of flows:
			logmsg(DBG_NETFLOW,"Number of flows: %u", packet->nflows);
			//NAS uptime:
			logmsg(DBG_NETFLOW,"NAS uptime: %u", packet->uptime);
			//flow_sequence
			logmsg(DBG_NETFLOW,"First flow sequence: %u", packet->seq);


			//Get NAS:
			logmsg(DBG_NETFLOW,"NAS SOURCE ADDRESS: %s, port %i",inet_ntoa(from.sin_addr),ntohs(from.sin_port));
			C_NAS* thisnas = nases.getbyport(ntohs(from.sin_port));

			if (thisnas == NULL){
			    logmsg(DBG_NETFLOW,"NAS not found in list.\n");
			}else
			{
			    logmsg(DBG_NETFLOW,"NAS %s\n",thisnas->getName());

                //Fill in packet data
                for (n = 0; n < packet->nflows; n++)
                {
                    fillflow(&(records[n]), buf + 24 + n * 48);

                    C_user* thisuser;
                    //Calculate flow start&end time in seconds:
                    uint32_t starttime = (records[n].starttime/1000 - packet->uptime/1000)+packet->time;
                    uint32_t endtime = (records[n].endtime/1000 - packet->uptime/1000)+packet->time;

                    //Get user by flow IP:
                    if ((thisuser = thisnas->getUserByIP(records[n].srcaddr,starttime,endtime))!=NULL)
                    {
                        dst_ip 	 = records[n].dstaddr;
                        dst_port = records[n].dstport;
                        flow_direction = 0;
                    }else if ((thisuser = thisnas->getUserByIP(records[n].dstaddr,starttime,endtime))!=NULL){
                        dst_ip 	 = records[n].srcaddr;
                        dst_port = records[n].srcport;
                        flow_direction = 1;
                    }else{
                        logmsg (DBG_NETFLOW,"IP not found: srcaddr %u, dstaddr %u, starttime %u, endtime %u",records[n].srcaddr,records[n].dstaddr,starttime,endtime);
                        flow_direction = -1;
                        continue;
                    };

                    //Save flow data
                    if (flow_direction>=0)
                    {
			uint32_t zgid;
                        if ((zgid=thisuser->updateTraffic(dst_ip,dst_port,records[n].bytecount,flow_direction))>0)
                        {
                            logmsg(DBG_NETFLOW,"Record %i: session %u, group %u, dst %u, f_dst %u, f_src %u, dir %i",packet->seq+n, thisuser->getSID(),zgid,dst_ip,records[n].dstaddr,records[n].srcaddr,flow_direction);
                        }else{
                            logmsg(DBG_NETFLOW,"Warning! Zone not found! (sid: %u, ip %s, port %u)", thisuser->getSID(), ipFromIntToStr(dst_ip), dst_port);
                        };
                    };

				/*if (thisuser != NULL) {

                        //Store per-host-port stats:
                        //Construct new record:
                        stat_record * stat_r = new stat_record;
                        if (flow_direction == 0)
                        {
                            stat_r->bytes_out=records[n].bytecount;
                            stat_r->bytes_in=0;
                            stat_r->packets_out=records[n].pktcount;
                            stat_r->packets_in=0;
                        }
                        else if (flow_direction == 1)
                        {
                            stat_r->bytes_in=records[n].bytecount;
                            stat_r->bytes_out=0;
                            stat_r->packets_out=0;
                            stat_r->packets_in=records[n].pktcount;
                        };
                        stat_r->host=dst_ip;
                        stat_r->port=dst_port;
                        stat_r->new_rec=true;
                        stat_r->updated=true;
                        //Store record in tree:
                        host_node * stat_result=fs_update(currentuser->hostport_tree,dst_ip,stat_r, NULL);
                        //If just started tree and got it from fs_update, attach it to user, otherwise keep tree head (or empty, which is error)
                        if ((stat_result != NULL) &&  (currentuser->hostport_tree == NULL)) currentuser->hostport_tree = stat_result;
                        if (currentuser->hostport_tree == NULL) {
                            logmsg(DBG_NETFLOW,"Warning! User %i still has empty hostport tree! :(",currentuser->id);
                        };
                        verbose_mutex_unlock(&(currentuser->user_mutex));
                    } else {
                        logmsg(DBG_NETFLOW,"Warning! User not found (srcaddr: %s, dstaddr %s)", ipFromIntToStr(records[n].srcaddr), ipFromIntToStr(records[n].dstaddr));
                    };*/
                };
			};
		};
	};
	delete packet;
	delete records;
	logmsg(DBG_THREADS,"netflow thread finished");
	pthread_exit(NULL);
}
