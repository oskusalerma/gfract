#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include "externs.h"
#include "fractal_types.h"
#include "palette.h"
#include "misc.h"
#include "attr_dlg.h"
#include "pal_rot_dlg.h"
#include "my_png.h"
#include "timer.h"
#include "version.h"
#include "zoom_in.xpm"
#include "zoom_out.xpm"

#define ZOOM_INTERVAL 25

/* percentage of image width that zoom box starts at */
#define ZOOM_BOX_WIDTH 0.35

#define DEFAULT_WIDTH    480
#define DEFAULT_HEIGHT   360
#define DEFAULT_AAFACTOR 1

#define MIN_WINDOW_WIDTH 320

/* recalc/stop button texts */
#define TEXT_RECALC     "Recalc"
#define TEXT_STOP       "Stop"

/* julia preview size */
#define JPRE_SIZE    160
#define JPRE_AAFACTOR 2


#define DUMP_COMMAND "--read-dump-from"
#define DEFAULT_PALETTE_FILE "/usr/local/share/gfract/palettes/blues.map"

/* why the fuck does gtk have to define these in its header files?
   the glib/gdk/gtk combo is forgetting seriously that it's just a Yet
   Another Widget Set, not a toolkit for every conceivable function
   you might someday have a need for... */
#undef MIN
#undef MAX

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

typedef struct {
    int zooming;
    int julia_browsing;

    /* zoom box info */
    int z_x;
    int z_y;
    int z_width;
    int z_height;
} status_info;

/* options */
typedef struct {
    /* display amount of time needed for rendering */
    int timing;
} options;

static gint idle_callback(image_info* img);
static void start_rendering(image_info* img);
static void stop_rendering(image_info* img);
static void start_julia_browsing(void);
static void stop_julia_browsing(void);
static void process_args(int argc, char** argv);
static void kill_zoom_timers(void);
static void zoom_resize(int arg);
static void resize_preview(void);
static int zoom_is_valid_size(void);
static void quit(void);
static void redraw_image(image_info* img);
static void create_menus(GtkWidget* vbox);
static GtkWidget* menu_add(GtkWidget* menu, char* name, void* func);
static void get_coords(double* x, double* y);
static GdkRectangle horiz_intersect(GdkRectangle* a1, GdkRectangle* a2);
static void my_fread(void* ptr, int size, FILE* fp);
static void my_fwrite(void* ptr, int size, FILE* fp);
static GtkWidget* create_pixmap(GtkWidget* widget, char** xpm_data);

static gint expose_event(GtkWidget* widget, GdkEventExpose* event,
                         image_info* img);
static gint key_event(GtkWidget* widget, GdkEventKey* event);
static gint button_press_event(GtkWidget* widget, GdkEventButton* event);
static gint j_pre_delete(GtkWidget *widget, GdkEvent *event, gpointer data);
static gint child_reaper(gpointer nothing);

static void invert(void);
static void switch_fractal_type(void);
static void print_help(void);
static void print_version(void);
     
/* DIALOG FUNCTIONS */

/* image attribute */
static void image_attr_ok_cmd(GtkWidget* w, image_attr_dialog* dl);
static void do_attr_dialog(void);

/* palette loading */
void reapply_palette(void);
static void load_palette_cmd(void);
static void palette_apply_cmd(GtkWidget* w, GtkFileSelection* fs);
static void palette_ok_cmd(GtkWidget* w, GtkFileSelection* fs);

/* palette cycling */
static void do_pal_rot_dialog(void);

/* palette saving */
static void save_cmd(void);


/* general stuff we need to have */
static status_info st;
static image_info img;
static image_info j_pre;
static Timer timing_info;
static options opts;
static GtkWidget* j_pre_window = NULL;
static GtkWidget* window = NULL;
static GtkWidget* drawing_area = NULL;
static GtkWidget* zoom_in_button = NULL;
static GtkWidget* zoom_out_button = NULL;
static GtkWidget* recalc_button_label = NULL;
static GtkWidget* depth_spin = NULL;
static GtkWidget* palette_ip = NULL;
static GtkWidget* pbar = NULL;
static GtkWidget* switch_menu_cmd = NULL;
static int zoom_timer;
static char* program_name = NULL;


/* DIALOG POINTERS */

static image_attr_dialog* img_attr_dlg = NULL;
static palette_rotation_dialog* pal_rot_dlg = NULL;



