#include "color_dlg.h"
#include "color.h"
#include "fractal_types.h"
#include "main.h"

static void on_destroy(GtkWidget* widget, color_dialog* dl);
static void on_refresh(GtkWidget* w, color_dialog* dl);
static void do_refresh(color_dialog* dl, bool isApply);


void on_destroy(GtkWidget* widget, color_dialog* dl)
{
    delete dl;
}

void on_refresh(GtkWidget* w, color_dialog* dl)
{
    do_refresh(dl, w == dl->apply_button);
}

void do_refresh(color_dialog* dl, bool isApply)
{
    main_refresh();
    if (!isApply)
    {
        gtk_widget_destroy(dl->dialog);
    }
}

void color_dlg_new(color_dialog** ptr, image_info* img)
{
    color_dialog* dl;

    GtkWidget* vbox;
    GtkWidget* tbox;
    GtkWidget* in_frame;
    GtkWidget* out_frame;
    GtkWidget* tmp;

    dl = new color_dialog;
    *ptr = dl;

    /* DIALOG AREA */
    dl->dialog = gtk_dialog_new();
    gtk_signal_connect(GTK_OBJECT(dl->dialog), "destroy",
        GTK_SIGNAL_FUNC(on_destroy), dl);
    gtk_signal_connect(GTK_OBJECT(dl->dialog), "destroy",
        GTK_SIGNAL_FUNC(gtk_widget_destroyed), ptr);
    gtk_window_set_title(GTK_WINDOW(dl->dialog), "Coloring Settings");
    gtk_window_set_policy(GTK_WINDOW(dl->dialog), FALSE, FALSE, FALSE);
    gtk_window_set_position(GTK_WINDOW(dl->dialog), GTK_WIN_POS_MOUSE);

    /* BUTTONS */
    dl->ok_button = gtk_button_new_with_label("OK");
    gtk_signal_connect(GTK_OBJECT(dl->ok_button), "clicked",
        GTK_SIGNAL_FUNC(on_refresh), dl);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->action_area),
        dl->ok_button, TRUE, TRUE, 0);
    
    dl->apply_button = gtk_button_new_with_label("Apply");
    gtk_signal_connect(GTK_OBJECT(dl->apply_button), "clicked",
        GTK_SIGNAL_FUNC(on_refresh), dl);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->action_area),
        dl->apply_button, TRUE, TRUE, 0);
    
    tmp = gtk_button_new_with_label("Cancel");
    gtk_signal_connect_object(GTK_OBJECT(tmp), "clicked",
        GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dl->dialog));
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->action_area), tmp,
        TRUE, TRUE, 0);

    /* main contents */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->vbox), vbox,
        TRUE, TRUE, 0);

    out_frame = gtk_frame_new("Outside");
    in_frame = gtk_frame_new("Inside");
    gtk_container_set_border_width(GTK_CONTAINER(out_frame), 5);
    gtk_container_set_border_width(GTK_CONTAINER(in_frame), 5);

    gtk_box_pack_start(GTK_BOX(vbox), out_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), in_frame, TRUE, TRUE, 0);

    tbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(out_frame), tbox);
    gtk_container_set_border_width(GTK_CONTAINER(tbox), 10);
    
    dl->out_label = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(dl->out_label),
        ops2str(&img->color_out).c_str());
    gtk_box_pack_start(GTK_BOX(tbox), dl->out_label, TRUE, TRUE, 0);
        
    tbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(in_frame), tbox);
    gtk_container_set_border_width(GTK_CONTAINER(tbox), 10);
    
    dl->in_label = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(dl->in_label),
        ops2str(&img->color_in).c_str());
    gtk_box_pack_start(GTK_BOX(tbox), dl->in_label, TRUE, TRUE, 0);

    gtk_widget_show_all(dl->dialog);
}
