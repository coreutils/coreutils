.PHONY: all
all: join-tests
	./join-tests

join-tests: main build-script test.data.pl
	./main test.data.pl > $@.n
	mv $@.n $@
	chmod 755 $@

.PHONY: distclean
distclean:
	rm -f t*.out core

.PHONY: clean
clean: distclean

.PHONY: maintainer-clean realclean
maintainer-clean realclean: clean
	rm -f join-tests t*.[12] t*.exp t*.log
