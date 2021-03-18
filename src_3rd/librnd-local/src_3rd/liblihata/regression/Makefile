ETESTER=../test_parser
DTESTER=../test_dom
TTESTER=../test_tree
LOG=Tests.log

all: Makefile.test
	@echo "Normal" > $(LOG)
	@make -f Makefile.test ETESTER=$(ETESTER) DTESTER=$(DTESTER) TTESTER=$(TTESTER) LOG=$(LOG)

# This rule will work only on modern UNIX systems (GNU/Linux)
# but this is not a big loss: it's for developers to check
# memory leaks using valgrind, which is not available on
# too many systems anyway.
valg: Makefile.test
	@echo "Valgrind" > $(LOG)
	@make -f Makefile.test valg ETESTER=$(ETESTER) DTESTER=$(DTESTER) TTESTER=$(TTESTER) LOG=$(LOG)
	@echo "*** Valgrind result summary (only leaky tests listed): ***"
	@grep -H "ERROR SUMMARY:" *.evalg *.dvalg | grep -v  "ERROR SUMMARY: 0 errors" ; true

force:
	@make clean && make all

Makefile.test: Makefile Makegen.sh *.tts *.lht Tests.exclude
	./Makegen.sh Tests.exclude > Makefile.test

.SUFFIXES: .REF .lht .tts

.lht.REF: Makefile.test
	@echo "" > $(LOG)
	@make -f Makefile.test $@ ETESTER=$(ETESTER) DTESTER=$(DTESTER) TTESTER=$(TTESTER) LOG=$(LOG)

.tts.REF: Makefile.test
	@echo "" > $(LOG)
	@make -f Makefile.test $@ ETESTER=$(ETESTER) DTESTER=$(DTESTER) TTESTER=$(TTESTER) LOG=$(LOG)

leak:
	grep -i lost *valg | awk -F '[:]' '{ print $$1 }' | uniq


clean: Makefile.test
	@make -f Makefile.test $@ ETESTER=$(ETESTER) DTESTER=$(DTESTER) TTESTER=$(TTESTER) LOG=$(LOG)

Tests.exclude:
	echo "" > Tests.exclude
