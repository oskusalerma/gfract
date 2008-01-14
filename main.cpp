#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <boost/filesystem/operations.hpp>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include "Config.h"
#include "Exception.h"
#include "Tool.h"
#include "attr_dlg.h"
#include "color_dlg.h"
#include "externs.h"
#include "image_info.h"
#include "misc.h"
#include "my_png.h"
#include "pal_rot_dlg.h"
#include "palette.h"
#include "timer.h"
#include "version.h"

#define MIN_WINDOW_WIDTH 320

/* recalc/stop button texts */
#define TEXT_RECALC     "Recalc"
#define TEXT_STOP       "Stop"

/* julia preview size */
#define JPRE_SIZE    160
#define JPRE_AAFACTOR 2

// config section/key names
static const std::string sectionMisc("misc");
static const std::string keyPalette("palette");

struct status_info
{
    int julia_browsing;
};

static std::string cfgFilename;

// we have a timeout every 10 seconds that checks whether the config has
// been updated and needs saving.
static bool cfgNeedsSaving = false;

// a simple history of the visited fractal positions
typedef std::vector<fractal_info*> finfo_list;
static finfo_list fhistory;

// index of current location in fhistory
static int fhistory_current_pos = -1;

// get current history entry
static const fractal_info& history_get_current();

// get given history entry
static const fractal_info& history_get(int pos);

// goto to the given history position. it is ok to give invalid values,
// in which case nothing is done.
static void history_goto(int pos);


/* options */
struct options
{
    // display amount of time needed for rendering
    int timing;
};

static gboolean io_callback(GIOChannel* source, GIOCondition condition,
    gpointer data);
static void stop_rendering(image_info* img);
static void start_julia_browsing(void);
static void stop_julia_browsing(void);
static void process_args(int argc, char** argv);
static void resize_preview(void);
static int find_palette_by_name(const std::string& name, bool must_exist);
static void quit(void);
static void redraw_image(image_info* img);
static void create_menus(GtkWidget* vbox);
static void create_threads(image_info* img);
static void menu_add_item(GtkWidget* menu, GtkWidget* item);
static GtkWidget* menu_add(GtkWidget* menu, const char* name, GCallback func,
    void* arg = NULL);
static void menu_bar_add(GtkWidget* menu, GtkWidget* submenu,
    const char* name);
static void set_tooltip(GtkWidget* w, const char* str);
static void show_msg_box(GtkWidget* parent, const std::string& msg);
static GdkRectangle horiz_intersect(GdkRectangle* a1, GdkRectangle* a2);
static GtkWidget* get_stock_image(const char* stock_id);

static gint expose_event(GtkWidget* widget, GdkEventExpose* event,
                         image_info* img);
static gint key_event(GtkWidget* widget, GdkEventKey* event);
static gint button_press_event(GtkWidget* widget, GdkEventButton* event);
static gint j_pre_delete(GtkWidget *widget, GdkEvent *event, gpointer data);
static gint child_reaper(gpointer nothing);
static gint cfg_saver(gpointer nothing);
static void tool_activate(Tool* tool);

static void invert(void);
static void switch_fractal_type(void);
static void print_help(void);
static void print_version(void);
static void save_config();
static void load_config();

/* DIALOG FUNCTIONS */

/* image attribute */
static void image_attr_ok_cmd(GtkWidget* w, image_attr_dialog* dl);
static void image_attr_apply_cmd(GtkWidget* w, image_attr_dialog* dl);
static void do_attr_dialog(void);

/* coloring mode */
static void do_color_dialog(void);

/* palette loading */
void reapply_palette(void);
static void load_palette_cmd(void);
static void load_builtin_palette_cmd(void* arg);
static void palette_apply_cmd(GtkWidget* w, GtkFileChooser* fc);

/* palette cycling */
static void do_pal_rot_dialog(void);

/* image saving */
static void save_cmd(void);

/* reset fractal position */
static void reset_fractal_cmd(void);

/* general stuff we need to have */
static status_info st;
static image_info img;
static image_info j_pre;
static Timer timing_info;
static options opts;
static GtkWidget* j_pre_window = NULL;
static GtkWidget* window = NULL;
static GtkWidget* drawing_area = NULL;
static GtkWidget* recalc_button_label = NULL;
static GtkWidget* depth_spin = NULL;
static GtkWidget* palette_ip = NULL;
static GtkWidget* pbar = NULL;
static GtkWidget* switch_menu_cmd = NULL;

