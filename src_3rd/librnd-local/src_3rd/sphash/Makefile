CFLAGS = -g $(SPHASH_CFLAGS)
LDFLAGS = $(SPHASH_LDFLAGS)
#CFLAGS += -Wall -ansi -pedantic
VERBOSE = -v

test: test.o test_hash.o teste_hash.o testd_hash.o

test_hash.c test_hash.h: sphash
	./sphash $(VERBOSE) --prefix test --out test_hash < words

teste_hash.c teste_hash.h: sphash
	./sphash $(VERBOSE) --multi --prefix teste --out teste_hash < wenums

testd_hash.c testd_hash.h: sphash
	./sphash $(VERBOSE) --aux "int i; double d;" --prefix testd --out testd_hash < denum

test_hash.o: test_hash.c test_hash.h
	$(CC) -c $(CFLAGS) test_hash.c -o test_hash.o

teste_hash.o: teste_hash.c teste_hash.h
	$(CC) -c $(CFLAGS) teste_hash.c -o teste_hash.o

test.o: test.c test_hash.h
	$(CC) -c $(CFLAGS) test.c -o test.o

sphash: sphash.c
	$(CC) $(CFLAGS) $(LDFLAGS) sphash.c -o sphash

install_: sphash
	mkdir -p $(install_root)$(DESTDIR)/usr/bin/
	$(CP) `pwd`/sphash $(install_root)$(DESTDIR)/usr/bin/sphash

install:
	make install_ CP=cp

linstall:
	make install_ CP="ln -s"

uninstall:
	rm $(install_root)/usr/bin/sphash

deb:
	fakeroot debian/rules binary

clean:
	rm -f *.o test_hash.c test_hash.h sphash test
