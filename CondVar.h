#ifndef GFRACT_CONDVAR_H
#define GFRACT_CONDVAR_H

#include <boost/utility.hpp>
#include <pthread.h>

class Mutex;

class CondVar : boost::noncopyable
{
public:
    CondVar();
    ~CondVar();

    void wait(Mutex* mutex);
    void broadcast();
    void signal();

private:
    pthread_cond_t cond;
};


#endif