static Tool* tool_active = NULL;
static Tool* tool_zoom_out = NULL;
static ZoomInTool* tool_zoom_in = NULL;

/* DIALOG POINTERS */

static image_attr_dialog* img_attr_dlg = NULL;
static color_dialog* color_dlg = NULL;
static palette_rotation_dialog* pal_rot_dlg = NULL;

void print_version(void)
{
    printf("gfract %s\n", VERSION);
}

void print_help(void)
{
    print_version();
    printf("Command-line arguments:\n");
    printf("-h, --help        Help\n");
    printf("--version         Display version\n");
    printf("-t                Display timing information\n");
}

void process_args(int argc, char** argv)
{
    int i;

    for (i=1; i < argc; i++) {
        if ((strcmp("-h", argv[i]) == 0) ||
            (strcmp("--help", argv[i]) == 0)) {
            print_help();
            exit(0);
        } else if (strcmp("-t", argv[i]) == 0) {
            opts.timing = 1;
        } else if (strcmp("--version", argv[i]) == 0) {
            print_version();
            exit(0);
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            exit(1);
        }
    }
}

const fractal_info& history_get_current()
{
    return history_get(fhistory_current_pos);
}

const fractal_info& history_get(int pos)
{
    return *fhistory.at(pos);
}

void history_goto(int pos)
{
    if ((pos < 0) || (pos >= (int)fhistory.size())) {
        return;
    }

    img.finfo = history_get(pos);
    fhistory_current_pos = pos;

    start_rendering(&img);
}

void invert(void)
{
    palette_invert();

    if (img.aa_factor == 1)
        palette_apply(&img, 0, 0, img.user_width, img.user_height);
    else
        rgb_invert(&img);

    redraw_image(&img);
}

void save_cmd(void)
{
    do_png_save(&img, window);
}

void reset_fractal_cmd(void)
{
    stop_rendering(&img);

    img.resetPosition();

    start_rendering(&img);
}

void tool_deactivate()
{
    tool_activate(NULL);
}

void tool_activate(Tool* tool)
{
    if (tool_active)
    {
        tool_active->deactivate();
    }

    if (!tool || (tool == tool_active))
    {
        tool_active = NULL;
    }
    else
    {
        tool_active = tool->activate() ? tool : NULL;
    }

    if (tool_active)
    {
        // if we have a tool still active, make sure the drawing area has
        // the keyboard focus so our tool gets any keyboard input

        gtk_widget_grab_focus(img.drawing_area);
    }
}

void switch_fractal_type(void)
{
    if (img.finfo.type == MANDELBROT) {
        if (!st.julia_browsing)
            start_julia_browsing();
    } else if (img.finfo.type == JULIA) {
        img.finfo.xmin = img.finfo.old_xmin;
        img.finfo.xmax = img.finfo.old_xmax;
        img.finfo.ymax = img.finfo.old_ymax;
        img.finfo.type = MANDELBROT;
        start_rendering(&img);
    }
}

void reapply_palette(void)
{
    if (img.aa_factor == 1)
        palette_apply(&img, 0, 0, img.user_width, img.user_height);
    else
        do_anti_aliasing(&img, 0, 0, img.user_width, img.user_height);

    redraw_image(&img);
}

void palette_apply_cmd(GtkWidget* w, GtkFileChooser* fc)
{
    char* filename = gtk_file_chooser_get_filename(fc);

    if (!palette_load(filename)) {
        show_msg_box(window, strf("Invalid palette file '%s'\n", filename));
    } else {
        reapply_palette();
    }

    g_free(filename);

    cfgNeedsSaving = true;
}

void load_palette_cmd(void)
{
    GtkWidget* dlg;

    dlg = gtk_file_chooser_dialog_new("Load palette", GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dlg),
        "/usr/local/share/gfract/palettes", NULL);

    const std::string& palName = palette_get_current_name();

    if (!palName.empty() && (palName[0] == '/'))
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg),
            palName.c_str());
    }

    GtkWidget* apply = gtk_button_new_with_label("Apply palette");
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dlg), apply);
    g_signal_connect(GTK_OBJECT(apply), "clicked",
        GTK_SIGNAL_FUNC(palette_apply_cmd), GTK_FILE_CHOOSER(dlg));

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        palette_apply_cmd(NULL, GTK_FILE_CHOOSER(dlg));
    }

    gtk_widget_destroy(dlg);
}

void load_builtin_palette_cmd(void* arg)
{
    int n = reinterpret_cast<int>(arg);

    palette_load_builtin(n);
    reapply_palette();

    cfgNeedsSaving = true;
}