void my_fread(void* ptr, int size, FILE* fp)
{
    if (fread(ptr, size, 1, fp) != 1) {
        perror("Can't read file");
        exit(1);
    }
}

void my_fwrite(void* ptr, int size, FILE* fp)
{
    if (fwrite(ptr, size, 1, fp) != 1) {
        perror("Can't write to file");
        exit(1);
    }
}

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
        } else if (strcmp(DUMP_COMMAND, argv[i]) == 0) {
            if ((i+1) == argc) {
                fprintf(stderr, "Filename missing from command line.\n");
                exit(1);
            } else {
                FILE* fp = fopen(argv[i+1], "r");
                if (fp == NULL) {
                    perror("Can't open file");
                    exit(1);
                }
                my_fread(&img, sizeof(image_info), fp);
                my_fread(palette, 256*4, fp);
                fclose(fp);
                
                if (remove(argv[i+1]) == -1) {
                    perror("Can't delete temp file");
                    exit(1);
                }
                img.idle_id = -1;
                img.rgb_data = NULL;
                img.raw_data = NULL;
                i++;
            }
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            exit(1);
        }
    }
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
    do_png_save(&img);
}

void switch_fractal_type(void)
{
    if (img.fr_type == MANDELBROT) {
        if (!st.julia_browsing)
            start_julia_browsing();
    } else if (img.fr_type == JULIA) {
        img.xmin = img.old_xmin;
        img.xmax = img.old_xmax;
        img.ymax = img.old_ymax;
        img.fr_type = MANDELBROT;
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

void palette_apply_cmd(GtkWidget* w, GtkFileSelection* fs)
{
    if (palette_load(gtk_file_selection_get_filename(fs)) == FALSE) {
        fprintf(stderr, "Invalid palette file %s\n",
                gtk_file_selection_get_filename(fs));
    } else {
        reapply_palette();
    }
}

void palette_ok_cmd(GtkWidget* w, GtkFileSelection* fs)
{
    palette_apply_cmd(w,fs);
    gtk_widget_destroy(GTK_WIDGET(fs));
}

void load_palette_cmd(void)
{
    GtkWidget* file_sel;
    GtkWidget* button;
    
    file_sel = gtk_file_selection_new("Load palette");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_sel));
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_sel),
                                    palette_get_filename());
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_sel)->ok_button),
                       "clicked", GTK_SIGNAL_FUNC(palette_ok_cmd),
                       file_sel);
    gtk_signal_connect_object(GTK_OBJECT
                              (GTK_FILE_SELECTION(file_sel)->cancel_button),
                              "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
                              GTK_OBJECT(file_sel));
    
    button = gtk_button_new_with_label("Apply palette");
    gtk_box_pack_start(GTK_BOX(GTK_FILE_SELECTION(file_sel)->action_area),
                       button, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(palette_apply_cmd), file_sel);
    gtk_widget_show(button);
    
    gtk_widget_show(file_sel);
}

void init_misc(void)
{
    /* main window init */
    img.real_width = img.user_width = DEFAULT_WIDTH;
    img.real_height = img.user_height = DEFAULT_HEIGHT;
    img.aa_factor = DEFAULT_AAFACTOR;
    
    img.depth = 300;
    img.rgb_data = NULL;
    img.raw_data = NULL;

    img.xmin = -2.21;
    img.xmax = 1.0;
    img.ymax = 1.2;

    img.idle_id = -1;
    img.j_pre = FALSE;
    img.fr_type = MANDELBROT;
    img.palette_ip = 0;

    img.cops_nr = 1;
    img.cops = malloc(img.cops_nr * sizeof(color_op));
    int i = 0;
    img.cops[i++].type = ITER;

    /* init preview */
    j_pre.depth = 300;
    j_pre.rgb_data = NULL;
    j_pre.raw_data = NULL;

    j_pre.xmin = -2.0;
    j_pre.xmax = 1.5;
    j_pre.ymax = 1.25;

    j_pre.u.julia.c_re = 0.3;
    j_pre.u.julia.c_im = 0.6;
    
    j_pre.idle_id = -1;
    j_pre.j_pre = TRUE;
    j_pre.fr_type = JULIA;
    img.palette_ip = 0;

    j_pre.cops_nr = 1;
    j_pre.cops = malloc(sizeof(color_op));
    j_pre.cops->type = ITER;
    
    /* misc init */
    st.zooming = FALSE;
    st.julia_browsing = FALSE;
    zoom_timer = -1;

    /* default values for options */
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

    cx1 = MAX(ax1,bx1);
    cx2 = MIN(ax2,bx2);

    /* no overlap */
    if (cx2 < cx1)
        return tmp;

    tmp.x = cx1;
    tmp.width = cx2-cx1+1;
        
    return tmp;
}

