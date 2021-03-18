for lht in $*
do
	../perstest $lht
	diff -u $lht $lht.out
	if test $? = 0
	then
		rm $lht.out
	else
		fail="$fail $lht"
	fi
done

if test -z "$fail"
then
	echo "*** All good. QC PASS. ***"
else
	echo "Failed: $fail"
fi