void init_misc(void)
{
    // FIXME: ignore sigpipe

    fhistory.push_back(new fractal_info(img.finfo));
    fhistory_current_pos = 0;

    /* init preview */
    j_pre.depth = 300;

    j_pre.finfo.xmin = -2.0;
    j_pre.finfo.xmax = 1.5;
    j_pre.finfo.ymax = 1.25;

    j_pre.finfo.u.julia.c_re = 0.3;
    j_pre.finfo.u.julia.c_im = 0.6;

    j_pre.j_pre = true;
    j_pre.finfo.type = JULIA;

    /* misc init */
    st.julia_browsing = false;

    opts.timing = 0;
}

/* returns the horizontal intersection part of a1 and a2. if the
   rectangles don't overlap, it returns a rectangle with a width of 0.
*/
GdkRectangle horiz_intersect(GdkRectangle* a1, GdkRectangle* a2)
{
    GdkRectangle tmp;
    int ax1, ax2, bx1, bx2, cx1, cx2;

    tmp.width = 0;
    tmp.y = a1->y;
    tmp.height = a1->height;

    ax1 = a1->x;
    ax2 = a1->x + a1->width - 1;

    bx1 = a2->x;
    bx2 = a2->x + a2->width - 1;

    cx1 = std::max(ax1,bx1);
    cx2 = std::min(ax2,bx2);

    /* no overlap */
    if (cx2 < cx1)
        return tmp;

    tmp.x = cx1;
    tmp.width = cx2-cx1+1;

    return tmp;
}

void redraw_image(image_info* img)
{
    GdkRectangle rect;

    rect.x = rect.y = 0;
    rect.width = img->user_width;
    rect.height = img->user_height;

    gdk_window_invalidate_rect(img->drawing_area->window, &rect, TRUE);
}

gint do_palette_rotation(bool forward)
{
    if (forward)
        palette_rotate_forward();
    else
        palette_rotate_backward();

    if (img.aa_factor == 1)
        palette_apply(&img, 0, 0, img.user_width, img.user_height);
    else
        do_anti_aliasing(&img, 0, 0, img.user_width, img.user_height);

    redraw_image(&img);

    return TRUE;
}

void show_msg_box(GtkWidget* parent, const std::string& msg)
{
    GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(parent),
        GtkDialogFlags(0),
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s",
        msg.c_str());

    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

void image_attr_ok_cmd(GtkWidget* w, image_attr_dialog* dl)
{
    image_attr_apply_cmd(w, dl);
    gtk_widget_destroy(dl->dialog);
}

void image_attr_apply_cmd(GtkWidget* w, image_attr_dialog* dl)
{
    stop_rendering(&img);

    img.setSize(
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dl->width)),
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dl->height)),
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dl->aa)));

    if (img.user_width < MIN_WINDOW_WIDTH) {
        /* limit minimum window width to MIN_WINDOW_WIDTH and handle
           this case in the expose event */
        gtk_widget_set_size_request(drawing_area,
                              MIN_WINDOW_WIDTH, img.user_height);
    } else {
        gtk_widget_set_size_request(drawing_area,
                              img.user_width, img.user_height);
    }

    img.nr_threads = gtk_spin_button_get_value_as_int(
        GTK_SPIN_BUTTON(dl->threads));

    start_rendering(&img);

    resize_preview();

    if (st.julia_browsing) {
        gtk_widget_hide(j_pre_window);
        gtk_widget_show(j_pre_window);
        start_rendering(&j_pre);
    }

    cfgNeedsSaving = true;
}

void main_refresh(void)
{
    start_rendering(&img);
}

/* change preview window to reflect possible new aspect ratio */
void resize_preview(void)
{
    stop_rendering(&j_pre);

    int xw,yw;

    if (JPRE_SIZE/img.ratio < JPRE_SIZE) {
        xw = JPRE_SIZE;
        yw = int(JPRE_SIZE/img.ratio);
        if (yw == 0)
            yw = 1;
    }
    else {
        xw = int(JPRE_SIZE*img.ratio);
        if (xw == 0)
            xw = 1;
        yw = JPRE_SIZE;
    }

    j_pre.setSize(xw, yw, JPRE_AAFACTOR);
    gtk_widget_set_size_request(j_pre.drawing_area, xw, yw);
}

