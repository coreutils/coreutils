.PHONY: all
all: run-tests
	./run-tests

run-tests: main build-script test.data.pl
	./main test.data.pl > $@.n
	mv $@.n $@
	chmod 755 $@

.PHONY: distclean
distclean:
	rm -f t*.out

.PHONY: clean
clean: distclean

.PHONY: realclean maintainer-clean
maintainer-clean realclean: clean
	rm -f run-tests t*.in t*.exp t*.err