void redraw_image(image_info* img)
{
    gdk_draw_rgb_32_image(img->drawing_area->window,
                          img->drawing_area->style->white_gc,
                          0, 0, img->user_width, img->user_height,
                          GDK_RGB_DITHER_NONE,
                          (guchar*)img->rgb_data,
                          img->user_width*4);
}

gint do_palette_rotation(gboolean forward)
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

void image_attr_ok_cmd(GtkWidget* w, image_attr_dialog* dl)
{
    set_image_info(&img,
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dl->width)),
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dl->height)),
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dl->aa)));

    if (img.user_width < MIN_WINDOW_WIDTH) {
        /* limit minimum window width to MIN_WINDOW_WIDTH and handle
           this case in the expose event */
        gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area),
                              MIN_WINDOW_WIDTH, img.user_height);
    } else
        gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area),
                              img.user_width, img.user_height);
        
    start_rendering(&img);

    resize_preview();
    if (st.julia_browsing) {
        gtk_widget_hide(j_pre_window);
        gtk_widget_show(j_pre_window);
        start_rendering(&j_pre);
    }
    
    gtk_widget_destroy(dl->dialog);
}

/* change preview window to reflect possible new aspect ratio */
void resize_preview(void)
{
    int xw,yw;
    
    if (JPRE_SIZE/img.ratio < JPRE_SIZE) {
        xw = JPRE_SIZE;
        yw = JPRE_SIZE/img.ratio;
        if (yw == 0)
            yw = 1;
    }
    else {
        xw = JPRE_SIZE*img.ratio;
        if (xw == 0)
            xw = 1;
        yw = JPRE_SIZE;
    }
    
    set_image_info(&j_pre, xw, yw, JPRE_AAFACTOR);
    gtk_drawing_area_size(GTK_DRAWING_AREA(j_pre.drawing_area),
                          xw, yw);
}


void do_attr_dialog(void)
{
    if (img_attr_dlg)
        return;

    attr_dlg_new(&img_attr_dlg, &img);
    gtk_signal_connect(GTK_OBJECT(img_attr_dlg->ok_button), "clicked",
                       GTK_SIGNAL_FUNC(image_attr_ok_cmd),
                       img_attr_dlg);
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

void duplicate(void)
{
    char fname[] = "/tmp/gfractXXXXXX";
    int fd;
    FILE* fp;
    pid_t result;
    
    fd = mkstemp(fname);
    if (fd == -1) {
        perror("Can't create temp file");
        exit(1);
    }
    
    fp = fdopen(fd, "w+");
    if (fp == NULL) {
        perror("Can't create temp file");
        exit(1);
    }

    my_fwrite(&img, sizeof(image_info), fp);
    my_fwrite(palette, 256*4, fp);
    
    if (fclose(fp) != 0) {
        perror("Error writing temp file");
        exit(1);
    }

    result = fork();
    
    if (result == 0) {
        close(ConnectionNumber(gdk_display));
        execl(program_name, program_name, DUMP_COMMAND,
              fname, NULL);
        perror("Error while exec'ing program");
        exit(1);
    }
    else if (result < 0) {
        perror("Could not fork");
        exit(1);
    }
}

void get_coords(double* x, double* y)
{
    if (y != NULL)
        *y = img.ymax - ((img.xmax - img.xmin)/(double)img.user_width)
            * (*y);
    
    if (x != NULL)
        *x = ((*x)/(double)img.user_width) *
            (img.xmax - img.xmin) + img.xmin;
}

GtkWidget* menu_add(GtkWidget* menu, char* name, void* func)
{
    GtkWidget* item;

    if (name != NULL) {
        item = gtk_menu_item_new_with_label(name);
        gtk_signal_connect_object(GTK_OBJECT(item), "activate",
                                  GTK_SIGNAL_FUNC(func), NULL);
    } else
        /* just add a separator line to the menu */
        item = gtk_menu_item_new();

    gtk_menu_append(GTK_MENU(menu), item);
    gtk_widget_show(item);

    return item;
}

void menu_bar_add(GtkWidget* menubar, GtkWidget* submenu, char* name)
{
    GtkWidget* temp = gtk_menu_item_new_with_label(name);
    gtk_widget_show(temp);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(temp), submenu);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), temp);
}

