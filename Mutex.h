#ifndef GFRACT_MUTEX_H
#define GFRACT_MUTEX_H

#include <pthread.h>

class CondVar;

class Mutex
{
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

private:
    // make it non-copyable
    Mutex(Mutex&);

    pthread_mutex_t mutex;

    friend class CondVar;
};

// scope lock
class Locker
{
public:
    Locker(Mutex* mutex)
    {
        this->mutex = mutex;
        this->mutex->lock();
    }

    ~Locker()
    {
        mutex->unlock();
    }

private:
    Mutex* mutex;
};

#endif
