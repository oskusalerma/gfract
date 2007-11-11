#ifndef GFRACT_MUTEX_H
#define GFRACT_MUTEX_H

#include <boost/utility.hpp>
#include <pthread.h>

class CondVar;

class Mutex : boost::noncopyable
{
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

private:
    pthread_mutex_t mutex;

    friend class CondVar;
};

// scope lock
class Locker : boost::noncopyable
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
