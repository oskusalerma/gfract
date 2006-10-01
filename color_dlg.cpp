#include "color_dlg.h"
#include "color.h"
#include "image_info.h"
#include "main.h"

static void on_destroy(GtkWidget* widget, color_dialog* dl);
static void on_help(GtkWidget* widget, color_dialog* dl);
static void on_refresh(GtkWidget* w, color_dialog* dl);
static void do_refresh(color_dialog* dl, bool isApply);
static bool update_cops(color_ops* ops, GtkWidget* w, color_dialog* dl);

void on_destroy(GtkWidget* widget, color_dialog* dl)
{
    delete dl;
}

void on_help(GtkWidget* widget, color_dialog* dl)
{
    GtkWidget* dlg = gtk_message_dialog_new(NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK, "Available operations:\n\n%s",
        get_cop_help().c_str());
    
    g_signal_connect_swapped(dlg, "response", G_CALLBACK(gtk_widget_destroy),
        dlg);
    gtk_widget_show(dlg);
}

void on_refresh(GtkWidget* w, color_dialog* dl)
{
    do_refresh(dl, w == dl->apply_button);
}

void do_refresh(color_dialog* dl, bool isApply)
{
    if (!update_cops(&dl->img->color_out, dl->out_label, dl) ||
        !update_cops(&dl->img->color_in, dl->in_label, dl))
    {
        return;
    }
    
    main_refresh();
    if (!isApply)
    {
        gtk_widget_destroy(dl->dialog);
    }
}

bool update_cops(color_ops* ops, GtkWidget* w, color_dialog* dl)
{
    color_ops tmp = *ops;
    std::string str = gtk_entry_get_text(GTK_ENTRY(w));
    std::string res = str2ops(str, &tmp);

    if (res.length() == 0)
    {
        *ops = tmp;
        
        return true;
    }

    GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(dl->dialog),
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK, "%s", res.c_str());
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    return false;
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
    dl->img = img;
    *ptr = dl;
    
    /* DIALOG AREA */
    dl->dialog = gtk_dialog_new();
    g_signal_connect(GTK_OBJECT(dl->dialog), "destroy",
        GTK_SIGNAL_FUNC(on_destroy), dl);
    g_signal_connect(GTK_OBJECT(dl->dialog), "destroy",
        GTK_SIGNAL_FUNC(gtk_widget_destroyed), ptr);
    gtk_window_set_title(GTK_WINDOW(dl->dialog), "Coloring Settings");
    gtk_window_set_resizable(GTK_WINDOW(dl->dialog), FALSE);
    gtk_window_set_position(GTK_WINDOW(dl->dialog), GTK_WIN_POS_MOUSE);

    /* BUTTONS */
    dl->ok_button = gtk_button_new_with_label("OK");
    g_signal_connect(GTK_OBJECT(dl->ok_button), "clicked",
        GTK_SIGNAL_FUNC(on_refresh), dl);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->action_area),
        dl->ok_button, TRUE, TRUE, 0);
    
    dl->apply_button = gtk_button_new_with_label("Apply");
    g_signal_connect(GTK_OBJECT(dl->apply_button), "clicked",
        GTK_SIGNAL_FUNC(on_refresh), dl);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->action_area),
        dl->apply_button, TRUE, TRUE, 0);
    
    tmp = gtk_button_new_with_label("Cancel");
    g_signal_connect_object(GTK_OBJECT(tmp), "clicked",
        GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dl->dialog),
                G_CONNECT_SWAPPED);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->action_area), tmp,
        TRUE, TRUE, 0);

    tmp = gtk_button_new_with_label("Help");
    g_signal_connect_object(GTK_OBJECT(tmp), "clicked",
        GTK_SIGNAL_FUNC(on_help), GTK_OBJECT(dl->dialog), 
                G_CONNECT_SWAPPED);
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
