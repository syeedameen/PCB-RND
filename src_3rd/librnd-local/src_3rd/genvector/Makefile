#CFLAGS = -I.. -O3 -Wall -ansi -pedantic
CFLAGS = -I.. -g $(GENVECTOR_CFLAGS)
LDFLAGS = $(GENVECTOR_LDFLAGS)
OBJS = gds_char.o vti0.o vts0.o vtp0.o vtii.o vti0.o vtd0.o
IMPL = genvector_impl.c genvector_impl.h genvector_undef.h

LIBA = libgenvector.a

all: $(LIBA)

$(LIBA): $(OBJS)
	ar ur $(LIBA) $(OBJS)
	-ar s $(LIBA)
	-ranlib $(LIBA)

gds_char.o: gds_char.c gds_char.h $(IMPL)
	$(CC) -c $(CFLAGS) $*.c -o $@

gds_wchar.o: gds_wchar.c gds_wchar.h $(IMPL)
	$(CC) -c $(CFLAGS) $*.c -o $@

vti0.o: vti0.c vti0.h $(IMPL)
	$(CC) -c $(CFLAGS) $*.c -o $@

vtd0.o: vtd0.c vtd0.h $(IMPL)
	$(CC) -c $(CFLAGS) $*.c -o $@

vts0.o: vts0.c vts0.h $(IMPL)
	$(CC) -c $(CFLAGS) $*.c -o $@

vtp0.o: vtp0.c vtp0.h $(IMPL)
	$(CC) -c $(CFLAGS) $*.c -o $@

vtii.o: vtii.c vtii.h $(IMPL)
	$(CC) -c $(CFLAGS) $*.c -o $@

clean:
	rm $(OBJS) $(LIBA) 2>/dev/null ; true

include ../Makefile.conf

install_:
	mkdir -p "$(INCDIR)" "$(LIBDIR)"
	$(CP) "$(PWD)/gds_char.h" "$(INCDIR)/gds_char.h"
	$(CP) "$(PWD)/gds_wchar.h" "$(INCDIR)/gds_wchar.h"
	$(CP) "$(PWD)/genvector_impl.h" "$(INCDIR)/genvector_impl.h"
	$(CP) "$(PWD)/genvector_undef.h" "$(INCDIR)/genvector_undef.h"
	$(CP) "$(PWD)/vti0.h" "$(INCDIR)/vti0.h"
	$(CP) "$(PWD)/vtii.h" "$(INCDIR)/vtii.h"
	$(CP) "$(PWD)/$(LIBA)" "$(LIBDIR)/$(LIBA)"

uninstall:
	rm $(LIBDIR)/$(LIBA)
	rm $(INCDIR)/gds_*.h $(INCDIR)/genvector*.h $(INCDIR)/vt*.h

install:
	make install_ CP="cp" PWD=`pwd`

linstall:
	make install_ CP="ln -s" PWD=`pwd`

