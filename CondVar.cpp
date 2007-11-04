#include "CondVar.h"
#include "Mutex.h"
#include "misc.h"

CondVar::CondVar(void)
{
    gf_a(pthread_cond_init(&cond, NULL) == 0);
}

CondVar::~CondVar()
{
    gf_a(pthread_cond_destroy(&cond) == 0);
}

void CondVar::wait(Mutex* mutex)
{
    gf_a(pthread_cond_wait(&cond, &mutex->mutex) == 0);
}

void CondVar::broadcast(void)
{
    gf_a(pthread_cond_broadcast(&cond) == 0);
}

void CondVar::signal(void)
{
    gf_a(pthread_cond_signal(&cond) == 0);
}
