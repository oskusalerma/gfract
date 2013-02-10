#include "Tool.h"
#include <gdk/gdkkeysyms.h>
#include "main.h"

const float ZoomInTool::ZOOM_BOX_WIDTH = 0.35;

ZoomInTool* ZoomInTool::me = NULL;

static gint zoom_in_callback(void* arg);
static gint j_pre_delete(GtkWidget *widget, GdkEvent *event, gpointer data);

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
                                     (void*)(2 * zoom_dir));
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
    double xmin = img->getX(rect.x);
    double xmax = img->getX(rect.x + rect.width);
    double ymax = img->getY(rect.y);

    img->finfo.xmin = xmin;
    img->finfo.xmax = xmax;
    img->finfo.ymax = ymax;

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

gint zoom_in_callback(void* arg)
{
    // doesn't compile on 64-bit systems unless we go
    // void* -> int64_t -> int
    ZoomInTool::me->timerCallback(reinterpret_cast<int64_t>(arg));

    return TRUE;
}

CropTool::CropTool(image_info* img, GtkWidget* toolbarButton)
    : Tool(img)
{
    this->toolbarButton = toolbarButton;

    x1 = -1;
}

bool CropTool::activate()
{
    return true;
}

void CropTool::deactivate()
{
    if (x1 != -1)
    {
        draw_xor_rect(getRect());
        x1 = -1;
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbarButton), FALSE);
}

void CropTool::buttonEvent(ButtonType type, bool isPress, int x, int y)
{
    if (!isPress || (type != LEFT))
    {
        return;
    }

    if (x1 == -1)
    {
        x1 = x;
        y1 = y;

        x2 = x;
        y2 = y;

        draw_xor_rect(getRect());
    }
    else
    {
        draw_xor_rect(getRect());

        x2 = x;
        y2 = y;

        crop();

        // disable drawing in deactivate
        x1 = -1;
    }
}

void CropTool::moveEvent(int x, int y)
{
    if (x1 != -1)
    {
        draw_xor_rect(getRect());

        x2 = x;
        y2 = y;

        draw_xor_rect(getRect());
    }
}

void CropTool::crop(void)
{
    GdkRectangle r = getRect();

    double oldW = img->finfo.xmax - img->finfo.xmin;
    double oldH = img->finfo.ymax - img->ymin();

    double xmin = img->getX(r.x);
    double xmax = img->getX(r.x + r.width);
    double ymax = img->getY(r.y);
    double ymin = img->getY(r.y + r.height);

    img->finfo.xmin = xmin;
    img->finfo.xmax = xmax;
    img->finfo.ymax = ymax;

    double newW = img->finfo.xmax - img->finfo.xmin;
    double newH = img->finfo.ymax - ymin;

    tool_deactivate();

    stop_rendering(img);

    img->setSize(std::max(1, int(img->user_width * (newW / oldW))),
                 std::max(1, int(img->user_height * (newH / oldH))),
                 img->aa_factor);

    image_size_changed();

    start_rendering(img);
}

GdkRectangle CropTool::getRect()
{
    GdkRectangle r;

    r.x = std::min(x1, x2);
    r.y = std::min(y1, y2);

    r.width = abs(x1 - x2) + 1;
    r.height = abs(y1 - y2) + 1;

    return r;
}

JuliaTool::JuliaTool(image_info* img, GtkWidget* toolbarButton)
    : Tool(img)
{
    this->toolbarButton = toolbarButton;

    j_pre.j_pre = true;
    j_pre.finfo.type = JULIA;

    j_pre.depth = 300;
    j_pre.nr_threads = 1;

    j_pre.finfo.xmin = -2.0;
    j_pre.finfo.xmax = 1.5;
    j_pre.finfo.ymax = 1.25;

    j_pre.finfo.u.julia.c_re = 0.3;
    j_pre.finfo.u.julia.c_im = 0.6;

    j_pre.setSize(JPRE_SIZE, int(JPRE_SIZE / img->ratio), JPRE_AA_FACTOR);

    /* preview window */
    j_pre_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(GTK_OBJECT(j_pre_window), "delete_event",
                     GTK_SIGNAL_FUNC(j_pre_delete), NULL);
    gtk_window_set_title(GTK_WINDOW(j_pre_window), "Preview");
    gtk_window_set_resizable(GTK_WINDOW(j_pre_window), FALSE);

    /* preview window drawing area */
    GtkWidget* tmp = gtk_drawing_area_new();
    gtk_widget_set_events(tmp, GDK_EXPOSURE_MASK);
    gtk_widget_set_size_request(tmp, j_pre.user_width, j_pre.user_height);
    gtk_container_add(GTK_CONTAINER(j_pre_window), tmp);
    g_signal_connect(GTK_OBJECT(tmp), "expose_event",
                     GTK_SIGNAL_FUNC(expose_event), &j_pre);
    gtk_widget_show(tmp);

    j_pre.drawing_area = tmp;
}

bool JuliaTool::activate()
{
    if (img->finfo.type == MANDELBROT)
    {
        gtk_widget_show(j_pre_window);
        start_rendering(&j_pre);

        return true;
    }
    else
    {
        img->finfo.xmin = old_xmin;
        img->finfo.xmax = old_xmax;
        img->finfo.ymax = old_ymax;
        img->finfo.type = MANDELBROT;

        start_rendering(img);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbarButton), FALSE);

        return false;
    }
}

void JuliaTool::deactivate()
{
    stop_rendering(&j_pre);
    gtk_widget_hide(j_pre_window);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbarButton), FALSE);
}

void JuliaTool::buttonEvent(ButtonType type, bool isPress, int x, int y)
{
    if ((type != LEFT) || !isPress)
    {
        return;
    }

    img->finfo.u.julia.c_re = img->getX(x);
    img->finfo.u.julia.c_im = img->getY(y);

    old_xmin = img->finfo.xmin;
    old_xmax = img->finfo.xmax;
    old_ymax = img->finfo.ymax;

    img->finfo.xmin = j_pre.finfo.xmin;
    img->finfo.xmax = j_pre.finfo.xmax;
    img->finfo.ymax = j_pre.finfo.ymax;

    img->finfo.type = JULIA;

    tool_deactivate();

    start_rendering(img);
}

void JuliaTool::moveEvent(int x, int y)
{
    j_pre.finfo.u.julia.c_re = img->getX(x);
    j_pre.finfo.u.julia.c_im = img->getY(y);

    start_rendering(&j_pre);
}

void JuliaTool::sizeEvent(bool toolIsActive)
{
    stop_rendering(&j_pre);

    int xw,yw;

    if ((JPRE_SIZE / img->ratio) < JPRE_SIZE)
    {
        xw = JPRE_SIZE;
        yw = int(JPRE_SIZE / img->ratio);

        if (yw == 0)
            yw = 1;
    }
    else
    {
        xw = int(JPRE_SIZE * img->ratio);

        if (xw == 0)
            xw = 1;

        yw = JPRE_SIZE;
    }

    j_pre.setSize(xw, yw, JPRE_AA_FACTOR);
    gtk_widget_set_size_request(j_pre.drawing_area, xw, yw);

    if (toolIsActive)
    {
        gtk_widget_hide(j_pre_window);
        gtk_widget_show(j_pre_window);

        start_rendering(&j_pre);
    }
}

// called when user closes the Julia preview window. nothing's actually
// deleted, we just hide the window by deactivating the tool.
gint j_pre_delete(GtkWidget* widget, GdkEvent* event, gpointer data)
{
    tool_deactivate();

    return TRUE;
}
