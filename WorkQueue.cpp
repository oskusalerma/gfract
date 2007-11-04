#include "WorkQueue.h"
#include "misc.h"

WorkQueue::WorkQueue()
{
}

WorkQueue::~WorkQueue()
{
    removeAll();
}

int WorkQueue::removeAll()
{
    Locker l(&mutex);

    int size = items.size();

    clearContainer(&items);

    return size;
}

void WorkQueue::add(WorkItem* item)
{
    Locker l(&mutex);

    items.push_back(item);
    condVar.signal();
}

inline WorkItem* WorkQueue::getItem()
{
    WorkItem* ptr = *items.begin();
    items.pop_front();

    return ptr;
}

WorkItem* WorkQueue::getAvailable()
{
    Locker l(&mutex);

    if (!items.empty())
    {
        return getItem();
    }
    else
    {
        return NULL;
    }
}

WorkItem* WorkQueue::waitForItem()
{
    Locker l(&mutex);

    while (1)
    {
        if (!items.empty())
        {
            return getItem();
        }
        else
        {
            condVar.wait(&mutex);
        }
    }
}
