.PHONY: all
all: tr-tests
	./tr-tests

tr-tests: main build-script test.data.pl
	./main test.data.pl > $@.n
	mv $@.n $@
	chmod 755 $@

.PHONY: distclean
distclean:
	rm -f t[0-9]*.out

.PHONY: clean
clean: distclean

.PHONY: realclean
realclean: clean
	rm -f tr-tests t[0-9]*.in t[0-9]*.expected