void create_menus(GtkWidget* vbox)
{
    GtkWidget* menu;
    GtkWidget* menu_bar;

    menu_bar = gtk_menu_bar_new();
    gtk_menu_bar_set_shadow_type(GTK_MENU_BAR(menu_bar), GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
    gtk_widget_show(menu_bar);

    menu = gtk_menu_new();
    menu_add(menu, "Save as PNG", save_cmd);
    menu_add(menu, NULL, NULL);
    menu_add(menu, "Duplicate", duplicate);
    menu_add(menu, NULL, NULL);
    menu_add(menu, "Exit", quit);
    menu_bar_add(menu_bar, menu, "File");
    
    menu = gtk_menu_new();
    menu_add(menu, "Attributes...", do_attr_dialog);
    menu_add(menu, NULL, NULL);
    switch_menu_cmd = menu_add(menu, "Switch fractal type",
                             switch_fractal_type);
    menu_bar_add(menu_bar, menu, "Image");

    menu = gtk_menu_new();
    menu_add(menu, "Load", load_palette_cmd);
    menu_add(menu, NULL, NULL);
    menu_add(menu, "Invert", invert);
    menu_add(menu, "Cycle...", do_pal_rot_dialog);
    menu_bar_add(menu_bar, menu, "Palette");
    
    menu = gtk_hseparator_new();
    gtk_widget_show(menu);
    gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);
}

GtkWidget* create_pixmap(GtkWidget* widget, char** xpm_data)
{
    GdkPixmap* pixmap;
    GdkBitmap* mask;
    GtkWidget* pwid;

    pixmap = gdk_pixmap_create_from_xpm_d(widget->window, &mask,
             &(gtk_widget_get_style(widget)->bg[GTK_STATE_NORMAL]),
             (gchar**)xpm_data);
    g_assert(pixmap != NULL);
    pwid = gtk_pixmap_new(pixmap, mask);
    gtk_widget_show(pwid);

    return pwid;
}

void start_rendering(image_info* img)
{
    img->lines_done = 0;
    stop_rendering(img);

    if (!img->j_pre) {
        gtk_spin_button_update(GTK_SPIN_BUTTON(depth_spin));
        img->depth =
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(depth_spin));
        
        gtk_progress_configure(GTK_PROGRESS(pbar), 0.0, 0.0,
                               (gfloat)img->real_height);
        gtk_widget_show(pbar);
        gtk_label_set_text(GTK_LABEL(recalc_button_label), TEXT_STOP);
        timer_start(&timing_info);
    }
    img->idle_id = gtk_idle_add((GtkFunction)idle_callback, img);
}

void stop_rendering(image_info* img)
{
    if (img->idle_id != -1) {
        timer_stop(&timing_info);
        if (opts.timing && (img->lines_done == img->real_height))
            printf("Image rendering took %.3f seconds.\n",
                   timer_get_elapsed(&timing_info) / (double)1e6);
        gtk_idle_remove(img->idle_id);
        img->idle_id = -1;
        if (!img->j_pre) {
            gtk_widget_hide(pbar);
            gtk_label_set_text(GTK_LABEL(recalc_button_label), TEXT_RECALC);
        }
    }
}

void start_julia_browsing(void)
{
    st.julia_browsing = TRUE;
    gtk_widget_show(j_pre_window);
    gtk_widget_set_sensitive(switch_menu_cmd, FALSE);
}

void stop_julia_browsing(void)
{
    stop_rendering(&j_pre);
    st.julia_browsing = FALSE;
    gtk_widget_hide(j_pre_window);
    gtk_widget_set_sensitive(switch_menu_cmd, TRUE);
}

gint j_pre_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    stop_julia_browsing();
    
    return TRUE;
}

void draw_zoom_box(void)
{
    gdk_gc_set_function(drawing_area->style->white_gc, GDK_XOR);

    gdk_draw_rectangle(drawing_area->window, drawing_area->style->white_gc,
                       FALSE, st.z_x, st.z_y, st.z_width,
                       st.z_height);
    
    gdk_gc_set_function(drawing_area->style->white_gc, GDK_COPY);
}

