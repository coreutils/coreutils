.PHONY: all
all: join-tests
	./join-tests

join-tests: main build-script test.data.pl
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
	rm -f join-tests t*.in t*.exp
