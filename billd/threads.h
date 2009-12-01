#ifndef THREADS_H_INCLUDED
#define THREADS_H_INCLUDED

#include<iostream>
#include<pthread.h>
#include<sys/types.h>

using namespace std;

// this class is used to make generic calls to the run_thread function
// in any object which have been inherited from object class

class Object
{
    public:
      virtual void runThread(){}
};

//Threads class
class thread : public Object
{
        pthread_t thid;
        int ret;
        bool I_ran;

    public:
        thread();
        void start();
        void tryJoin();
        void join();
};

#endif // THREADS_H_INCLUDED
