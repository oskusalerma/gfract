#include "image_info.h"
#include <string.h>
#include "Config.h"
#include "RenderThread.h"
#include "externs.h"
#include "color.h"
#include "misc.h"

// config key names
static const std::string keyWidth("width");
static const std::string keyHeight("height");
static const std::string keyDepth("depth");
static const std::string keyAA("anti-aliasing");
static const std::string keyThreads("threads");

std::string fractal_info::to_str() const
{
    std::string s;

    s += "Type: ";
    switch (type) {
    case MANDELBROT:
        s += "Mandelbrot";
        break;
    case JULIA:
        s += "Julia";
        break;
    };

    s += "\n";

    s += strf(
        "XMin: %s\n"
        "XMax: %s\n"
        "YMax: %s\n",
        dbl2str(xmin).c_str(),
        dbl2str(xmax).c_str(),
        dbl2str(ymax).c_str());

    // FIXME: add Julia-specific info here

    return s;
}

double fractal_info::getY(const image_info& img, int y) const
{
    return ymax - ((xmax - xmin) / img.user_width) * y;
}

double fractal_info::getX(const image_info& img, int x) const
{
    return xmin + ((xmax - xmin) / img.user_width) * x;
}

double fractal_info::ymin(const image_info& img) const
{
    return getY(img, img.user_height);
}

bool fractal_info::operator==(const fractal_info& rhs) const
{
    if ((type != rhs.type) ||
        (xmin != rhs.xmin) ||
        (xmax != rhs.xmax) ||
        (ymax != rhs.ymax)) {
        return false;
    }

    // FIXME: test Julia-specific stuff here (if type is JULIA)

    return true;
}

bool fractal_info::operator!=(const fractal_info& rhs) const
{
    return !(*this == rhs);
}

image_info::image_info()
{
    real_width = user_width = DEFAULT_WIDTH;
    real_height = user_height = DEFAULT_HEIGHT;
    aa_factor = DEFAULT_AAFACTOR;
    depth = DEFAULT_DEPTH;
    nr_threads = DEFAULT_NR_THREADS;

    rgb_data = NULL;
    raw_data = NULL;

    resetPosition();

    render_in_progress = false;
    stop_in_progress = false;

    j_pre = false;
    palette_ip = false;
    finfo.type = MANDELBROT;

    color_out.nr = 1;
    color_out.ops[0].type = OP_ITER;

    color_in.nr = 1;
    color_in.ops[0].type = OP_NUMBER;
    color_in.ops[0].value = 0.0;

    io_id = -1;
    lines_posted = 0;
    lines_done = 0;
}

void image_info::resetPosition()
{
    finfo.xmin = -2.21;
    finfo.xmax = 1.0;
    finfo.ymax = 1.2;
}

double image_info::getY(int y) const
{
    return finfo.getY(*this, y);
}

double image_info::getX(int x) const
{
    return finfo.getX(*this, x);
}

double image_info::ymin() const
{
    return finfo.ymin(*this);
}

void image_info::load(Config* cfg, const std::string& section)
{
    user_width = cfg->getInt(section, keyWidth, DEFAULT_WIDTH, 1);
    user_height = cfg->getInt(section, keyHeight, DEFAULT_HEIGHT, 1);
    depth = cfg->getInt(section, keyDepth, DEFAULT_DEPTH, 1);
    aa_factor = cfg->getInt(section, keyAA, DEFAULT_AAFACTOR, 1);
    nr_threads = cfg->getInt(section, keyThreads, DEFAULT_NR_THREADS, 1);
}

void image_info::save(Config* cfg, const std::string& section) const
{
    cfg->setInt(section, keyWidth, user_width);
    cfg->setInt(section, keyHeight, user_height);
    cfg->setInt(section, keyDepth, depth);
    cfg->setInt(section, keyAA, aa_factor);
    cfg->setInt(section, keyThreads, nr_threads);
}

void image_info::signalRowCompleted(int row)
{
    gf_a(row >= 0);
    gf_a(row < real_height);

    {
        Locker l(&rows_completed_mutex);

        rows_completed.push_back(row);
    }

    char ch = 'Z';
    gf_a(write(writeFd, &ch, 1) == 1);
}

void image_info::setSize(int w, int h, int aaFactor)
{
    gf_a(w > 0);
    gf_a(h > 0);
    gf_a(aaFactor >= 1);

    bool same_size;

    same_size = (user_width == w) && (user_height == h) && rgb_data;

    if (!same_size)
    {
        delete[] rgb_data;
    }

    delete[] raw_data;

    user_width = w;
    user_height = h;
    aa_factor = aaFactor;
    ratio = (double)w / h;

    real_width = w * aaFactor;
    real_height = h * aaFactor;

    if (!same_size)
    {
        rgb_data = new uint32_t[user_width * user_height];
    }

    raw_data = new double[real_width * real_height];

    lines_done_vec.resize(real_height);

    clear(true, !same_size);
    clearLinesDone();
}

void image_info::clear(bool raw, bool rgb)
{
    if (raw) {
        for (int i = 0; i < real_width * real_height; i++)
            raw_data[i] = 0.0;
    }

    if (rgb) {
        for (int i = 0; i < user_width * user_height; i++)
            rgb_data[i] = palette[0];
    }
}

void image_info::clearLinesDone()
{
    memset(&lines_done_vec.front(), false, lines_done_vec.size());
}

bool image_info::isAALineComplete(int row) const
{
    gf_a(aa_factor > 1);
    gf_a(row >= 0);
    gf_a(row < user_height);

    for (int i = 0; i < aa_factor; i++)
    {
        if (!lines_done_vec.at(row * aa_factor + i))
        {
            return false;
        }
    }

    return true;
}

void image_info::adjustThreads()
{
    int new_threads = nr_threads - threads.size();

    if (new_threads == 0)
    {
        return;
    }
    else if (new_threads > 0)
    {
        for (int i = 0; i < new_threads; i++)
        {
            // FIXME: avoid leaking the memory for RenderThread
            threads.push_back(Thread(new RenderThread(this)));
        }
    }
    else
    {
        // this is inefficient but we don't know which threads are going to
        // pick up the quit notices, so we don't know which threads we
        // should call join on, so just delete all of them and create
        // the new number of threads.

        stopThreads();
        adjustThreads();
    }
}

void image_info::stopThreads()
{
    int nr = threads.size();

    for (int i = 0; i < nr; i++)
    {
        wq.add(new QuitWorkItem());
    }

    for (threads_t::iterator it = threads.begin(); it != threads.end(); ++it)
    {
        it->join();
    }

    threads.clear();
}