void start_zooming(void)
{
    st.z_x = 0;
    st.z_y = 0;
    st.z_width = ZOOM_BOX_WIDTH * img.user_width;
    st.z_height = st.z_width/img.ratio;
    st.zooming = TRUE;
    
    draw_zoom_box();
}

void stop_zooming(void)
{
    st.zooming = FALSE;
    draw_zoom_box();
    kill_zoom_timers();
}

void zoom_in(void)
{
    double xmin,xmax,ymax;

    st.zooming = FALSE;
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(zoom_in_button),
                                FALSE);

    ymax = (double)st.z_y;
    xmin = (double)st.z_x;
    xmax = (double)(st.z_x+st.z_width-1);
    
    get_coords(&xmin, &ymax);
    get_coords(&xmax, NULL);
    
    img.ymax = ymax;
    img.xmin = xmin;
    img.xmax = xmax;

    start_rendering(&img);
}

void zoom_resize(int arg)
{
    st.z_width += arg*4;
    st.z_height = st.z_width/img.ratio;
}

int zoom_is_valid_size(void)
{
    if ( (st.z_width < 4) || (st.z_width > (img.user_width-16))
         || (st.z_height < 4) || (st.z_height > (img.user_height-16))
         )
        return FALSE;
    else
        return TRUE;
}

gint zoom_callback(int arg)
{
    draw_zoom_box();

    zoom_resize(arg);
    if (!zoom_is_valid_size())
        zoom_resize(-1*arg);

    draw_zoom_box();

    return TRUE;
}

void zoom_in_func(GtkWidget* widget)
{
    if (GTK_TOGGLE_BUTTON(widget)->active) {
        start_zooming();
    } else {
        stop_zooming();
    }
}

void zoom_out_func(GtkWidget* widget)
{
    double ymin,half_w,half_h;
        
    ymin = img.ymax - ((img.xmax - img.xmin)/(double)img.real_width)
        * (double)(img.real_height-1);

    half_w = (img.xmax-img.xmin)/2.0;
    half_h = (img.ymax-ymin)/2.0;
    
    img.ymax += half_h;
    img.xmin -= half_w;
    img.xmax += half_w;
    
    start_rendering(&img);
}

void recalc_button(GtkWidget* widget)
{
    if (img.idle_id == -1)
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
    if ( (event->type == GDK_2BUTTON_PRESS) ||
         (event->type == GDK_3BUTTON_PRESS) )
        return TRUE;

    /* don't react to pressing the other button if we're
       zooming in or out */
    if ( (zoom_timer != -1) && ( (event->button == 1) ||
                                 (event->button == 3) ) )
        return TRUE;
    
    if (st.zooming) {
        draw_zoom_box();
        if (event->button == 1) {
            zoom_resize(1);
            if (!zoom_is_valid_size())
                zoom_resize(-1);
            else
                zoom_timer = gtk_timeout_add(ZOOM_INTERVAL,
                                           (GtkFunction)zoom_callback,
                                           (gpointer)2);
        } else if (event->button == 2)
            zoom_in();
        else if (event->button == 3) {
            zoom_resize(-1);
            if (!zoom_is_valid_size())
                zoom_resize(1);
            else
                zoom_timer = gtk_timeout_add(ZOOM_INTERVAL,
                                           (GtkFunction)zoom_callback,
                                           (gpointer)-2);
                
        }
        draw_zoom_box();
    } else if (st.julia_browsing) {
        if (event->button == 1) {
            img.u.julia.c_re = event->x;
            img.u.julia.c_im = event->y;
            
            get_coords(&img.u.julia.c_re, &img.u.julia.c_im);
            stop_julia_browsing();

            /* save old coordinates */
            img.old_xmin = img.xmin;
            img.old_xmax = img.xmax;
            img.old_ymax = img.ymax;
            
            img.xmin = j_pre.xmin;
            img.xmax = j_pre.xmax;
            img.ymax = j_pre.ymax;
            img.fr_type = JULIA;
            
            start_rendering(&img);
        }
    }

    return TRUE;
}

void kill_zoom_timers(void)
{
    if (zoom_timer != -1) {
        gtk_timeout_remove(zoom_timer);
    }
    zoom_timer = -1;
}

gint button_release_event(GtkWidget* widget, GdkEventButton* event)
{
    kill_zoom_timers();
    
    return TRUE;
}

