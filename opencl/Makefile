CFLAGS:=-Wall -Wextra -Ofast
LDFLAGS:=-lcl

.PHONY: all clean run

all: mc

clean:
	rm -f mc_hypot_opencl.cli prog.h mc

run: mc
	./mc

mc_hypot_opencl.cli: mc_hypot_opencl.cl
	$(CC) -E -o $@ $<

prog.h: mc_hypot_opencl.cli
	echo "static const char * prog = " > $@
	sed 's/\"/\\"/g;s/^/"/;s/$$/\\n"/' $< >> $@
	echo ";" >> $@

mc: mc_hypot_opencl_main.c tool.c tool.h prog.h
	$(CC) -o $@ $(CFLAGS) mc_hypot_opencl_main.c tool.c $(LDFLAGS)
