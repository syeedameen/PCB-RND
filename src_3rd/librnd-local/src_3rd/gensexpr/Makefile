CFLAGS = -I.. $(GENSEXPR_CFLAGS)
LDFLAGS = $(GENSEXPR_LDFLAGS)
#CFLAGS += -Wall -g -ansi -pedantic

PREFIX=/usr
INCDIR=$(install_root)$(DESTDIR)$(PREFIX)/include/gensexpr
LIBDIR=$(install_root)$(DESTDIR)$(PREFIX)/lib


OBJS = gsxl.o gsxnl.o gsx_parse.o
COMMON_DEPS = gensexpr_impl.c gensexpr_impl.h gsx_parse.h
LIBA=libgensexpr.a

all: $(LIBA)

$(LIBA): $(OBJS)
	ar rv $(LIBA) $(OBJS)
	-ar s $(LIBA)
	-ranlib $(LIBA)

gsxl.o: gsxl.c gsxl.h $(COMMON_DEPS)
	$(CC) -c $(CFLAGS) gsxl.c -o gsxl.o

gsxnl.o: gsxnl.c gsxnl.h $(COMMON_DEPS)
	$(CC) -c $(CFLAGS) gsxnl.c -o gsxnl.o

gsx_parse.o: gsx_parse.c gsx_parse.h
	$(CC) -c $(CFLAGS) gsx_parse.c -o gsx_parse.o

clean:
	-rm $(OBJS) $(LIBA)

install_:
	mkdir -p $(INCDIR) $(LIBDIR)
	$(CP) `pwd`/gensexpr_impl.h  $(INCDIR)/gensexpr_impl.h
	$(CP) `pwd`/gsx_parse.h      $(INCDIR)/gsx_parse.h
	$(CP) `pwd`/gsxl.h           $(INCDIR)/gsxl.h
	$(CP) `pwd`/gsxnl.h          $(INCDIR)/gsxnl.h
	$(CP) `pwd`/libgensexpr.a    $(LIBDIR)/libgensexpr.a

uninstall:
	rm $(INCDIR)/gensexpr_impl.h
	rm $(INCDIR)/gsx_parse.h
	rm $(INCDIR)/gsxl.h
	rm $(INCDIR)/gsxnl.h
	rm $(LIBDIR)/libgensexpr.a

install:
	make install_ CP="cp"

linstall:
	make install_ CP="ln -s"


