.PHONY: all
all: sort-tests
	./sort-tests

sort-tests: main build-script test.data.pl
	./main test.data.pl > $@.n
	mv $@.n $@
	chmod 755 $@

.PHONY: distclean
distclean:
	rm -f t*.out

.PHONY: clean
clean: distclean

.PHONY: realclean
realclean: clean
	rm -f sort-tests t*.in t*.exp t*.err
