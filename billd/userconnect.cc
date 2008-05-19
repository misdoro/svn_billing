#include "billing.h"
#define MAX_MSG 8192
#define LINE_ARRAY_SIZE (MAX_MSG+1)
#define CONNECT 1
#define DISCONNECT 2

using namespace std;

static pthread_mutex_t ucsocket_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ucsocket_event = PTHREAD_COND_INITIALIZER;
int ntr;


int getucsocket( void ) {
	int rc = 1, listensocket;
	if( ( listensocket = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
		logmsg(DBG_EVENTS, "create stream socket on %s, port %i failed", ipFromIntToStr(cfg.events_listen_addr), cfg.events_listen_port );
	if( setsockopt( listensocket, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof( rc ) ) != 0 )
		logmsg(DBG_EVENTS, "set socket option SO_REUSEADDR failed" );
	//Set socket timeout:
	timeval tv;
	tv.tv_sec = cfg.events_timeout;
	tv.tv_usec = 0;
	if( setsockopt (listensocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(timeval)) !=0 )
		logmsg(DBG_EVENTS, "set socket option SO_RCVTIMEO failed" );
	memset( &cfg.events_addr, 0, sizeof( cfg.events_addr ) );
	//addr.sin_len = sizeof( addr );
	cfg.events_addr.sin_family = AF_INET;
	cfg.events_addr.sin_port = htons( cfg.events_listen_port );
	cfg.events_addr.sin_addr.s_addr = htonl( cfg.events_listen_addr );
	if( bind( listensocket, (struct sockaddr *)&cfg.events_addr, sizeof( cfg.events_addr ) ) != 0 )
		logmsg(DBG_EVENTS, "bind socket on %s, port %i  failed", ipFromIntToStr(cfg.events_listen_addr), cfg.events_listen_port);
	//fcntl(ls, F_SETFL, 0);
	if( listen( listensocket, 5 ) != 0 ) printf( "E:put socket in listen state failed\n" );
	return listensocket;
};

void datahandler(int connectSocket){
	//Read data from connection, handle connecting and disconnecting users:
	int recivied = 0, rsize = 0;
	char * buffer = NULL;
	char buf[1024];
	for (;;) {
		rsize = recv(connectSocket, buf, 1024, 0);
		if (rsize <= 0) {
			break;
		}
		buffer = (char*) realloc(buffer, recivied + rsize);
		memcpy(buffer+recivied, &buf[0], rsize);
		recivied += rsize;
	}
	shutdown(connectSocket, 2);
	close(connectSocket);
	if (buffer != NULL) {
		//Received data to buffer
		//Data format:
		//[connect|disconnect] username sessionid ppp_peer_ip linkname
		//
		//
		buffer = (char*) realloc(buffer, recivied + 1);
		char null = '\0';
		memcpy(buffer+recivied, &null, 1);
		int start = 0, pos = 0;
		int step = 0, conn_mode = 0;
		char * user_ip = NULL, * user_name = NULL, * session_id = NULL, *link_name = NULL;
		for (char * p = buffer; (strncmp(p, "\0",1) != 0); p++) {
			if (strncmp(p, "\n",1) == 0) {
				char * str = new char[pos-start+1];
				memcpy(str, p-pos+start, pos-start);
				memcpy(str+pos-start, &null ,1);
				p++;
				switch (step) {
					// get action
					case 0: if (strcmp(str, "connect") == 0) {
							logmsg(DBG_EVENTS,"Some connected!");
							step = 1;
							conn_mode = CONNECT;
						} else if (strcmp(str, "disconnect") == 0) {
							logmsg(DBG_EVENTS,"Some disconnected!");
							step = 1;
							conn_mode = DISCONNECT;
						} else {
							logmsg(DBG_EVENTS,"Warning: Invalid data received (%s)!", str);
							step = -10;
						}
						break;
					// connect :: username
					case 1: user_name = new char[strlen(str)+1];
                                                       strcpy(user_name, str);
						step++;
					break;
					// connect :: session_id
					case 2:
						session_id = new char[strlen(str)+1];
						strcpy(session_id, str);
						step++;
					break;
					// connect :: ip_address
					case 3:
						user_ip = new char[strlen(str)+1];
						strcpy(user_ip, str);
						step++;
					break;
					// connect :: ip address
					case 4: link_name = new char[strlen(str)+1];
						strcpy(link_name, str);
						step++;
						logmsg(DBG_EVENTS,"Username: '%s' User ip: '%s' Session id: '%s' Link name: '%s'", user_name, user_ip, session_id, link_name);
						if (conn_mode == CONNECT) {
							onUserConnected(session_id);
						}
						if (conn_mode == DISCONNECT) {
							onUserDisconnected(session_id);
						}
						if (user_ip != NULL) delete user_ip; user_ip = NULL;
						if (user_name != NULL) delete user_name; user_name = NULL;
						if (session_id != NULL) delete session_id; session_id = NULL;
						if (link_name != NULL) delete link_name; link_name = NULL;
					break;
				}
				delete str;
				start = pos;
			}
			pos++;
		}
		free(buffer);
	}
}

void* ucsconn_handler ( void* ps ) {
	int sc = *(int*)ps;
	int rs;
	struct sockaddr_in client;
	socklen_t fromlen;
	fromlen=sizeof(client);

	sched_yield();

	while (cfg.stayalive){
		rs = accept( sc, (struct sockaddr *) &client, &fromlen );
		if (rs < 0 && errno!=EAGAIN) logmsg(DBG_EVENTS,"Error accepting" );
		else if (rs<0 &&	errno==EAGAIN ){	//Recvfrom exited by timeout, necessary to exit properly
			logmsg(DBG_EVENTS,"Accept timeout, nothing special");
			continue;
		}else datahandler( rs );
	};
	/*pthread_mutex_lock( &ucsocket_mutex );
	ntr++;
	if ()
	pthread_cond_signal( &ucsocket_event );
	pthread_mutex_unlock( &ucsocket_mutex );
*/
	pthread_exit (NULL);
}

void * userconnectlistener (void *threadid) {
	logmsg(DBG_EVENTS,"In thread: created userconnect thread");
	int listenSocket;
	listenSocket = getucsocket();

	ntr=cfg.events_threads;
	pthread_t threads[ntr+1];
	//threads = (pthread_t*) malloc(sizeof(pthread_t)*(cfg.events_threads_max));

	while( cfg.stayalive ) {
		if( pthread_create( &threads[ntr], NULL, &ucsconn_handler, &listenSocket ) ) logmsg(DBG_EVENTS,"E:thread create error" );
		sched_yield();
		pthread_mutex_lock( &ucsocket_mutex );
		ntr--;
		while( ntr <= 0 ) pthread_cond_wait( &ucsocket_event, &ucsocket_mutex );
		pthread_mutex_unlock( &ucsocket_mutex );
	};
    pthread_exit (NULL);
};

