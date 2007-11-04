#ifndef GFRACT_RUNNABLE_H
#define GFRACT_RUNNABLE_H

/** Same as Java's Runnable interface. */
class Runnable
{
public:
    virtual ~Runnable() { }

    virtual void* run() = 0;
};

#endif
