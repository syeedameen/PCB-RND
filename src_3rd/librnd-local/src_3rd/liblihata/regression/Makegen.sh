#!/bin/sh

# make sure sort order is the same
export LANG=C

# This will not work if:
#  - there's no ls, /bin/sh or awk installed (but the output is provided so only new tests are affected, can be done manually)
#  - test names contain space (they won't)
#  - instead of setting RS to "[ \t\r\n]" and deal with records, we need to read file names from fields - some awk implementations won't allow regex in RS
ls *.lht *.tts | sort | awk -v "exfn=$1" '
	BEGIN {
		lht_names = 0;
		tts_names = 0;
		out_dir = "out/"
		ref_dir = "ref/"
		valg = "valgrind -v --show-reachable=yes --leak-check=full"
		while((getline < exfn) > 0)
			EXCL[$1]++
	}

	/.lht$/ {
		if ($1 in EXCL)
			next
		name=$0
		sub(".lht$", "", name)
		LHT_NAME[lht_names] = name;
		LHTS[name]++
		lht_names++
		next
	}

	/.tts$/ {
		if ($1 in EXCL)
			next
		name=$0
		sub(".tts$", "", name)
		TTS_NAME[tts_names] = name;
		TTSS[name]++
		tts_names++
		next
	}

	{
		print "WARNING: unknown test suffix " $0 " - not generating test rule for that" > "/dev/stderr"
	}

	# calculate lht dependencies of a tts file by reading all load_doc commands
	function tts_deps(fn    ,deps, DEPS)
	{
		close(fn)
		deps = ""
		while((getline < fn) > 0) {
			if ($1 == "load_doc") {
				if (!($3 in DEPS)) {
					deps = deps " " $3
					DEPS[$3]++
				}
			}
		}
		close(fn)
		return deps
	}

	# generate rules for .lht: run parser and dom parser tests
	function lht_rule(name) {
		print ""
		print "# " name
		print out_dir name ".eout: " name ".lht $(ETESTER)"
		print "	@$(ETESTER) < " name ".lht >$@"
		print ""
		print out_dir name ".dout: " name ".lht $(DTESTER)"
		print "	@$(DTESTER) < " name ".lht >$@"
		print ""
		print name ".ediff: " out_dir name ".eout " ref_dir name ".eref"
		print "	@diff -u " ref_dir name ".eref " out_dir name ".eout"
		print "	@echo \"ev  " name "\": ok. >> $(LOG)"
		print ""
		print name ".ddiff: " out_dir name ".dout " ref_dir name ".dref"
		print "	@diff -u " ref_dir name ".dref " out_dir name ".dout"
		print "	@echo \"dom " name "\": ok. >> $(LOG)"
		print ""
	}

# generate tree_test rules for a .tts file
	function tts_rule(name) {
		print ""
		print "# " name
		print out_dir name ".tout: " name ".tts $(TTESTER)" tts_deps(name ".tts")
		print "	@$(TTESTER) < " name ".tts >$@"
		print ""
		print name ".tdiff: " out_dir name ".tout " ref_dir name ".tref"
		print "	@diff -u " ref_dir name ".tref " out_dir name ".tout"
		print "	@echo \"tts " name "\": ok. >> $(LOG)"
		print ""
	}

# generate common rules for tests, depending on whether name is
# a .lht and/or a .tts test
	function common_rule(name) {
		if (name in COMMON_SEEN)
			return

		COMMON_SEEN[name]++

		print name ".REF: "   ((name in LHTS) ? name ".lht" : ""), ((name in TTSS) ? name ".tts" : "")
		if (name in LHTS) {
			print "	@$(ETESTER) < " name ".lht >" ref_dir name ".eref"
			print "	@$(DTESTER) < " name ".lht >" ref_dir name ".dref"
			print "	@echo \"*** WARNING: missing reference " ref_dir name ".eref and .dref have been generated. Please validate ***\""
		}
		if (name in TTSS) {
			print "	@$(TTESTER) < " name ".tts >" ref_dir name ".tref"
			print "	@echo \"*** WARNING: missing reference " ref_dir name ".tref has been generated. Please validate ***\""
		}
		print ""

		if (name in LHTS)
			lht = name ".lht "
		else
			lht = ""
		print out_dir name ".valg: " lht "$(ETESTER)"
		if (name in LHTS) {
			print "	" valg " --log-file=" name ".evalg $(ETESTER) < " name ".lht > /dev/null"
			print "	" valg " --log-file=" name ".dvalg $(DTESTER) < " name ".lht > /dev/null"
		}
		if (name in TTSS)
			print "	" valg " --log-file=" name ".tvalg $(TTESTER) < " name ".tts > /dev/null"

		print ""
	}


# generate all rules, starting with the invoke-all ones
	END {
		printf("all:");

		for(n = 0; n < lht_names; n++)
			printf(" %s.ediff", LHT_NAME[n]);

# run dom test only after evtests - the errors found by the first batch will
# break the tree and cause errors in the dom batch as well
		for(n = 0; n < lht_names; n++)
			printf(" %s.ddiff", LHT_NAME[n]);

		for(n = 0; n < tts_names; n++)
			printf(" %s.tdiff", TTS_NAME[n]);

		print ""
		print "	@awk -f Eval.awk < $(LOG)"
		print ""

		printf("valg:");
		for(n = 0; n < lht_names; n++)
			printf(" %s%s.valg", out_dir, LHT_NAME[n]);
		for(n = 0; n < tts_names; n++)
			printf(" %s%s.valg", out_dir, TTS_NAME[n]);
		print ""
		print ""


		print "clean:"
		printf("	@rm $(LOG) ")
		for(n = 0; n < lht_names; n++) {
			printf(" %s%s.eout", out_dir, LHT_NAME[n]);
			printf(" %s%s.dout", out_dir, LHT_NAME[n]);
			printf(" %s%s.valg", out_dir, LHT_NAME[n]);
		}
		for(n = 0; n < tts_names; n++) {
			printf(" %s%s.tout", out_dir, TTS_NAME[n]);
		}
		print " 2>/dev/null ; true"
		print ""

		print "############## lht (parser/dom) rules ##############"
		for(n = 0; n < lht_names; n++)
			lht_rule(LHT_NAME[n])

		print "############## tts (tree tester) rules ##############"
		for(n = 0; n < tts_names; n++)
			tts_rule(TTS_NAME[n])

		print "############## common rules ##############"
		for(n = 0; n < lht_names; n++)
			common_rule(LHT_NAME[n])
		for(n = 0; n < tts_names; n++)
			common_rule(TTS_NAME[n])

		print "############## build testers ##############"
		print "$(ETESTER) $(DTESTER) $(TTESTER):"
		print "	cd .. && make"

	}
'
