#ifndef RENDER_THREAD_H
#define RENDER_THREAD_H

#include <boost/utility.hpp>
#include "Runnable.h"

struct image_info;

/** Background thread for rendering. */
class RenderThread : public Runnable, boost::noncopyable
{
public:
    RenderThread(image_info* img);

    virtual void* run();

private:
    void doRow(int row);

    image_info* img;
};

#endif
