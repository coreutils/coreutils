N = 1000000

dot30 = ..............................
dot31 = $(dot30).

safe_redirect_prologue = rm -f $@ $@-tmp
safe_redirect = $@-tmp && mv $@-tmp $@

time = /p/bin/time --verbose

all: t30 t31

t3%: in-3%
	@$(safe_redirect_prologue)
	$(time) sort $< > $(safe_redirect)

rand-digits: rand-gen
	@$(safe_redirect_prologue)
	./$< --integer $(N) |tr -d '\012' > $(safe_redirect)

in-30: rand-digits
	@$(safe_redirect_prologue)
	perl -pe 's/($(dot30))/$$1\n/g' $< > $(safe_redirect)

in-31: rand-digits
	@$(safe_redirect_prologue)
	perl -pe 's/($(dot31))/$$1\n/g' $< > $(safe_redirect)

clean:
	rm -f t30 t31 in-30 in-31

realclean: clean
	rm -f rand-digits
