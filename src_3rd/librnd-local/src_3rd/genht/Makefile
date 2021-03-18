# use -DGENHT_WANT_INLINE to turn half of the API into static inline functions
PREFIX=/usr
INCDIR=$(install_root)$(DESTDIR)$(PREFIX)/include/genht
LIBDIR=$(install_root)$(DESTDIR)$(PREFIX)/lib

HOSTCC = $(CC)

VER_MAJOR=1
VER_MINOR=1
VER_PATCH=1
VER=$(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)

CFLAGS=-g $(GENHT_CFLAGS)
LDFLAGS=$(GENHT_LDFLAGS)

BIN=mainsi
OBJS=htss.o htsp.o htsi.o htip.o htpp.o htpi.o hash.o siphash24.o
LIBSO=libgenht.so.$(VER)
LIBSO1=libgenht.so.$(VER_MAJOR)
LIBA=libgenht.a

all: version.h $(BIN) $(OBJS) $(LIBA) $(LIBSO)

clean:
	-rm $(BIN) $(OBJS) $(LIBA) $(LIBSO) genht_std.so genht_std.a version.h ver_edit

.SUFFIXES:
.SUFFIXES: .o .c

.c.o:
	$(CC) -o $@ -c -fPIC $< $(CFLAGS)

mainsi: mainsi.o htsi.o
	$(CC) -o $@ mainsi.o htsi.o $(LDFLAGS)

$(LIBSO): $(OBJS)
	$(CC) $(LDFLAGS) -shared -dynamic -rdynamic -o $@ $(OBJS)

$(LIBA): $(OBJS)
	ar rvu $@ $(OBJS)
	-ar s $@
	-ranlib $@

# for compatibility
genht_std.so: $(LIBSO)
	ln -s $(LIBSO) genht_std.so

# for compatibility
genht_std.a: $(LIBA)
	ln -s $(LIBA) genht_std.a

version.h: version.h.in ver_edit
	./ver_edit $(VER_MAJOR) $(VER_MINOR) $(VER_PATCH) < version.h.in > version.h

ver_edit: ver_edit.c
	$(HOSTCC) -o $@ $(CFLAGS) ver_edit.c

install_:
	mkdir -p $(INCDIR) $(LIBDIR)
	$(CP) `pwd`/hash.h $(INCDIR)/hash.h
	$(CP) `pwd`/ht.h $(INCDIR)/ht.h
	$(CP) `pwd`/version.h $(INCDIR)/version.h
	$(CP) `pwd`/ht_inlines.h $(INCDIR)/ht_inlines.h
	$(CP) `pwd`/ht_utils.h $(INCDIR)/ht_utils.h
	$(CP) `pwd`/htip.h $(INCDIR)/htip.h
	$(CP) `pwd`/htsi.h $(INCDIR)/htsi.h
	$(CP) `pwd`/htsp.h $(INCDIR)/htsp.h
	$(CP) `pwd`/htss.h $(INCDIR)/htss.h
	$(CP) `pwd`/htpp.h $(INCDIR)/htpp.h
	$(CP) `pwd`/$(LIBSO) $(LIBDIR)/$(LIBSO)
	$(CP) `pwd`/$(LIBA) $(LIBDIR)/$(LIBA)

uninstall:
	rm -rf $(INCDIR)
	rm $(LIBDIR)/$(LIBSO) $(LIBDIR)/$(LIBA) $(LIBDIR)/$(LIBSO1)

install:
	make install_ CP="cp"
	-@rm $(LIBDIR)/$(LIBSO1) 2>/dev/null
	ln -s $(LIBSO) $(LIBDIR)/$(LIBSO1)

linstall:
	make install_ CP="ln -s"
	-@rm $(LIBDIR)/$(LIBSO1) 2>/dev/null
	ln -s $(LIBSO) $(LIBDIR)/$(LIBSO1)

test: mainsi
	@./mainsi > mainsi.out
	@diff mainsi.ref mainsi.out
	@echo "*** QC pass ***"