void do_attr_dialog(void)
{
    if (img_attr_dlg)
        return;

    attr_dlg_new(&img_attr_dlg, &img);
    g_signal_connect(GTK_OBJECT(img_attr_dlg->ok_button), "clicked",
        GTK_SIGNAL_FUNC(image_attr_ok_cmd), img_attr_dlg);
    g_signal_connect(GTK_OBJECT(img_attr_dlg->apply_button), "clicked",
        GTK_SIGNAL_FUNC(image_attr_apply_cmd), img_attr_dlg);
}

void do_color_dialog(void)
{
    if (color_dlg)
        return;

    color_dlg_new(&color_dlg, &img);
}

void do_pal_rot_dialog(void)
{
    if (pal_rot_dlg)
        return;

    palette_rotation_dlg_new(&pal_rot_dlg, &img);
}

gint child_reaper(gpointer nothing)
{
    /* wait until all dead child processes are cleaned up */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    return TRUE;
}

void get_coords(double* x, double* y)
{
    if (y != NULL)
        *y = img.finfo.ymax - ((img.finfo.xmax - img.finfo.xmin) /
                               img.user_width) * (*y);

    if (x != NULL)
        *x = ((*x)/(double)img.user_width) *
            (img.finfo.xmax - img.finfo.xmin) + img.finfo.xmin;
}

void menu_add_item(GtkWidget* menu, GtkWidget* item)
{
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
}

GtkWidget* menu_add(GtkWidget* menu, const char* name, GCallback func,
    void* arg)
{
    GtkWidget* item;

    if (name != NULL) {
        item = gtk_menu_item_new_with_label(name);
        g_signal_connect_swapped(GTK_OBJECT(item), "activate", func, arg);
    } else
        /* just add a separator line to the menu */
        item = gtk_menu_item_new();

    menu_add_item(menu, item);

    return item;
}

void menu_bar_add(GtkWidget* menu, GtkWidget* submenu, const char* name)
{
    GtkWidget* temp = gtk_menu_item_new_with_mnemonic(name);
    gtk_widget_show(temp);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(temp), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), temp);
}

void create_menus(GtkWidget* vbox)
{
    GtkWidget* menu;
    GtkWidget* menu2;
    GtkWidget* menu_bar;

    menu_bar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
    gtk_widget_show(menu_bar);

    menu = gtk_menu_new();
    menu_add(menu, "Reset fractal", reset_fractal_cmd);
    menu_add(menu, NULL, NULL);
    menu_add(menu, "Save as PNG", save_cmd);
    menu_add(menu, NULL, NULL);
    menu_add(menu, "Exit", quit);
    menu_bar_add(menu_bar, menu, "_File");

    menu = gtk_menu_new();
    menu_add(menu, "Attributes...", do_attr_dialog);
    menu_add(menu, "Coloring...", do_color_dialog);
    menu_add(menu, NULL, NULL);
    switch_menu_cmd = menu_add(menu, "Switch fractal type",
                             switch_fractal_type);
    menu_bar_add(menu_bar, menu, "_Image");

    menu2 = gtk_menu_new();
    menu_add_item(menu2, gtk_tearoff_menu_item_new());

    for (int i = 0; i < palette_get_nr_of_builtins(); i++) {
        menu_add(menu2, palette_get_builtin_name(i),
            G_CALLBACK(load_builtin_palette_cmd),
            reinterpret_cast<void*>(i));
    }

    menu = gtk_menu_new();
    menu_add(menu, "Load", load_palette_cmd);
    menu_bar_add(menu, menu2, "Builtin");
    menu_add(menu, NULL, NULL);
    menu_add(menu, "Invert", invert);
    menu_add(menu, "Cycle...", do_pal_rot_dialog);
    menu_bar_add(menu_bar, menu, "_Palette");

    menu = gtk_hseparator_new();
    gtk_widget_show(menu);
    gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);
}

void set_tooltip(GtkWidget* w, const char* str)
{
    static GtkTooltips* tooltips = NULL;

    if (!tooltips)
    {
        tooltips = gtk_tooltips_new();
    }

    gtk_tooltips_set_tip(tooltips, w, str, NULL);

    // TODO: GTK 2.12 has a new tooltip API and has deprecated the old
    // one. Switch over to using that when it's widespread enough (Debian
    // Etch has GTK 2.8, as an example). In the new API you can simply do:
    //
    // gtk_widget_set_tooltip_text(tmp, str);
}

