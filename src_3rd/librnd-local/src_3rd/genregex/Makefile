#CFLAGS = -Wall -ansi -pedantic -g
CFLAGS = -g $(GENREGEX_CFLAGS)
LDFLAGS = $(GENREGEX_LDFLAGS)

PREFIX=/usr
INCDIR=$(install_root)$(DESTDIR)$(PREFIX)/include/genregex
LIBDIR=$(install_root)$(DESTDIR)$(PREFIX)/lib


OBJS = tester.o regex.o  regex_se.o regex_sei.o regex_be.o regex_bei.o \
       regex_st.o regex_sti.o regex_bt.o regex_bti.o

LIBA = libgenregex.a

all: tester $(LIBA)

tester: $(OBJS)

tester.o: tester.c regex_se.h regex_be.h regex_st.h regex_bt.h regex_templ.h regex.h tester_main.c

regex.o: regex.c regex.h

regex_se.o: regex_se.c regex_se.h   regex_templ.c regex_templ.h regex.h

regex_sei.o: regex_sei.c regex_sei.h   regex_templ.c regex_templ.h regex.h

regex_be.o: regex_be.c regex_be.h   regex_templ.c regex_templ.h regex.h

regex_bei.o: regex_bei.c regex_bei.h   regex_templ.c regex_templ.h regex.h

regex_st.o: regex_st.c regex_st.h   regex_templ.c regex_templ.h regex.h

regex_sti.o: regex_sti.c regex_sti.h   regex_templ.c regex_templ.h regex.h

regex_bt.o: regex_bt.c regex_bt.h   regex_templ.c regex_templ.h regex.h

regex_bti.o: regex_bti.c regex_bti.h   regex_templ.c regex_templ.h regex.h

$(LIBA): $(OBJS)
	-rm $(LIBA)
	ar r $(LIBA) $(OBJS)
	-ar s $(LIBA)
	-ranlib $(LIBA)

test:
	cd ../regression; make all

clean:
	rm $(OBJS) $(LIBA) tester 2>/dev/null ; true

install_:
	mkdir -p $(INCDIR) $(LIBDIR)
	$(CP) `pwd`/regex.h          $(INCDIR)/regex.h
	$(CP) `pwd`/regex_be.h       $(INCDIR)/regex_be.h
	$(CP) `pwd`/regex_bei.h      $(INCDIR)/regex_bei.h
	$(CP) `pwd`/regex_bt.h       $(INCDIR)/regex_bt.h
	$(CP) `pwd`/regex_bti.h      $(INCDIR)/regex_bti.h
	$(CP) `pwd`/regex_se.h       $(INCDIR)/regex_se.h
	$(CP) `pwd`/regex_sei.h      $(INCDIR)/regex_sei.h
	$(CP) `pwd`/regex_st.h       $(INCDIR)/regex_st.h
	$(CP) `pwd`/regex_sti.h      $(INCDIR)/regex_sti.h
	$(CP) `pwd`/regex_templ.h    $(INCDIR)/regex_templ.h
	$(CP) `pwd`/libgenregex.a    $(LIBDIR)/libgenregex.a

uninstall:
	rm $(INCDIR)/regex.h
	rm $(INCDIR)/regex_be.h
	rm $(INCDIR)/regex_bei.h
	rm $(INCDIR)/regex_bt.h
	rm $(INCDIR)/regex_bti.h
	rm $(INCDIR)/regex_se.h
	rm $(INCDIR)/regex_sei.h
	rm $(INCDIR)/regex_st.h
	rm $(INCDIR)/regex_sti.h
	rm $(INCDIR)/regex_templ.h
	rm $(LIBDIR)/libgenregex.a

install:
	make install_ CP="cp"

linstall:
	make install_ CP="ln -s"


