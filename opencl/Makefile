CFLAGS:=-Wall -Wextra -Ofast
LDFLAGS:=-lOpenCL

.PHONY: all clean run

all: mc

clean:
	rm -f mc_hypot_opencl.cli mc_hypot_opencl.d prog.h mc

run: mc
	./mc

mc_hypot_opencl.cli: mc_hypot_opencl.cl
	$(CC) -x c -E -o $@ $<
	$(CC) -x c -o $(@:.cli=.d) -MM -MT $@ $<

-include mc_hypot_opencl.d

prog.h: mc_hypot_opencl.cli
	echo "static const char * prog = " > $@
	sed 's/\"/\\"/g;s/^/"/;s/$$/\\n"/' $< >> $@
	echo ";" >> $@

mc: mc_hypot_opencl_main.c tool.c tool.h prog.h
	$(CC) -o $@ $(CFLAGS) mc_hypot_opencl_main.c tool.c $(LDFLAGS)