GtkWidget* get_stock_image(const char* stock_id)
{
    GtkWidget* w = gtk_image_new_from_stock(stock_id,
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(w);

    return w;
}

void create_threads(image_info* img)
{
    if (img->io_id == -1)
    {
        int fd[2];

        gf_a(pipe(fd) == 0);

        img->readFd = fd[0];
        img->writeFd = fd[1];

        img->ioc = g_io_channel_unix_new(img->readFd);
        gf_a(g_io_channel_set_encoding(img->ioc, NULL, NULL) ==
            G_IO_STATUS_NORMAL);

        img->io_id = g_io_add_watch(img->ioc,
            (GIOCondition)(G_IO_IN | G_IO_HUP), io_callback, img);
    }

    img->adjustThreads();

    for (int i = 0; i < img->real_height; i++)
    {
        img->wq.add(new RowWorkItem(i));
        img->lines_posted++;
    }
}

void start_rendering(image_info* img)
{
    stop_rendering(img);

    img->render_in_progress = true;

    if (!img->j_pre) {
        gtk_spin_button_update(GTK_SPIN_BUTTON(depth_spin));

        unsigned depth = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(depth_spin));

        if (depth != img->depth)
        {
            img->depth = depth;
            cfgNeedsSaving = true;
        }

        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar), 0.0);
        gtk_widget_show(pbar);
        gtk_label_set_text(GTK_LABEL(recalc_button_label), TEXT_STOP);
        timer_start(&timing_info);

        if (history_get_current() != img->finfo) {
            fhistory.push_back(new fractal_info(img->finfo));
            fhistory_current_pos = fhistory.size() - 1;
        }
    }

    create_threads(img);
}

void stop_rendering(image_info* img)
{
    if (img->render_in_progress && !img->stop_in_progress) {
        img->stop_in_progress = true;

        img->lines_posted -= img->wq.removeAll();

        gf_a(img->lines_posted >= img->lines_done);

        while (img->lines_done != img->lines_posted)
        {
            io_callback(img->ioc, G_IO_IN, img);
        }

        timer_stop(&timing_info);

        if (opts.timing && (img->lines_done == img->real_height))
        {
            printf("Image rendering took %.3f seconds.\n",
                   timer_get_elapsed(&timing_info) / (double)1e6);
        }

        if (!img->j_pre) {
            gtk_widget_hide(pbar);
            gtk_label_set_text(GTK_LABEL(recalc_button_label), TEXT_RECALC);
        }

        img->render_in_progress = false;
        img->stop_in_progress = false;

        img->lines_done = 0;
        img->lines_posted = 0;
        img->clearLinesDone();
    }
}

void start_julia_browsing(void)
{
    st.julia_browsing = true;
    gtk_widget_show(j_pre_window);
    gtk_widget_set_sensitive(switch_menu_cmd, FALSE);
}

void stop_julia_browsing(void)
{
    stop_rendering(&j_pre);
    st.julia_browsing = false;
    gtk_widget_hide(j_pre_window);
    gtk_widget_set_sensitive(switch_menu_cmd, TRUE);
}

gint j_pre_delete(GtkWidget* widget, GdkEvent* event, gpointer data)
{
    stop_julia_browsing();

    return TRUE;
}

void draw_xor_rect(const GdkRectangle& rect)
{
    gdk_gc_set_function(drawing_area->style->white_gc, GDK_XOR);

    gdk_draw_rectangle(drawing_area->window, drawing_area->style->white_gc,
                       FALSE, rect.x, rect.y, rect.width, rect.height);

    gdk_gc_set_function(drawing_area->style->white_gc, GDK_COPY);
}

void zoom_in_func(GtkWidget* widget)
{
    tool_activate(tool_zoom_in);
}

void zoom_out_func(GtkWidget* widget)
{
    tool_activate(tool_zoom_out);
}

void recalc_button(GtkWidget* widget)
{
    if (!img.render_in_progress)
        start_rendering(&img);
    else
        stop_rendering(&img);
}

void toggle_palette_ip(GtkWidget* widget)
{
    img.palette_ip = GTK_TOGGLE_BUTTON(widget)->active;
    reapply_palette();
}

