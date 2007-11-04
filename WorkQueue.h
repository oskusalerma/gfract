#ifndef GFRACT_WORKQUEUE_H
#define GFRACT_WORKQUEUE_H

#include <list>
#include "CondVar.h"
#include "Mutex.h"

/** Base class for workitems. */
class WorkItem
{
public:
    virtual ~WorkItem() { }
};

/** Classic work queue. */
class WorkQueue
{
public:
    WorkQueue();

    /** Leftover work items are deleted. */
    ~WorkQueue();

    /** Add a work items and wake up a random waiter (if one exists). */
    void add(WorkItem* item);

    /** Get a work item if one is available without waiting. If not,
        returns NULL. */
    WorkItem* getAvailable();

    /** Wait until a work item is available and return it. */
    WorkItem* waitForItem();

    /** Remove all items. Returns the number of items removed. */
    int removeAll();

private:
    /** Pop the first item from the list and return it. NOTE: caller has
        to have locked the mutex and checked that the list is not
        empty. */
    inline WorkItem* getItem();

    typedef std::list<WorkItem*> wlist_t;
    /** List of work items. */
    wlist_t items;

    // locking/signaling variables
    Mutex mutex;
    CondVar condVar;
};

#endif
