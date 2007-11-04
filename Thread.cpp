#include "Thread.h"
#include "Runnable.h"
#include "misc.h"

Thread::Thread(Runnable* runnable, Joinable attr)
{
    init(startRunnable, runnable, attr);
}

Thread::Thread(startFunc_t startFunc, void* arg, Joinable attr)
{
    init(startFunc, arg, attr);
}

void Thread::init(startFunc_t startFunc, void* arg, Joinable attr)
{
    int ret = pthread_create(&tid, NULL, startFunc, arg);
    gf_a(ret == 0);

    if (attr == NOT_JOINABLE)
    {
        gf_a(pthread_detach(tid) == 0);
    }
}

void* Thread::startRunnable(void* arg)
{
    Runnable* runnable = static_cast<Runnable*>(arg);

    return runnable->run();
}

void* Thread::join()
{
    void* res;

    int ret = pthread_join(tid, &res);

    gf_a(ret == 0);

    return res;
}
