#include "billing.h"

void *userconnectlistener (void *threadid) {
	int *tid;
	tid = (int*)threadid;
	printf("In thread: created userconnect thread %d\n", tid);
	
	pthread_exit(NULL);
}
