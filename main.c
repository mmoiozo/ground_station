#include <gtk/gtk.h>

typedef struct _Widgets Widgets;

struct _Widgets
{
   GtkEntry *e1;
   GtkEntry *e2;
   GtkLabel *l1;
   GtkLabel *l2;
};


int main(int argc, char *argv[])
{
    GtkBuilder      *builder; 
    GtkWidget       *window;
    Widgets     widg;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "window_main.glade", NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
    widg.e1 = GTK_ENTRY(gtk_builder_get_object(builder, "entry1" )); 
    widg.l1 = GTK_LABEL(gtk_builder_get_object( builder, "label1" ));

    gtk_builder_connect_signals(builder, &widg);

    g_object_unref(builder);

    gtk_widget_show(window);
    gtk_main();

    return 0;
}

// called when window is closed
void on_window_main_destroy()
{
    gtk_main_quit();
}

//button
void button1_clicked_cb ( GtkButton *button, Widgets *widg)
{
	gtk_entry_set_text (widg->e1,"test");
	gtk_label_set_label (widg->l1,"test 2");
}
