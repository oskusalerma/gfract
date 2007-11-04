#ifndef GFRACT_THREAD_H
#define GFRACT_THREAD_H

#include <pthread.h>

class Runnable;

class Thread
{
public:
    enum Joinable { JOINABLE, NOT_JOINABLE };

    typedef void*(*startFunc_t)(void*);

    Thread(Runnable* runnable, Joinable attr = JOINABLE);

    Thread(startFunc_t startFunc, void* arg, Joinable attr = JOINABLE);

    void* join();

private:
    void init(startFunc_t startFunc, void* arg, Joinable attr);

    static void* startRunnable(void* arg);

    pthread_t tid;

};

#endif