gint button_press_event(GtkWidget* widget, GdkEventButton* event)
{
    /* ignore double- and triple clicks */
    if ((event->type == GDK_2BUTTON_PRESS) ||
        (event->type == GDK_3BUTTON_PRESS))
    {
        return TRUE;
    }

    if (tool_active)
    {
        tool_active->buttonEvent(Tool::fromGDKButtonType(event->button),
                                 true, int(event->x), int(event->y));
    }

    if (st.julia_browsing) {
        if (event->button == 1) {
            img.finfo.u.julia.c_re = event->x;
            img.finfo.u.julia.c_im = event->y;

            get_coords(&img.finfo.u.julia.c_re, &img.finfo.u.julia.c_im);
            stop_julia_browsing();

            /* save old coordinates */
            img.finfo.old_xmin = img.finfo.xmin;
            img.finfo.old_xmax = img.finfo.xmax;
            img.finfo.old_ymax = img.finfo.ymax;

            img.finfo.xmin = j_pre.finfo.xmin;
            img.finfo.xmax = j_pre.finfo.xmax;
            img.finfo.ymax = j_pre.finfo.ymax;
            img.finfo.type = JULIA;

            start_rendering(&img);
        }
    }

    return TRUE;
}

gint button_release_event(GtkWidget* widget, GdkEventButton* event)
{
    if (tool_active)
    {
        tool_active->buttonEvent(Tool::fromGDKButtonType(event->button),
                                 false, int(event->x), int(event->y));
    }

    return TRUE;
}

gint motion_event(GtkWidget* widget, GdkEventMotion* event)
{
    if (tool_active)
    {
        tool_active->moveEvent(int(event->x), int(event->y));
    }

    // FIXME: julia browsing should be a Tool (it interacts very badly if
    // you try to do that and zooming in at the same time, for example)

    if (st.julia_browsing) {
        j_pre.finfo.u.julia.c_re = event->x;
        j_pre.finfo.u.julia.c_im = event->y;
        get_coords(&j_pre.finfo.u.julia.c_re, &j_pre.finfo.u.julia.c_im);
        start_rendering(&j_pre);
    }

    return TRUE;
}

gint expose_event(GtkWidget* widget, GdkEventExpose* event,
                  image_info* img)
{
    GdkRectangle pic_area, padding_area,tmp;

    /* sometimes gtk gives us invalid exposes when we're changing
       image size, and when that happens, draw_image below segfaults
       when it's trying to access rgb_data at offsets way past the
       current image size.

       so check for that and don't do anything on invalid exposes. */
    if ((event->area.y + event->area.height - 1) >
        (img->user_height - 1)) {
        return TRUE;
    }

    tmp.x = 0;
    tmp.width = img->user_width;
    pic_area = horiz_intersect(&event->area, &tmp);

    tmp.x = img->user_width;
    tmp.width = USHRT_MAX;
    padding_area = horiz_intersect(&event->area, &tmp);

    if (pic_area.width != 0) {
        gdk_draw_rgb_32_image(widget->window,
            widget->style->white_gc,
            pic_area.x, pic_area.y,
            pic_area.width, pic_area.height,
            GDK_RGB_DITHER_NONE,
            (uint8_t*)&(img->rgb_data[pic_area.y * img->user_width +
                    pic_area.x]),
            img->user_width * 4);
    }

    if (padding_area.width != 0) {
        gdk_draw_rectangle(widget->window,
                           widget->style->white_gc,
                           TRUE,
                           padding_area.x, padding_area.y,
                           padding_area.width, padding_area.height);
    }

    return TRUE;
}

gint key_event(GtkWidget* widget, GdkEventKey* event)
{
    if (tool_active && tool_active->keyEvent(event))
    {
        return TRUE;
    }

    int ret = FALSE;

    switch (event->keyval)
    {
    case GDK_Page_Up:
        history_goto(fhistory_current_pos - 1);
        ret = TRUE;
        break;

    case GDK_Page_Down:
        history_goto(fhistory_current_pos + 1);
        ret = TRUE;
        break;
    }

    return ret;
}

gboolean io_callback(GIOChannel* source, GIOCondition condition, gpointer data)
{
    image_info* img = reinterpret_cast<image_info*>(data);
    char tmpCh;

    gf_a(condition == G_IO_IN);
    gf_a(read(img->readFd, &tmpCh, 1) == 1);

    int rowsDone = 0;

    while (1)
    {
        int row = -1;

        {
            Locker l(&img->rows_completed_mutex);

            if (img->rows_completed.empty())
            {
                break;
            }

            row = img->rows_completed.front();
            img->rows_completed.pop_front();
        }

        gf_a(row >= 0);
        gf_a(row < img->real_height);
        gf_a(!img->lines_done_vec.at(row));

        img->lines_done++;
        img->lines_done_vec.at(row) = true;

        rowsDone++;

        if (img->aa_factor > 1) {
            int userRow = row / img->aa_factor;

            if (img->isAALineComplete(userRow))
            {
                do_anti_aliasing(img, 0, userRow, img->user_width, 1);

                // to get the correct expose below
                row = userRow;
            }
            else
            {
                continue;
            }
        }

        GdkRectangle rect;

        rect.x = 0;
        rect.width = img->user_width;
        rect.y = row;
        rect.height = 1;

        gdk_window_invalidate_rect(img->drawing_area->window, &rect, TRUE);
    }

    if (rowsDone == 0)
    {
        return TRUE;
    }

    if (!img->j_pre)
    {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar),
            (float)img->lines_done / img->real_height);
    }

    if (img->lines_done == img->real_height)
    {
        stop_rendering(img);
    }

    return TRUE;
}

