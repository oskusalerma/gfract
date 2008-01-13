#ifndef __FRACTAL_TYPES_H
#define __FRACTAL_TYPES_H

#include <stdint.h>
#include <string>
#include <list>
#include <boost/utility.hpp>
#include <gtk/gtk.h>
#include "Mutex.h"
#include "Thread.h"
#include "WorkQueue.h"
#include "color.h"
#include "fractal.h"
#include "misc.h"

class Config;

struct julia_info
{
    double c_re;
    double c_im;
};

class fractal_info
{
public:
    // fractal type
    fractal_type type;

    // coordinates */
    double xmin,xmax,ymax;

    // saved mandelbrot coordinates. we need these when switching back
    // from julia mode.
    double old_xmin, old_xmax, old_ymax;

    // different fractal types' parameters
    union {
        julia_info julia;
    } u;

    std::string to_str() const;

    bool operator==(const fractal_info& rhs) const;
    bool operator!=(const fractal_info& rhs) const;
};

struct image_info : boost::noncopyable
{
    image_info();

    enum {
        DEFAULT_WIDTH = 800,
        DEFAULT_HEIGHT = 600,
        DEFAULT_AAFACTOR = 1,
        DEFAULT_DEPTH = 300,
        DEFAULT_NR_THREADS = 2
    };

    // reset to default Mandelbrot position
    void resetPosition();

    // load config. NOTE: setSize MUST be called after this.
    void load(Config* cfg, const std::string& section);

    // save config
    void save(Config* cfg, const std::string& section);

    // allocate memory for an image of the given size
    void setSize(int w, int h, int aaFactor);

    // clear the given memory buffers (set them to 0 / 0.0)
    void clear(bool raw, bool rgb);

    // clear the lines_done_vec array (set all to false)
    void clearLinesDone();

    // returns true if all the lines needed to AA the given row are rendered
    bool isAALineComplete(int row);

    // stop all background threads (and wait for them to stop)
    void stopThreads();

    // are we currently rendering this fractal
    bool render_in_progress;

    // are we currently stopping rendering
    bool stop_in_progress;

    // fractal info
    fractal_info finfo;

    /* recursion depth */
    unsigned int depth;

    /* actual data pointers */
    double* raw_data;
    uint32_t* rgb_data;

    /* true if we're a julia preview */
    bool j_pre;

    /* it's handy to keep this around */
    GtkWidget* drawing_area;

    /* real size. differs from user_size if anti-aliasing is used */
    int real_width;
    int real_height;

    /* user-visible size */
    int user_width;
    int user_height;

    /* width/height ratio */
    double ratio;

    /* anti-aliasing factor. if 1, no anti-aliasing */
    int aa_factor;

    /* true if palette interpolation should be used. */
    bool palette_ip;

    /* coloring information */
    color_ops color_out;
    color_ops color_in;



    /* our I/O callback id */
    int io_id;

    // threading stuff
    GIOChannel* ioc;
    int readFd;
    int writeFd;

    void signalRowCompleted(int row);

    // render threads post completed row numbers to this
    std::list<int> rows_completed;

    // protects the above list
    Mutex rows_completed_mutex;

    // used to post render tasks for threads to do
    WorkQueue wq;

    // how many lines to render have been posted on the work queue
    int lines_posted;

    // lines done
    int lines_done;

    // true for a given line if that is rendered. size is always
    // real_height. NOTE: this is a char, not a bool, because of the
    // stupidity of std::vector<bool> in the C++ standard (it does not
    // actually store bools and is slower).
    std::vector<char> lines_done_vec;

    // how many threads we should use
    int nr_threads;

    // render threads
    typedef std::list<Thread> threads_t;
    threads_t threads;
};

/** An order to render a single row. */
class RowWorkItem : public WorkItem
{
public:
    RowWorkItem(int row)
    {
        gf_a(row >= 0);

        this->row = row;
    }

    int row;
};

/** An order to render a range of rows (not used yet). */
class RowRangeWorkItem : public WorkItem
{
public:
    RowRangeWorkItem(int startRow, int endRow)
    {
        gf_a(startRow >= 0);
        gf_a(endRow >= 0);
        gf_a(startRow <= endRow);

        this->startRow = startRow;
        this->endRow = endRow;
    }

    int startRow;
    int endRow;
};

/** An order for the thread receiving it to quit. */
class QuitWorkItem : public WorkItem
{
public:
    QuitWorkItem() { }
};

#endif