gint motion_event(GtkWidget* widget, GdkEventMotion* event)
{
    if ((!st.zooming) && (!st.julia_browsing))
        return TRUE;

    if (st.julia_browsing) {
        j_pre.u.julia.c_re = event->x;
        j_pre.u.julia.c_im = event->y;
        get_coords(&j_pre.u.julia.c_re, &j_pre.u.julia.c_im);
        start_rendering(&j_pre);
    } else if (st.zooming) {
        draw_zoom_box();
        
        st.z_x = event->x;
        st.z_y = event->y;
        
        draw_zoom_box();
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
                              (guchar*)&(img->rgb_data[pic_area.y*
                                                      img->user_width
                                                      + pic_area.x]),
                              img->user_width*4);
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
    switch (event->keyval) {
    case GDK_Escape:
        if (st.julia_browsing)
            stop_julia_browsing();
        else
            stop_rendering(&img);
        break;
    }
    
    return TRUE;
}

gint idle_callback(image_info* img)
{
    int y_offset;

    if (img->aa_factor == 1) {
        fractal_next_line(img);
        palette_apply(img, 0, img->lines_done-1, img->real_width, 1);
    } else {
        int i;

        for (i=0; i < img->aa_factor; i++)
            fractal_next_line(img);
        do_anti_aliasing(img, 0, img->lines_done/img->aa_factor-1,
                         img->user_width, 1);
    }

    y_offset = img->lines_done/img->aa_factor-1;
    gdk_draw_rgb_32_image(img->drawing_area->window,
                          img->drawing_area->style->white_gc,
                          0, y_offset, img->user_width, 1,
                          GDK_RGB_DITHER_NONE,
                          (guchar*)
                          (&(img->rgb_data[img->user_width*y_offset])),
                          img->user_width*4);

    if (!img->j_pre)
        gtk_progress_set_value(GTK_PROGRESS(pbar), (gfloat)img->lines_done);
    
    if (img->lines_done == img->real_height) {
        stop_rendering(img);

        return FALSE;
    }
    else
        return TRUE;
}

void quit(void)
{
  gtk_exit (0);
}

