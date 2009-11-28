#include "threads.h"
//executer non-member function
void* executer(void* param)
{
   Object *obj=(Object*)param;
   obj->runThread();
   pthread_exit(0);
}

void thread::start()
{
   ret=pthread_create(&thid,NULL,executer,(void*)this);
}

void thread::join()
{
   pthread_join(thid,NULL);
}
