#include <stdlib.h>
#include <string.h>

#include "gui.h"

static PSWindow *pswindow = NULL;

gboolean cb_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
	GdkWindow *drawable = widget->window;
	cairo_t *cr;
	double line_width = 30.0;

	cr = gdk_cairo_create(drawable);
	cairo_set_line_width(cr, line_width);

	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	cairo_move_to(cr, 50.0, 50.0);
	cairo_line_to(cr, 350.0, 50.0);
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to(cr, 50.0, 100.0);
	cairo_line_to(cr, 350.0, 100.0);
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	cairo_move_to(cr, 50.0, 150.0);
	cairo_line_to(cr, 350.0, 150.0);
	cairo_stroke(cr);
	
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

	cairo_move_to(cr, 50.0, 15.0);
	cairo_line_to(cr, 50.0, 185.0);
	//cairo_stroke(cr);

	cairo_line_to(cr, 50.0 - line_width / 2.0, 15.0);
	cairo_line_to(cr, 50.0 - line_width / 2.0, 185.0);
	cairo_stroke(cr);

	cairo_destroy(cr);

	return FALSE;
}

int initPSWindow()
{
	GtkWidget *window;
	GtkWidget *canvas;

	if(! pswindow){
		pswindow = malloc(sizeof(PSWindow));
		if(! pswindow){
			return RET_ERROR;
		}

		memset(pswindow, 0, sizeof(PSWindow));

		gtk_init(NULL, NULL);

		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(window), "NN chromosme");;
		gtk_widget_set_size_request(window, 400, 200);
		g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	
		canvas = gtk_drawing_area_new();
		gtk_container_add(GTK_CONTAINER(window), canvas);
		g_signal_connect(G_OBJECT(canvas), "expose-event", G_CALLBACK(cb_expose_event), NULL);

		gtk_widget_show_all(window);
		gtk_main();

		printf("finish initPSWindow()\n");
	}

	return RET_SUCCESS;
}

void destructPSWindow()
{
	if(pswindow){
		free(pswindow);
	}
}
