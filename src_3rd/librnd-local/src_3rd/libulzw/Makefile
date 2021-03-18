# where to install:
PREFIX=/usr/local
INCDIR=$(DESTDIR)$(inst_root)$(PREFIX)/include/libulzw
LIBDIR=$(DESTDIR)$(inst_root)$(PREFIX)/lib

CFLAGS_CFG=-O3
#CFLAGS_CFG=-Wall -g
CFLAGS = $(CFLAGS_CFG) -I.. $(CFLAGS_LIBULZW)

OBJSA=libulzw_comp.o libulzw_decomp.o
LIBA=libulzw.a
PWD=`pwd`

all: $(LIBA)

libulzw_comp.o: libulzw_comp.c
	$(CC) $(CFLAGS) -c libulzw_comp.c -o libulzw_comp.o

libulzw_decomp.o: libulzw_decomp.c
	$(CC) $(CFLAGS) -c libulzw_decomp.c -o libulzw_decomp.o

$(LIBA): $(OBJSA)
	touch $(LIBA)
	rm $(LIBA)
	ar ru $(LIBA) $(OBJSA)
	-ar s $(LIBA)
	-ranlib $(LIBA)

install_: $(LIBA)
	mkdir -p $(LIBDIR) $(INCDIR)
	$(CP) $(PWD)/libulzw.h            $(INCDIR)/libulzw.h
	$(CP) $(PWD)/$(LIBA)              $(LIBDIR)/$(LIBA)

uninstall:
	-rm $(INCDIR)/*_api.h $(INCDIR)/*_impl.h $(INCDIR)/libulzw.h $(LIBDIR)/$(LIBA)
	-rmdir $(INCDIR)

install:
	make install_ CP="cp"

linstall:
	make install_ CP="ln -s"

clean:
	-rm $(LIBA) $(OBJSA)

