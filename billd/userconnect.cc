#include "billing.h"
#define MAX_MSG 100
#define LINE_ARRAY_SIZE (MAX_MSG+1)
#define CONNECT 1
#define DISCONNECT 2

using namespace std;

void *userconnectlistener (void *threadid) {
	int *tid;
	tid = (int*)threadid;
	printf("In thread: created userconnect thread %d\n", tid);
	int listenSocket, connectSocket, length;
	struct sockaddr_in server;
	struct sockaddr_in client;
	socklen_t fromlen;
	
	char linebuffer[LINE_ARRAY_SIZE];

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
		std::cout<<"listeningsocket"<< std::endl;
		connectSocket = accept(listenSocket,(struct sockaddr *) &client, &fromlen);
		memset(linebuffer, 0x0, LINE_ARRAY_SIZE);	
		//Read data from connection, handle connecting and disconnecting users:
		while (recv(connectSocket, linebuffer, MAX_MSG, 0) > 0) {
			int action=0;
			int step=0;
			//Look for type string:
			if (linebuffer[1]=='c') 
			{
				action=CONNECT;
				std::cout<<"Got connect"<< std::endl;
			}
			else if (linebuffer[1]=='d')
			{
				action=DISCONNECT;
				std::cout<<"Got disconnect"<< std::endl;
			}
			else if (action==CONNECT)
			{
				step++;
				if (step==4){
					std::cout<<"Got ip"<< linebuffer<< std::endl;
				};
				if (step==5){
                                        std::cout<<"Got user"<< linebuffer<< std::endl;
                                };
			}
			else if (action==DISCONNECT)
			{
			};
			std::cout<<"Got data"<<linebuffer<< std::endl;
			memset(linebuffer, 0x0, LINE_ARRAY_SIZE);
		}
	};
	
	pthread_exit(NULL);
}
