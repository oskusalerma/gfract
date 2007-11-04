#include "RenderThread.h"
#include <memory>
#include "image_info.h"
#include "palette.h"

RenderThread::RenderThread(image_info* img)
{
    this->img = img;
}

void* RenderThread::run()
{
    while (1)
    {
        std::auto_ptr<WorkItem> wi(img->wq.waitForItem());

        RowWorkItem* rwi = NULL;
        QuitWorkItem* qwi = NULL;

        if ((rwi = dynamic_cast<RowWorkItem*>(wi.get())))
        {
            doRow(rwi->row);
        }
        else if ((qwi = dynamic_cast<QuitWorkItem*>(wi.get())))
        {
            break;
        }
        else
        {
            gf_a(0);
        }
    }

    return NULL;
}

void RenderThread::doRow(int row)
{
    gf_a(row >= 0);
    gf_a(row < img->real_height);

    fractal_do_row(img->finfo.xmin, img->finfo.xmax,
        fractal_calc_y(row, img->finfo.ymax, img->finfo.xmin,
            img->finfo.xmax, img->real_width),
        img->real_width, img->depth, img->finfo.type,
        img->finfo.u.julia.c_re, img->finfo.u.julia.c_im,
        &img->color_in, &img->color_out,
        &img->raw_data[row * img->real_width]);

    if (img->aa_factor == 1) {
        palette_apply(img, 0, row, img->real_width, 1);
    }

    img->signalRowCompleted(row);
}
