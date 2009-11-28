#include "billing.h"
//Class implementing locks for multiple reads && single write
rwLock::rwLock(void){
    pthread_mutex_init(&flagMutex, NULL);
    pthread_cond_init(&readDone, NULL);
    pthread_cond_init(&writeDone, NULL);
    writing = 0;
    readers = 0;
}

//Get read lock
uint32_t rwLock::lockRead(void){
    uint32_t myreaders;
    pthread_mutex_lock(&flagMutex);
    if (writing){//If writer is working, wait for it to finish.
        pthread_cond_wait(&writeDone,&flagMutex);
    };
    myreaders=++readers;
    pthread_mutex_unlock(&flagMutex);
    return myreaders;
}

//Release read lock
uint32_t rwLock::unlockRead(void){
    uint32_t myreaders;
    pthread_mutex_lock(&flagMutex);
    if (!(myreaders=--readers)){
        pthread_cond_broadcast(&readDone);
    };
    pthread_mutex_unlock(&flagMutex);
    return myreaders;
}

//Acquire write lock
void rwLock::lockWrite(void){
    pthread_mutex_lock(&flagMutex);
    while (writing) pthread_cond_wait(&writeDone,&flagMutex);	//Wait if someone is writing
    writing = true;						//Lock writer so new readers will wait
    while (readers>0) pthread_cond_wait(&readDone,&flagMutex);	//Wait for all readers to finish
    pthread_mutex_unlock(&flagMutex);
}

//Release write lock
void rwLock::unlockWrite(void){
    pthread_mutex_lock(&flagMutex);
    writing=false;
    pthread_cond_broadcast(&writeDone);
    pthread_mutex_unlock(&flagMutex);
}

//Destructor
rwLock::~rwLock(){
    pthread_cond_destroy(&writeDone);
    pthread_cond_destroy(&readDone);
    pthread_mutex_destroy(&flagMutex);

}
