all: tester

ROOT=../..
include $(ROOT)/Makefile.conf

UNM=$(ROOT)/src_plugins/io_kicad
LHT=$(ROOT)/src_3rd
SRC=$(ROOT)/src
SRC_3RD=$(ROOT)/src_3rd
CFLAGS = $(PCB_RND_C89FLAGS) -I$(UNM) -I$(LHT) -I$(SRC) -I$(ROOT)
LDLIBS = -lm
LIB_OBJS=$(SRC_3RD)/librnd-local/src/librnd-hid.a $(SRC_3RD)/librnd-local/src/librnd-3rd.a


test: tester.diff
	@echo "uniq_name:           *** QC PASS ***"
	@rm tester.out

tester: tester.o $(UNM)/uniq_name.o $(LIB_OBJS)
	$(CC) $(LDFLAGS) -o tester tester.o $(UNM)/uniq_name.o $(LIB_OBJS)  $(LDLIBS)

tester.o: tester.c $(UNM)/uniq_name.h

$(UNM)/uniq_name.o: $(UNM)/uniq_name.c $(UNM)/uniq_name.h

tester.diff: tester.ref tester.out
	@diff -u tester.ref tester.out

tester.out: tester
	./tester > tester.out

clean:
	-$(SCCBOX) rm -f tester.out tester tester.o
