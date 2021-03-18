PREFIX=/usr
LIBDIR=$(install_root)$(DESTDIR)$(PREFIX)/lib
INCDIR=$(install_root)$(DESTDIR)$(PREFIX)/include

#CFLAGS += -Wall -g -ansi -pedantic

OBJS = uundo.o uundo_debug.o

all: libuundo.a

libuundo.a: $(OBJS)
	@touch $@
	@rm $@
	@ar ru $@ $(OBJS)
	-ar s $@
	-ranlib $@

uundo.o: uundo.c uundo.h
	$(CC) -c $(CFLAGS) uundo.c -o uundo.o

uundo_debug.o: uundo_debug.c uundo_debug.h
	$(CC) -c $(CFLAGS) uundo_debug.c -o uundo_debug.o

install_:
	mkdir -p $(LIBDIR) $(INCDIR)
	$(CP) `pwd`/libuundo.a $(LIBDIR)/libuundo.a
	$(CP) `pwd`/uundo.h $(INCDIR)/uundo.h
	$(CP) `pwd`/uundo_debug.h $(INCDIR)/uundo_debug.h

uninstall:
	rm $(LIBDIR)/libuundo.a $(INCDIR)/uundo.h $(INCDIR)/uundo_debug.h

install:
	make install_ CP="cp"

linstall:
	make install_ CP="ln -s"


clean:
	rm $(OBJS) libuundo.a 2>/dev/null; true

