.PHONY: all
all: tr-tests
	./tr-tests

tr-tests: main build-script test.data.pl
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
	rm -f tr-tests t*.in t*.exp