void quit(void)
{
    cfg_saver(NULL);

    stop_rendering(&img);
    stop_rendering(&j_pre);

    img.stopThreads();
    j_pre.stopThreads();

    gtk_main_quit();
}

int find_palette_by_name(const std::string& name, bool must_exist)
{
    for (int i = 0; i < palette_get_nr_of_builtins(); i++) {
        if (name == palette_get_builtin_name(i)) {
            return i;
        }
    }

    if (must_exist)
    {
        throw std::invalid_argument("can't find palette " + name);
    }
    else
    {
        return -1;
    }
}

gint cfg_saver(gpointer nothing)
{
    if (cfgNeedsSaving)
    {
        save_config();
    }

    return TRUE;
}

void save_config()
{
    try
    {
        Config cfg;

        img.save(&cfg, sectionMisc);
        cfg.setStr(sectionMisc, keyPalette, palette_get_current_name());

        cfg.saveToFile(cfgFilename);

        cfgNeedsSaving = false;
    }
    catch (const GfractException& e)
    {
        show_msg_box(window, strf("Error saving config file '%s': %s",
                         cfgFilename.c_str(), e.what()));
    }
}

void load_config()
{
    using namespace boost::filesystem;

    if (!exists(path(cfgFilename, native)))
    {
        return;
    }

    try
    {
        Config cfg;
        cfg.loadFromFile(cfgFilename);

        img.load(&cfg, sectionMisc);

        std::string palName = cfg.getStr(sectionMisc, keyPalette, "");

        if (!palName.empty())
        {
            std::string err;

            if (palName[0] == '/')
            {
                if (!palette_load(palName.c_str()))
                {
                    err = "Could not load palette file " + palName;
                }

            }
            else
            {
                int idx = find_palette_by_name(palName, false);

                if (idx != -1)
                {
                    palette_load_builtin(idx);
                }
                else
                {
                    err = "Could not load built-in palette " + palName;
                }
            }

            if (!err.empty())
            {
                show_msg_box(window, "Error: " + err);
            }
        }
    }
    catch (const GfractException& e)
    {
        show_msg_box(NULL, strf("Error loading config file '%s': %s",
                         cfgFilename.c_str(), e.what()));
    }
}

