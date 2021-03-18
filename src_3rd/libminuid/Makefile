CFLAGS = $(LIBMINUID_CFLAGS)
LDFLAGS = $(LIBMINUID_LDFLAGS)

#CFLAGS += -g -Wall -ansi -pedantic
#LDFLAGS = -rdynamic

INCDIR=$(install_root)$(DESTDIR)/usr/include
LIBDIR=$(install_root)$(DESTDIR)/usr/lib

VER_MAJOR=1
VER_MINOR=0
VER_PATCH=1
VER=$(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)

LIBSO=libminuid.so.$(VER)
LIBSO1=libminuid.so.$(VER_MAJOR)
LIBA=libminuid.a


all: $(LIBA) $(LIBSO)

libminuid.o: libminuid.c libminuid.h
	$(CC) $(CFLAGS) -c libminuid.c -o $@

$(LIBSO): libminuid.o
	$(CC) $(LDFLAGS) -shared -dynamic -o $@ libminuid.o

$(LIBA): libminuid.o
	@touch $(LIBA)
	@rm $(LIBA)
	@ar ru $(LIBA) libminuid.o
	-ar s $(LIBA)
	-ranlib $(LIBA)

clean:
	-rm libminuid.o $(LIBA) $(LIBSO) $(LIBSO1)

install_:
	mkdir -p $(INCDIR) $(LIBDIR)
	$(CP) `pwd`/libminuid.h $(INCDIR)/libminuid.h
	$(CP) `pwd`/$(LIBSO) $(LIBDIR)/$(LIBSO)
	$(CP) `pwd`/$(LIBA) $(LIBDIR)/$(LIBA)

uninstall:
	rm $(INCDIR)/libminuid.h $(LIBDIR)/$(LIBSO) $(LIBDIR)/$(LIBA) $(LIBDIR)/$(LIBSO1)

install:
	make install_ CP="cp"
	ln -s $(LIBSO) $(LIBDIR)/$(LIBSO1)

linstall:
	make install_ CP="ln -s"
	ln -s $(LIBSO) $(LIBDIR)/$(LIBSO1)
