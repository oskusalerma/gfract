#include "Mutex.h"
#include "misc.h"

Mutex::Mutex()
{
    gf_a(pthread_mutex_init(&mutex, NULL) == 0);
}

Mutex::~Mutex()
{
    gf_a(pthread_mutex_destroy(&mutex) == 0);
}


void Mutex::lock()
{
    gf_a(pthread_mutex_lock(&mutex) == 0);
}

void Mutex::unlock()
{
    gf_a(pthread_mutex_unlock(&mutex) == 0);
}
