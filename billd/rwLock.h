#ifndef RWLOCK_H_INCLUDED
#define RWLOCK_H_INCLUDED

class rwLock {
    private:

    uint32_t readers;
    uint8_t writing;
    pthread_mutex_t flagMutex;
    pthread_cond_t writeDone;
    pthread_cond_t readDone;

    public:
    rwLock(void);
    ~rwLock(void);
    uint32_t lockRead(void);
    uint32_t unlockRead(void);
    void lockWrite(void);
    void unlockWrite(void);

};

#endif // RWLOCK_H_INCLUDED
