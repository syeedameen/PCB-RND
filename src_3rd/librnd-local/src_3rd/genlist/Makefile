PREFIX=/usr
INCDIR=$(install_root)$(DESTDIR)$(PREFIX)/include/genlist
LIBDIR=$(install_root)$(DESTDIR)$(PREFIX)/lib

CFLAGS = -I. -I.. $(GENLIST_CFLAGS)
LDFLAGS = $(GENLIST_LDFLAGS)

#CFLAGS += -Wall -g -I. -I..

OBJS = genlistalloc.o genadlist.o
LIBA = libgenlist.a

all: $(LIBA) genlist.a

$(LIBA): $(OBJS)
	ar r $(LIBA) $(OBJS)
	-ar s $(LIBA)
	-ranlib $(LIBA)

# obsolete: DO NOT DEPEND ON THIS
genlist.a: $(LIBA)
	cp $(LIBA) genlist.a

example_ad: example_ad.o $(OBJS)

example_d: example_d.c gendlist.h

clean:
	rm $(OBJS) $(LIBA) genlist.a example_ad example_d; true

install_:
	mkdir -p $(INCDIR) $(LIBDIR)
	$(CP) `pwd`/genadlist.h        $(INCDIR)/genadlist.h
	$(CP) `pwd`/gendlist.h         $(INCDIR)/gendlist.h
	$(CP) `pwd`/genlistalloc.h     $(INCDIR)/genlistalloc.h
	$(CP) `pwd`/gentdlist_impl.h   $(INCDIR)/gentdlist_impl.h
	$(CP) `pwd`/gentdlist_undef.h  $(INCDIR)/gentdlist_undef.h
	$(CP) `pwd`/$(LIBA)            $(LIBDIR)/$(LIBA)

uninstall:
	rm $(INCDIR)/$(INCDIR)/genadlist.h
	rm $(INCDIR)/$(INCDIR)/gendlist.h
	rm $(INCDIR)/$(INCDIR)/genlistalloc.h
	rm $(INCDIR)/$(INCDIR)/gentdlist_impl.h
	rm $(INCDIR)/$(INCDIR)/gentdlist_undef.h
	rm $(LIBDIR)/$(LIBA)

install:
	make install_ CP="cp"

linstall:
	make install_ CP="ln -s"