int main (int argc, char** argv)
{
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* button;
    GtkWidget* tmp;
    GtkAdjustment* adj;
    
    program_name = argv[0];
    gtk_init(&argc, &argv);

    gdk_rgb_init();
    
    palette = g_malloc(256*4);
    if (palette_load(DEFAULT_PALETTE_FILE) == FALSE) {
        fprintf(stderr, "Can't load palette file %s\n",
                DEFAULT_PALETTE_FILE);
        exit(1);
    }

    init_misc();
    process_args(argc, argv);
    gtk_timeout_add(10*1000, child_reaper, NULL);
    set_image_info(&img, img.user_width, img.user_height, img.aa_factor);
    set_image_info(&j_pre, JPRE_SIZE, JPRE_SIZE/img.ratio, JPRE_AAFACTOR);
    
    /* main window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
    gtk_widget_realize(window);
    gtk_signal_connect(GTK_OBJECT(window), "key_press_event",
                       GTK_SIGNAL_FUNC(key_event), NULL);
    
    /* preview window */
    j_pre_window = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_signal_connect(GTK_OBJECT(j_pre_window), "delete_event",
                       GTK_SIGNAL_FUNC(j_pre_delete), NULL);
    gtk_window_set_title(GTK_WINDOW(j_pre_window), "Preview");
    gtk_window_set_policy(GTK_WINDOW(j_pre_window), FALSE, FALSE, TRUE);
    
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);

    create_menus(vbox);
    
    gtk_signal_connect (GTK_OBJECT (window), "destroy",
                        GTK_SIGNAL_FUNC (quit), NULL);

    /* toolbar stuff */
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* zoom in */
    zoom_in_button = gtk_toggle_button_new();
    gtk_container_add(GTK_CONTAINER(zoom_in_button),
                      create_pixmap(window, zoom_in_xpm));
    gtk_signal_connect(GTK_OBJECT(zoom_in_button), "toggled",
                       GTK_SIGNAL_FUNC(zoom_in_func), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), zoom_in_button, FALSE, FALSE,
                       0);
    gtk_button_set_relief(GTK_BUTTON(zoom_in_button),
                          GTK_RELIEF_NONE);
    GTK_WIDGET_UNSET_FLAGS(zoom_in_button, GTK_CAN_FOCUS);
    gtk_widget_show(zoom_in_button);

    /* zoom out */
    zoom_out_button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(zoom_out_button),
                      create_pixmap(window, zoom_out_xpm));
    gtk_signal_connect(GTK_OBJECT(zoom_out_button), "clicked",
                       GTK_SIGNAL_FUNC(zoom_out_func), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), zoom_out_button, FALSE, FALSE,
                       0);
    gtk_button_set_relief(GTK_BUTTON(zoom_out_button),
                          GTK_RELIEF_NONE);
    GTK_WIDGET_UNSET_FLAGS(zoom_out_button, GTK_CAN_FOCUS);
    gtk_widget_show(zoom_out_button);

    /* depth label */
    button = gtk_label_new("Depth");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    /* depth spin-button */
    adj = (GtkAdjustment*)gtk_adjustment_new((gfloat)img.depth, 1.0,
                                             2147483647.0, 1.0, 1.0, 0.0);
    depth_spin = gtk_spin_button_new(adj, 0, 0);
    gtk_widget_set_usize(depth_spin, 70, 0);
    gtk_box_pack_start(GTK_BOX(hbox), depth_spin, FALSE, FALSE, 0);
    gtk_widget_show(depth_spin);

    /* palette interpolation */
    palette_ip = gtk_check_button_new_with_label("Palette interpolation");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(palette_ip),
        img.palette_ip);
    gtk_box_pack_start(GTK_BOX(hbox), palette_ip, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(palette_ip), "toggled",
        GTK_SIGNAL_FUNC(toggle_palette_ip), NULL);
    gtk_widget_show(palette_ip);
    
    /* recalc button */
    button = gtk_button_new();
    recalc_button_label = gtk_label_new(TEXT_STOP);
    gtk_misc_set_alignment(GTK_MISC(recalc_button_label),
                           0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), recalc_button_label);
    gtk_widget_show(recalc_button_label);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(recalc_button), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    
    /* progress bar */
    pbar = gtk_progress_bar_new();
    gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
                                     GTK_PROGRESS_LEFT_TO_RIGHT);
    gtk_progress_configure(GTK_PROGRESS(pbar), 0.0, 0.0,
                           (gfloat)img.real_height);
    gtk_widget_set_usize(pbar, 75, 0);
    gtk_box_pack_end(GTK_BOX(hbox), pbar, FALSE, FALSE, 0);

    /* separator */
    button = gtk_hseparator_new();
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    /* main window drawing area */
    tmp = gtk_drawing_area_new();
    gtk_widget_set_events (tmp, GDK_BUTTON_PRESS_MASK
                           | GDK_BUTTON_RELEASE_MASK
                           | GDK_POINTER_MOTION_MASK
                           | GDK_EXPOSURE_MASK);
    gtk_drawing_area_size(GTK_DRAWING_AREA(tmp),
                          img.user_width >= MIN_WINDOW_WIDTH ?
                          img.user_width : MIN_WINDOW_WIDTH,
                          img.user_height);
    gtk_box_pack_start(GTK_BOX(vbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);
    
    gtk_signal_connect(GTK_OBJECT(tmp), "button_press_event",
                        (GtkSignalFunc)button_press_event, NULL);
    gtk_signal_connect(GTK_OBJECT(tmp), "button_release_event",
                        (GtkSignalFunc)button_release_event, NULL);
    gtk_signal_connect(GTK_OBJECT(tmp), "expose_event",
                       GTK_SIGNAL_FUNC(expose_event), &img);
    gtk_signal_connect(GTK_OBJECT(tmp), "motion_notify_event",
                       GTK_SIGNAL_FUNC(motion_event), NULL);
    img.drawing_area = drawing_area = tmp;

    /* preview window drawing area */
    tmp = gtk_drawing_area_new();
    gtk_widget_set_events (tmp, GDK_EXPOSURE_MASK);
    gtk_drawing_area_size(GTK_DRAWING_AREA(tmp),
                           j_pre.user_width, j_pre.user_height);
    gtk_container_add(GTK_CONTAINER(j_pre_window), tmp);
    gtk_signal_connect(GTK_OBJECT(tmp), "expose_event",
                       GTK_SIGNAL_FUNC(expose_event), &j_pre);
    gtk_widget_show(tmp);
    j_pre.drawing_area = tmp;
    
    start_rendering(&img);
    gtk_widget_show(window);

    gtk_main();

    return 0;
}
