/: ok/ { pass++ }
END {
	if (pass > 0)
		print "*** QC PASS (" int(pass) " test cases - the full list is in Tests.log) ***"
	else
		print "*** QC PASS (no change in test cases or tester - no actions taken) ***"
}
