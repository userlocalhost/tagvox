#ifndef __GUI_H__
#define __GUI_H__

#include <gtk/gtk.h>
#include "common.h"

typedef struct _PSWindow{
	GtkWidget *window;
	GtkWidget *canvas;
	double *data;
}PSWindow;

int initPSWnidow(void);
void destructPSWnidow(void);

#endif
