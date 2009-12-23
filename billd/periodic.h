#ifndef PERIODIC_H_INCLUDED
#define PERIODIC_H_INCLUDED

class C_periodic: public thread{
    private:
        uint32_t lastDailyRun;  //Last daily functions run
        uint32_t startedDate;   //The date program started

        bool checkDailyRun();

    public:
    //Constructor
    C_periodic();
    //Destructor
    ~C_periodic();

    //Thread
    void runThread();

    void runDailyTasks(MYSQL*);

};

#endif // PERIODIC_H_INCLUDED
