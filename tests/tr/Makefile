all: tr-tests
	./tr-tests
.PHONY: all

tr-tests: main build-script test.data.pl
	./main test.data.pl > .tmp-$@
	mv .tmp-$@ $@
	chmod 755 $@

clean:
	rm -f t[0-9]*.out
.PHONY: clean

realclean:
	rm -f tr-tests t[0-9]*.in t[0-9]*.expected
.PHONY: realclean
