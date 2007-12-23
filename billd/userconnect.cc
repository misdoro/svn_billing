#include "billing.h"
#define MAX_MSG 8192
#define LINE_ARRAY_SIZE (MAX_MSG+1)
#define CONNECT 1
#define DISCONNECT 2

using namespace std;

void * userconnectlistener (void *threadid) {
	printf("In thread: created userconnect thread\n");
	int listenSocket, connectSocket, length;
	struct sockaddr_in server;
	struct sockaddr_in client;
	socklen_t fromlen;

	listenSocket=socket(AF_INET, SOCK_STREAM, 0);
	length = sizeof(server);
	bzero(&server, length);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port=htons(cfg.connectmsgport);
	bind(listenSocket,(struct sockaddr *) &server, sizeof(server));
	listen(listenSocket, 5);
	while (1) {
		fromlen=sizeof(client);
		//Wait for connection:
		connectSocket = accept(listenSocket,(struct sockaddr *) &client, &fromlen);
		//Read data from connection, handle connecting and disconnecting users:
                int recivied = 0, rsize = 0;
                char * buffer = NULL;
                char buf[1024];
                for (;;) {                    
                    rsize = recv(connectSocket, buf, 1024, 0);
                  //  printf("receiveddata \n");
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
                    buffer = (char*) realloc(buffer, recivied + 1);
                    char null = '\0';
                    memcpy(buffer+recivied, &null, 1);
                    int start = 0, pos = 0;
                    int step = 0, conn_mode = 0;
                    char * user_ip = NULL, * user_name = NULL; 
                    for (char * p = buffer; (strncmp(p, "\0",1) != 0); p++) {
                        if (strncmp(p, "\n",1) == 0) {
                            char * str = new char[pos-start+1];
                            memcpy(str, p-pos+start, pos-start);
                            memcpy(str+pos-start, &null ,1);                            
                            p++;
                            switch (step) {
                                // get action
                                case 0: if (strcmp(str, "connect") == 0) {
                                            printf ("Some connected!\n");
                                            step = 1;
                                            conn_mode = CONNECT;
                                        } else if (strcmp(str, "disconnect") == 0) {
                                            printf ("Some disconnected!\n");
                                            step = 1;
                                            conn_mode = DISCONNECT;
                                        } else {
                                            printf ("Warning: Invalid data received (%s)!\n", str);
                                            step = -10;
                                        }
                                        break;
                                // connect :: interface
                                case 1: step++; break;
                                // connect :: conn type
                                case 2: step++; break;
                                // connect :: gateway
                                case 3: step++;
                                        break;
                                // connect :: ip address
                                case 4: user_ip = new char[strlen(str)+1];
                                        strcpy(user_ip, str);
                                        step++;
                                        break;
                                // connect :: username
                                case 5: user_name = new char[strlen(str)+1];
                                        strcpy(user_name, str);
                                        printf("Username: %s, User ip: %s\n", user_name, user_ip);
                                        if (conn_mode == CONNECT) {
                                            onUserConnected(user_name, user_ip, 0);
                                        }
                                        if (conn_mode == DISCONNECT) {
                                            onUserDisconnected(htonl(inet_addr(user_ip)));
                                        }
                                        if (user_ip != NULL) delete user_ip; user_ip = NULL;
                                        if (user_name != NULL) delete user_name; user_name = NULL;                                        
                                        break;
                            }

                            delete str;
                            start = pos;
                        }
                        pos++;
                    }                    
                    free(buffer);
                }

	};
	
	pthread_exit(NULL);
}