int main(int argc, char** argv)
{
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* button;
    GtkWidget* tmp;
    GtkObject* adj;

    cfgFilename = std::string(getenv("HOME")) + "/.gfractrc";

    gtk_init(&argc, &argv);

    init_misc();

    palette_load_builtin(find_palette_by_name("blues", true));

    load_config();

    process_args(argc, argv);

    g_timeout_add(10*1000, child_reaper, NULL);
    g_timeout_add(10*1000, cfg_saver, NULL);

    img.setSize(img.user_width, img.user_height, img.aa_factor);
    j_pre.setSize(JPRE_SIZE, int(JPRE_SIZE / img.ratio), JPRE_AAFACTOR);

    j_pre.nr_threads = 1;

    /* main window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_widget_realize(window);

    /* preview window */
    j_pre_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(GTK_OBJECT(j_pre_window), "delete_event",
                     GTK_SIGNAL_FUNC(j_pre_delete), NULL);
    gtk_window_set_title(GTK_WINDOW(j_pre_window), "Preview");
    gtk_window_set_resizable(GTK_WINDOW(j_pre_window), FALSE);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    create_menus(vbox);

    g_signal_connect(GTK_OBJECT(window), "destroy",
                     GTK_SIGNAL_FUNC(quit), NULL);

    /* toolbar stuff */
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* zoom in */
    tmp = gtk_toggle_button_new();
    set_tooltip(
        tmp,
        "Zoom in.\n\n"
        "Left mouse button = increase size\n"
        "Middle mouse button/Return = zoom\n"
        "Right mouse button = decrease size");

    gtk_container_add(GTK_CONTAINER(tmp), get_stock_image(GTK_STOCK_ZOOM_IN));
    g_signal_connect(GTK_OBJECT(tmp), "toggled",
                     GTK_SIGNAL_FUNC(zoom_in_func), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_button_set_relief(GTK_BUTTON(tmp), GTK_RELIEF_NONE);
    GTK_WIDGET_UNSET_FLAGS(tmp, GTK_CAN_FOCUS);
    gtk_widget_show(tmp);
    tool_zoom_in = new ZoomInTool(&img, tmp);

    /* zoom out */
    tmp = gtk_button_new();
    set_tooltip(tmp, "Zoom out.");
    gtk_container_add(GTK_CONTAINER(tmp),
                      get_stock_image(GTK_STOCK_ZOOM_OUT));
    g_signal_connect(GTK_OBJECT(tmp), "clicked",
                     GTK_SIGNAL_FUNC(zoom_out_func), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_button_set_relief(GTK_BUTTON(tmp), GTK_RELIEF_NONE);
    GTK_WIDGET_UNSET_FLAGS(tmp, GTK_CAN_FOCUS);
    gtk_widget_show(tmp);
    tool_zoom_out = new ZoomOutTool(&img);

    /* depth label */
    button = gtk_label_new("Depth");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    /* depth spin-button */
    adj = gtk_adjustment_new(img.depth, 1.0, 2147483647.0, 1, 100, 0.0);
    depth_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), depth_spin, FALSE, FALSE, 0);
    gtk_widget_show(depth_spin);

    /* palette interpolation */
    palette_ip = gtk_check_button_new_with_label("Palette interpolation");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(palette_ip),
                                 img.palette_ip);
    gtk_box_pack_start(GTK_BOX(hbox), palette_ip, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(palette_ip), "toggled",
                     GTK_SIGNAL_FUNC(toggle_palette_ip), NULL);
    gtk_widget_show(palette_ip);

    /* recalc button */
    button = gtk_button_new();
    recalc_button_label = gtk_label_new(TEXT_STOP);
    gtk_misc_set_alignment(GTK_MISC(recalc_button_label), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), recalc_button_label);
    gtk_widget_show(recalc_button_label);
    g_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(recalc_button), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    /* progress bar */
    pbar = gtk_progress_bar_new();
    gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
                                     GTK_PROGRESS_LEFT_TO_RIGHT);
    gtk_box_pack_end(GTK_BOX(hbox), pbar, FALSE, FALSE, 0);

    /* separator */
    button = gtk_hseparator_new();
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    /* main window drawing area */
    tmp = gtk_drawing_area_new();
    GTK_WIDGET_SET_FLAGS(tmp, GTK_CAN_FOCUS);
    gtk_widget_set_events(
        tmp, GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK);
    gtk_widget_set_size_request(
        tmp, (img.user_width >= MIN_WINDOW_WIDTH) ?
        img.user_width : MIN_WINDOW_WIDTH, img.user_height);
    gtk_box_pack_start(GTK_BOX(vbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    g_signal_connect(GTK_OBJECT(tmp), "key_press_event",
                     GTK_SIGNAL_FUNC(key_event), NULL);
    g_signal_connect(GTK_OBJECT(tmp), "button_press_event",
                     (GtkSignalFunc)button_press_event, NULL);
    g_signal_connect(GTK_OBJECT(tmp), "button_release_event",
                     (GtkSignalFunc)button_release_event, NULL);
    g_signal_connect(GTK_OBJECT(tmp), "expose_event",
                     GTK_SIGNAL_FUNC(expose_event), &img);
    g_signal_connect(GTK_OBJECT(tmp), "motion_notify_event",
                     GTK_SIGNAL_FUNC(motion_event), NULL);
    img.drawing_area = drawing_area = tmp;

    /* preview window drawing area */
    tmp = gtk_drawing_area_new();
    gtk_widget_set_events(tmp, GDK_EXPOSURE_MASK);
    gtk_widget_set_size_request(tmp, j_pre.user_width, j_pre.user_height);
    gtk_container_add(GTK_CONTAINER(j_pre_window), tmp);
    g_signal_connect(GTK_OBJECT(tmp), "expose_event",
                     GTK_SIGNAL_FUNC(expose_event), &j_pre);
    gtk_widget_show(tmp);
    j_pre.drawing_area = tmp;

    start_rendering(&img);
    gtk_widget_show(window);

    gtk_main();

    return 0;
}
