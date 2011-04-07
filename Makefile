fname="tagvox"
tools_dir="tools"

GTK_FLAGS=-D_REENTRANT -I/usr/include/gtk-2.0 -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/freetype2 -I/usr/include/directfb -I/usr/include/libpng12 -I/usr/include/pixman-1  -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0

all: src/nn_chromosome.o src/get_ps.o src/get_sound.o src/judge.o src/gui.o
	gcc -o ${fname} $^ ${GTK_FLAGS} -lm -luuid -lgsl -lgslcblas

debug:
	make all "CFLAGS=-D __DEBUG__"

input_stream: src/input_stream.c
	gcc -o ${tools_dir}/$@ $<

output_stream: src/output_stream.c
	gcc -o ${tools_dir}/$@ $<

src/gui.o: src/gui.c
	gcc -o $@ -c $< ${GTK_FLAGS}

get_ps.o:
	gcc -c get_ps.c -lm -lgsl -lgslcblas

clean:
	rm -f src/*.o tagvox ${tools_dir}/input_stream ${tools_dir}/output_stream
