#include <stdio.h>
#include <stdlib.h>
#include "attr_dlg.h"

static void width_update(GtkWidget* w, image_attr_dialog* dl);
static void height_update(GtkWidget* w, image_attr_dialog* dl);
static void aa_update(GtkWidget* w, image_attr_dialog* dl);
static void constrain_update(GtkWidget*w, image_attr_dialog* dl);
static void update_text(GtkLabel* label, int w, int h, int aa);

void width_update(GtkWidget* w, image_attr_dialog* dl)
{
    int height;
    int width = atoi(gtk_entry_get_text(GTK_ENTRY(dl->width)));

    if (width <= 0)
        return;

    if (GTK_TOGGLE_BUTTON(dl->const_ra)->active) {
        height = int(width/dl->ratio);
        g_signal_handlers_block_by_func(GTK_OBJECT(dl->height),
                                    (gpointer)G_CALLBACK(height_update),
                                    dl);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(dl->height),
                                  height);
        g_signal_handlers_unblock_by_func(GTK_OBJECT(dl->height),
                                    (gpointer)G_CALLBACK(height_update),
                                    dl);
    }
    
    update_text(GTK_LABEL(dl->text), width,
                gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(dl->height)),
                gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(dl->aa)));
}

void height_update(GtkWidget* w, image_attr_dialog* dl)
{
    int width;
    int height = atoi(gtk_entry_get_text(GTK_ENTRY(dl->height)));

    if (height <= 0)
        return;

    if (GTK_TOGGLE_BUTTON(dl->const_ra)->active) {
        width = int(height*dl->ratio);
        g_signal_handlers_block_by_func(GTK_OBJECT(dl->width),
                                    (gpointer)G_CALLBACK(width_update),
                                    dl);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(dl->width),
                                  width);
        g_signal_handlers_unblock_by_func(GTK_OBJECT(dl->width),
                                    (gpointer)G_CALLBACK(width_update),
                                    dl);
    }
    
    update_text(GTK_LABEL(dl->text),
                gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(dl->width)), height,
                gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(dl->aa)));
}

void aa_update(GtkWidget* w, image_attr_dialog* dl)
{
    int aa = atoi(gtk_entry_get_text(GTK_ENTRY(dl->aa)));

    if (aa <= 0)
        return;
    
    update_text(GTK_LABEL(dl->text),
                gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(dl->width)),
                gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(dl->height)),
                aa);
}

void constrain_update(GtkWidget* w, image_attr_dialog* dl)
{
    if (GTK_TOGGLE_BUTTON(dl->const_ra)->active) {
        dl->ratio =
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(dl->width)) /
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(dl->height));
    }
}

void update_text(GtkLabel* label, int w, int h, int aa)
{
    char buf[256];
    double megs;

    megs = (w*h*sizeof(double)*aa*aa + w*h*4.0)/(1024.0*1024.0);
    snprintf(buf, 256, "Memory required: %.2f M", megs);
    gtk_label_set_text(label, buf);
}

static void image_attr_destroy(GtkWidget* widget,
                               image_attr_dialog* dl)
{
    delete dl;
}

void attr_dlg_new(image_attr_dialog** ptr, image_info* img)
{
    GtkWidget* tmp;
    GtkWidget* table;
    GtkWidget* vbox;
    GtkObject* adj;
    image_attr_dialog* dl;

    dl = new image_attr_dialog;
    *ptr = dl;

    dl->ratio = (double)img->user_width/img->user_height;
    dl->dialog = gtk_dialog_new();
    g_signal_connect(GTK_OBJECT(dl->dialog), "destroy",
                       GTK_SIGNAL_FUNC(image_attr_destroy),
                       dl);
    g_signal_connect(GTK_OBJECT(dl->dialog), "destroy",
                       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                       ptr);
    gtk_window_set_title(GTK_WINDOW(dl->dialog), "Attributes");
    gtk_window_set_resizable(GTK_WINDOW(dl->dialog), FALSE);
    gtk_window_set_position(GTK_WINDOW(dl->dialog), GTK_WIN_POS_MOUSE);
    
    dl->ok_button = gtk_button_new_with_label("OK");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->action_area),
                       dl->ok_button, TRUE, TRUE, 0);
    gtk_widget_show(dl->ok_button);

    dl->apply_button = gtk_button_new_with_label("Apply");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->action_area),
                       dl->apply_button, TRUE, TRUE, 0);
    gtk_widget_show(dl->apply_button);

    tmp = gtk_button_new_with_label("Cancel");
    g_signal_connect_object(GTK_OBJECT(tmp), "clicked",
                              GTK_SIGNAL_FUNC(gtk_widget_destroy),
                              GTK_OBJECT(dl->dialog), G_CONNECT_SWAPPED);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->action_area), tmp,
                       TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    table = gtk_table_new(8, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 2);
    gtk_table_set_row_spacing(GTK_TABLE(table), 2, 10);
    gtk_table_set_row_spacing(GTK_TABLE(table), 3, 15);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl->dialog)->vbox), vbox,
                       TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_widget_show(vbox);
    
    tmp = gtk_label_new("Width:");
    gtk_misc_set_alignment(GTK_MISC(tmp), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 0, 1, 0, 1);
    gtk_widget_show(tmp);

    adj = gtk_adjustment_new(img->user_width, 1.0, 999999.0,
                             4.0, 4.0, 0.0);
    tmp = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.0, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 1, 2, 0, 1);
    g_signal_connect(GTK_OBJECT(tmp), "changed",
                       GTK_SIGNAL_FUNC(width_update),
                       dl);
    gtk_widget_show(tmp);
    dl->width = tmp;
    
    tmp = gtk_label_new("Height:");
    gtk_misc_set_alignment(GTK_MISC(tmp), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 0, 1, 1, 2);
    gtk_widget_show(tmp);

    adj = gtk_adjustment_new(img->user_height, 1.0, 999999.0,
                             3.0, 3.0, 0.0);
    tmp = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.0, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 1, 2, 1, 2);
    g_signal_connect(GTK_OBJECT(tmp), "changed",
                       GTK_SIGNAL_FUNC(height_update),
                       dl);
    gtk_widget_show(tmp);
    dl->height = tmp;
    
    tmp = gtk_label_new("Anti-aliasing factor:\n(1 = no anti-aliasing)");
    gtk_misc_set_alignment(GTK_MISC(tmp), 0.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(tmp), GTK_JUSTIFY_LEFT);
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 0, 1, 2, 3);
    gtk_widget_show(tmp);

    adj = gtk_adjustment_new(img->aa_factor, 1.0, 500.0, 1.0, 1.0, 0.0);
    tmp = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.0, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 1, 2, 2, 3);
    g_signal_connect(GTK_OBJECT(tmp), "changed",
                       GTK_SIGNAL_FUNC(aa_update),
                       dl);
    gtk_widget_show(tmp);
    dl->aa = tmp;
    
    tmp = gtk_check_button_new_with_label("Constrain ratio");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), TRUE);
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 0, 2, 3, 4);
    g_signal_connect(GTK_OBJECT(tmp), "toggled",
                       GTK_SIGNAL_FUNC(constrain_update),
                       dl);
    gtk_widget_show(tmp);
    dl->const_ra = tmp;
    
    tmp = gtk_label_new("");
    update_text(GTK_LABEL(tmp), img->user_width, img->user_height,
                img->aa_factor);
    gtk_misc_set_alignment(GTK_MISC(tmp), 0.5, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 0, 2, 4, 5);
    gtk_widget_show(tmp);
    dl->text = tmp;
    
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
    gtk_widget_show(table);
    
    gtk_widget_show(dl->dialog);
}
