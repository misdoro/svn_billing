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
		logmsg(DBG_EVENTS, "create stream socket on %s, port %i failed", inet_ntoa(cfg.events_listen_addr), cfg.events_listen_port );
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
	cfg.events_addr.sin_addr.s_addr = cfg.events_listen_addr.s_addr;
	if( bind( listensocket, (struct sockaddr *)&cfg.events_addr, sizeof( cfg.events_addr ) ) != 0 )
		logmsg(DBG_EVENTS, "bind socket on %s, port %i  failed", inet_ntoa(cfg.events_listen_addr), cfg.events_listen_port);
	//fcntl(ls, F_SETFL, 0);
	if( listen( listensocket, 5 ) != 0 ) printf( "E:put socket in listen state failed\n" );
	return listensocket;
};

void datahandler(int connectSocket, MYSQL * mysqllink){
	//Read data from connection, handle connecting and disconnecting users:
	int recieved = 0, rsize = 0;
	char * buffer = NULL;
	char buf[1024];
	for (;;) {
		rsize = recv(connectSocket, buf, 1024, 0);
		if (rsize <= 0) {
			break;
		}
		buffer = (char*) realloc(buffer, recieved + rsize);
		memcpy(buffer+recieved, &buf[0], rsize);
		recieved += rsize;
	}
	shutdown(connectSocket, 2);
	close(connectSocket);
	if (buffer != NULL) {

		int start = 0, step = 0, conn_mode = 0;
		char * p = buffer;
		for (int i=0; i<recieved ; i++) {
			if (*(p+i)==0x0a) {
				char * str = new char[i-start+1];
				memcpy(str, p+start, i-start);
				memset(str+i-start, '\0' ,1);
				switch (step) {
					//Check event type
					case 0:
						if (strcmp(str, "connect") == 0) {
							logmsg(DBG_EVENTS,"Some connected!");
							step = 1;
							conn_mode = CONNECT;
						} else if (strcmp(str, "disconnect") == 0) {
							logmsg(DBG_EVENTS,"Some disconnected!");
							step = 1;
							conn_mode = DISCONNECT;
						} else {
							logmsg(DBG_EVENTS,"Warning: Invalid data recieved (%s)!", str);
							step = -10;
						}
						break;
					//Get session ID and go on with it:
					case 1:
						char *session_id = new char[strlen(str)+1];
						strcpy(session_id, str);
						step++;
						logmsg(DBG_EVENTS,"Session id: '%s'", session_id);
						if (conn_mode == CONNECT) {
							nases.UserConnected(session_id);
						}
						if (conn_mode == DISCONNECT) {
							nases.UserDisconnected(session_id);
						}
						delete session_id;
					break;
				}
				delete str;
				start = i+1;
			}
		}
		free(buffer);
	}
}

void* ucsconn_handler ( void* ps ) {
	logmsg(DBG_THREADS,"ucsconn_handler started");
	int sc = *(int*)ps;
	int rs;
	struct sockaddr_in client;
	socklen_t fromlen;
	fromlen=sizeof(client);

	sched_yield();
	MYSQL *my_link = connectdb();

	while (cfg.stayalive){
		rs = accept( sc, (struct sockaddr *) &client, &fromlen );
		if (rs < 0 && errno!=EAGAIN) logmsg(DBG_EVENTS,"Error accepting" );
		else if (rs<0 &&	errno==EAGAIN ){	//Recvfrom exited by timeout, necessary to exit properly
			//logmsg(DBG_EVENTS,"Accept timeout, nothing special");
			continue;
		}else datahandler( rs, my_link );
	};
	verbose_mutex_lock( &ucsocket_mutex );
	ntr++;
	pthread_cond_signal( &ucsocket_event );
	verbose_mutex_unlock( &ucsocket_mutex );

	logmsg(DBG_THREADS,"ucsconn_handler has quit");
	mysql_thread_end();
	mysql_close(my_link);
	pthread_exit (NULL);
}

void * userconnectlistener (void *threadid) {
	logmsg(DBG_THREADS,"userconnectlistener started");
	int listenSocket;
	listenSocket = getucsocket();

	ntr=cfg.events_threads;
	pthread_t threads[ntr+1];
	//threads = (pthread_t*) malloc(sizeof(pthread_t)*(cfg.events_threads_max));

	while( cfg.stayalive ) {
		if( pthread_create( &threads[ntr], NULL, &ucsconn_handler, &listenSocket ) ) logmsg(DBG_EVENTS,"E:thread create error" );
		sched_yield();
		verbose_mutex_lock( &ucsocket_mutex );
		ntr--;
		while( ntr <= 0 ) pthread_cond_wait( &ucsocket_event, &ucsocket_mutex );
		verbose_mutex_unlock( &ucsocket_mutex );
	};
	logmsg(DBG_THREADS,"userconnectlistener has quit");
	mysql_thread_end();
    pthread_exit (NULL);
};


