#include "threads.h"
//executer non-member function
void* executer(void* param)
{
   Object *obj=(Object*)param;
   obj->runThread();
   pthread_exit(0);
}

thread::thread(){
    I_ran=false;
}

void thread::start()
{
    I_ran=true;
    ret=pthread_create(&thid,NULL,executer,(void*)this);
}

void thread::tryJoin()
{
    if (I_ran) join();
}

void thread::join()
{
    I_ran=false;
    pthread_join(thid,NULL);
}
