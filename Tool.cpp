#include "Tool.h"
#include <gdk/gdkkeysyms.h>
#include "image_info.h"
#include "main.h"

const float ZoomInTool::ZOOM_BOX_WIDTH = 0.35;

ZoomInTool* ZoomInTool::me = NULL;

static gint zoom_in_callback(int arg);

Tool::Tool(image_info* img)
{
    this->img = img;
}

Tool::ButtonType Tool::fromGDKButtonType(int button)
{
    ButtonType bt;

    switch (button)
    {
    case 1:
        bt = LEFT;

        break;

    case 2:
        bt = MIDDLE;

        break;

    case 3:
        bt = RIGHT;

        break;

    default:
        bt = LEFT;
    }

    return bt;
}

ZoomOutTool::ZoomOutTool(image_info* img)
    : Tool(img)
{
}

bool ZoomOutTool::activate()
{
    double half_w, half_h;

    half_w = (img->finfo.xmax - img->finfo.xmin) / 2.0;
    half_h = (img->finfo.ymax - img->ymin()) / 2.0;

    img->finfo.ymax += half_h;
    img->finfo.xmin -= half_w;
    img->finfo.xmax += half_w;

    start_rendering(img);

    return false;
}

ZoomInTool::ZoomInTool(image_info* img, GtkWidget* toolbarButton)
    : Tool(img)
{
    this->toolbarButton = toolbarButton;

    timer_id = -1;

    if (!me)
    {
        me = this;
    }
}

bool ZoomInTool::activate()
{
    rect.x = 0;
    rect.y = 0;
    rect.width = int(ZOOM_BOX_WIDTH * img->user_width);
    rect.height = int(rect.width / img->ratio);

    draw_xor_rect(rect);

    return true;
}

void ZoomInTool::deactivate()
{
    draw_xor_rect(rect);

    killTimer();

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbarButton), FALSE);
}

void ZoomInTool::killTimer()
{
    if (timer_id != -1)
    {
        g_source_remove(timer_id);
        timer_id = -1;
    }
}

void ZoomInTool::buttonEvent(ButtonType type, bool isPress, int x, int y)
{
    if (!isPress)
    {
        // button has been released, stop changing zoom box size if we
        // were doing that
        killTimer();

        return;
    }

    // don't accept additional clicks while timer is active (other than
    // middle click)
    if ((timer_id != -1) && ((type == LEFT) || (type == RIGHT)))
    {
        return;
    }

    int zoom_dir = 0;

    if (type == LEFT)
    {
        zoom_dir = 1;
    }
    else if (type == MIDDLE)
    {
        zoom();
    }
    else if (type == RIGHT)
    {
        zoom_dir = -1;
    }

    if (zoom_dir)
    {
        draw_xor_rect(rect);

        resizeRect(zoom_dir);

        if (!rectIsValidSize())
        {
            resizeRect(-zoom_dir);
        }
        else
        {
            timer_id = g_timeout_add(ZOOM_INTERVAL,
                                     (GtkFunction)zoom_in_callback,
                                     (gpointer)(2 * zoom_dir));
        }

        draw_xor_rect(rect);
    }
}

void ZoomInTool::moveEvent(int x, int y)
{
    draw_xor_rect(rect);

    rect.x = x;
    rect.y = y;

    draw_xor_rect(rect);
}

bool ZoomInTool::keyEvent(GdkEventKey* ev)
{
    switch (ev->keyval)
    {
    case GDK_Return:
        zoom();

        return true;
    }

    return false;
}

void ZoomInTool::zoom(void)
{
    double xmin, xmax, ymax;

    ymax = rect.y;
    xmin = rect.x;
    xmax = rect.x + rect.width - 1;

    get_coords(&xmin, &ymax);
    get_coords(&xmax, NULL);

    img->finfo.ymax = ymax;
    img->finfo.xmin = xmin;
    img->finfo.xmax = xmax;

    tool_deactivate();

    start_rendering(img);
}

void ZoomInTool::resizeRect(int dir)
{
    rect.width += dir * 4;
    rect.height = int(rect.width / img->ratio);
}

int ZoomInTool::rectIsValidSize(void)
{
    return !((rect.width < 4) || (rect.width > (img->user_width - 16)) ||
             (rect.height < 4) || (rect.height > (img->user_height - 16)));
}

void ZoomInTool::timerCallback(int arg)
{
    draw_xor_rect(rect);

    resizeRect(arg);

    if (!rectIsValidSize())
    {
        resizeRect(-arg);
    }

    draw_xor_rect(rect);
}

gint zoom_in_callback(int arg)
{
    ZoomInTool::me->timerCallback(arg);

    return TRUE;
}
