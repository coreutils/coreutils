## Process this file with automake to produce Makefile.in -*-Makefile-*-.

##test-files-begin
x = pr
explicit = 0F 0FF 0FFnt 0FFt 0FnFnt 0FnFt 0Fnt 0Ft 2-S_f-t_notab 2-Sf-t_notab \
2f-t_notab 2s_f-t_notab 2s_w60f-t_nota 2sf-t_notab 2sw60f-t_notab \
2w60f-t_notab 3-0F 3-5l17f-t 3-FF 3a2l17-FF 3a3f-0F 3a3l15-t 3a3l8f-t \
3b2l17-FF 3b3f-0F 3b3f-0FF 3b3f-FF 3b3l15-t 3b3l8f-t 3f-0F 3f-FF 3l17f-t \
3l24-t 3ml17f-t 3ml24-FF 3ml24-t 3ml24-t-FF 4-7l24-FF 4l24-FF FF FFn FFtn \
FnFn Ja3l17f-lm Jb3l17f-lm Jml17f-lm-lo W-72l17f-ll W20l17f-ll W26l17f-ll \
W27l17f-ll W28l17f-ll W35Ja3l17f-lm W35Jb3l17f-lm W35Jml17f-lmlo W35a3l17f-lm \
W35b3l17f-lm W35ml17f-lm-lo W72Jl17f-ll a2l15-FF a2l17-FF a3-0F a3f-0F \
a3f-0FF a3f-FF a3l15-t a3l17f-lm a3l8f-t b2l15-FF b2l17-FF b3-0F b3f-0F \
b3f-0FF b3f-FF b3l15-t b3l17f-lm b3l8f-t l17f-t l24-FF l24-t loli ml17f-0F \
ml17f-lm-lo ml17f-t ml17f-t-0F ml20-FF-t ml24-FF ml24-t ml24-t-FF \
n+2-5l17f-0FF n+2l17f-0FF n+2l17f-bl n+3-7l24-FF n+3l17f-0FF n+3l17f-bl \
n+3ml13f-bl-FF n+3ml17f-bl-tn n+3ml17f-tn-bl n+4-8a2l17-FF n+4b2l10f-0FF \
n+5-8b3l10f-FF n+5a3l6f-0FF n+6a2l17-FF n+6b3l6f-FF n+7l24-FF n+8l20-FF \
nJml17f-lmlmlo nJml17f-lmlolm nN1+3l17f-bl nN15l17f-bl nSml13-bl-FF \
nSml13-t-t-FF nSml13-t-tFFFF nSml17-bl-FF nSml17-t-t-FF nSml17-t-tFFFF \
nl17f-bl o3Jml17f-lm-lo o3a3Sl17f-tn o3a3Snl17f-tn o3a3l17f-tn o3b3Sl17f-tn \
o3b3Snl17f-tn o3b3l17f-tn o3mSl17f-bl-tn o3mSnl17fbltn o3ml17f-bl-tn t-0FF \
t-FF t-bl t-t tFFn tFFt tFFt-bl tFFt-ll tFFt-lm tFnFt t_notab t_tab t_tab_ \
ta3-0FF ta3-FF tb3-0FF tb3-FF tn tn2e5o3-t_tab tn2e8-t_tab tn2e8o3-t_tab \
tn_2e8-t_tab tn_2e8S-t_tab tne8-t_tab tne8o3-t_tab tt-0FF tt-FF tt-bl tt-t \
tta3-0FF tta3-FF ttb3-0FF ttb3-FF w72l17f-ll
maint_gen = i-opt-a.I i-opt-a.X i-opt-b.I i-opt-b.X ncut-a.I ncut-a.X \
ncut-b.I ncut-b.X margin-0.I margin-0.X dbl-sp-a.I dbl-sp-a.X dbl-sp-b.I \
dbl-sp-b.X narrow-1.I narrow-1.X col-last.I col-last.X col-02.I col-02.X \
col-02.C.I col-02.C.X col-2.I col-2.X col-2.C.I col-2.C.X col-long.I \
col-long.X col-long.C.I col-long.C.X col-0.I col-0.X col-0.C.I col-0.C.X \
col-inval.I col-inval.X col-inval.C.I col-inval.C.X neg-page.I neg-page.X
run_gen = 1a.O 1a.E 1b.O 1b.E 1c.O 1c.E 1d.O 1d.E 1e.O 1e.E 1e.C.O 1e.C.E \
1f.O 1f.E 1f.C.O 1f.C.E 1g.O 1g.E 1g.C.O 1g.C.E 1h.O 1h.E 1h.C.O 1h.C.E 1i.O \
1i.E 1i.C.O 1i.C.E 1j.O 1j.E 1j.C.O 1j.C.E 1k.O 1k.E 1k.C.O 1k.C.E 1l.O 1l.E \
1l.C.O 1l.C.E 2a.O 2a.E 2b.O 2b.E 2c.O 2c.E 2c.C.O 2c.C.E 2d.O 2d.E 2d.C.O \
2d.C.E 2e.O 2e.E 2e.C.O 2e.C.E 2f.O 2f.E 2f.C.O 2f.C.E 2g.O 2g.E 2g.C.O \
2g.C.E 2h.O 2h.E 2h.C.O 2h.C.E 2i.O 2i.E 2i.C.O 2i.C.E 2j.O 2j.E 2j.C.O \
2j.C.E 3a.O 3a.E 3b.O 3b.E 3c.O 3c.E 3d.O 3d.E 3e.O 3e.E 3e.C.O 3e.C.E 3f.O \
3f.E 3f.C.O 3f.C.E 3g.O 3g.E 3g.C.O 3g.C.E 3h.O 3h.E 3h.C.O 3h.C.E 3i.O 3i.E \
3i.C.O 3i.C.E 3j.O 3j.E 3j.C.O 3j.C.E 3k.O 3k.E 3k.C.O 3k.C.E 3l.O 3l.E \
3l.C.O 3l.C.E 3m.O 3m.E 3m.C.O 3m.C.E 3la.O 3la.E 3lb.O 3lb.E 3lc.O 3lc.E \
3ld.O 3ld.E 3le.O 3le.E 3lf.O 3lf.E 3aa.O 3aa.E 3aa.C.O 3aa.C.E 3ab.O 3ab.E \
3ab.C.O 3ab.C.E 3ac.O 3ac.E 3ac.C.O 3ac.C.E 3ad.O 3ad.E 3ad.C.O 3ad.C.E 3ae.O \
3ae.E 3ae.C.O 3ae.C.E 3af.O 3af.E 3af.C.O 3af.C.E 3ag.O 3ag.E 3ag.C.O 3ag.C.E \
3ah.O 3ah.E 3ah.C.O 3ah.C.E 3ai.O 3ai.E 3ai.C.O 3ai.C.E 3aj.O 3aj.E 3aj.C.O \
3aj.C.E 3ak.O 3ak.E 3ak.C.O 3ak.C.E 3ba.O 3ba.E 3ba.C.O 3ba.C.E 3bb.O 3bb.E \
3bb.C.O 3bb.C.E 3bc.O 3bc.E 3bc.C.O 3bc.C.E 3bd.O 3bd.E 3bd.C.O 3bd.C.E 3be.O \
3be.E 3be.C.O 3be.C.E 3bf.O 3bf.E 3bf.C.O 3bf.C.E 3bg.O 3bg.E 3bg.C.O 3bg.C.E \
3bh.O 3bh.E 3bh.C.O 3bh.C.E 3bi.O 3bi.E 3bi.C.O 3bi.C.E 3bj.O 3bj.E 3bj.C.O \
3bj.C.E 3bk.O 3bk.E 3bk.C.O 3bk.C.E 3Pa.O 3Pa.E 3Pb.O 3Pb.E 3Pc.O 3Pc.E 3Pd.O \
3Pd.E 3Pe.O 3Pe.E 3Pf.O 3Pf.E 3Pg.O 3Pg.E 3Paa.O 3Paa.E 3Paa.C.O 3Paa.C.E \
3Pab.O 3Pab.E 3Pab.C.O 3Pab.C.E 3Pac.O 3Pac.E 3Pac.C.O 3Pac.C.E 3Pad.O 3Pad.E \
3Pad.C.O 3Pad.C.E 3Pae.O 3Pae.E 3Pae.C.O 3Pae.C.E 3Paf.O 3Paf.E 3Paf.C.O \
3Paf.C.E 3Pba.O 3Pba.E 3Pba.C.O 3Pba.C.E 3Pbb.O 3Pbb.E 3Pbb.C.O 3Pbb.C.E \
3Pbc.O 3Pbc.E 3Pbc.C.O 3Pbc.C.E 3Pbd.O 3Pbd.E 3Pbd.C.O 3Pbd.C.E 3Pbe.O 3Pbe.E \
3Pbe.C.O 3Pbe.C.E 3Pbf.O 3Pbf.E 3Pbf.C.O 3Pbf.C.E 4a.O 4a.E 4b.O 4b.E 4c.O \
4c.E 4d.O 4d.E 4e.O 4e.E 4f.O 4f.E 4f.C.O 4f.C.E 4g.O 4g.E 4g.C.O 4g.C.E 4h.O \
4h.E 4h.C.O 4h.C.E 4i.O 4i.E 4i.C.O 4i.C.E 4j.O 4j.E 4j.C.O 4j.C.E 4k.O 4k.E \
4k.C.O 4k.C.E 4l.O 4l.E 4l.C.O 4l.C.E 4m.O 4m.E 4m.C.O 4m.C.E 5a.O 5a.E 5b.O \
5b.E 5c.O 5c.E 5d.O 5d.E 5e.O 5e.E 5f.O 5f.E 5g.O 5g.E 5ma.O 5ma.E 5mb.O \
5mb.E 5mc.O 5mc.E 5md.O 5md.E 5Pma.O 5Pma.E 5Pmb.O 5Pmb.E 5Pmc.O 5Pmc.E 6a.O \
6a.E 6b.O 6b.E 6c.O 6c.E 6d.O 6d.E 6e.O 6e.E 6f.O 6f.E 6g.O 6g.E 6aa.O 6aa.E \
6aa.C.O 6aa.C.E 6ab.O 6ab.E 6ab.C.O 6ab.C.E 6ac.O 6ac.E 6ac.C.O 6ac.C.E 6ba.O \
6ba.E 6ba.C.O 6ba.C.E 6bb.O 6bb.E 6bb.C.O 6bb.C.E 6bc.O 6bc.E 6bc.C.O 6bc.C.E \
7a.O 7a.E 7b.O 7b.E 7c.O 7c.E 7d.O 7d.E 7e.O 7e.E 7f.O 7f.E 7g.O 7g.E 7aa.O \
7aa.E 7aa.C.O 7aa.C.E 7ab.O 7ab.E 7ab.C.O 7ab.C.E 7ac.O 7ac.E 7ac.C.O 7ac.C.E \
7ba.O 7ba.E 7ba.C.O 7ba.C.E 7bb.O 7bb.E 7bb.C.O 7bb.C.E 7bc.O 7bc.E 7bc.C.O \
7bc.C.E 9a.O 9a.E 9b.O 9b.E 9Pa.O 9Pa.E 9Pb.O 9Pb.E 9Pc.O 9Pc.E 9Pe.O 9Pe.E \
9Pf.O 9Pf.E 9Pg.O 9Pg.E 9Ph.O 9Ph.E 9Pi.O 9Pi.E 9Pj.O 9Pj.E 9Pk.O 9Pk.E 9Pl.O \
9Pl.E 9Pm.O 9Pm.E 9Pn.O 9Pn.E 9Po.O 9Po.E 9Pp.O 9Pp.E 9Pq.O 9Pq.E 9Pr.O 9Pr.E \
9Ps.O 9Ps.E 9Pt.O 9Pt.E 9Paa.O 9Paa.E 9Paa.C.O 9Paa.C.E 9Pab.O 9Pab.E \
9Pab.C.O 9Pab.C.E 9Pac.O 9Pac.E 9Pac.C.O 9Pac.C.E 9Pad.O 9Pad.E 9Pad.C.O \
9Pad.C.E 9Pae.O 9Pae.E 9Pae.C.O 9Pae.C.E 9Paf.O 9Paf.E 9Paf.C.O 9Paf.C.E \
9Pag.O 9Pag.E 9Pag.C.O 9Pag.C.E 9Pah.O 9Pah.E 9Pah.C.O 9Pah.C.E 9Pai.O 9Pai.E \
9Pai.C.O 9Pai.C.E 9Paj.O 9Paj.E 9Paj.C.O 9Paj.C.E 9Pak.O 9Pak.E 9Pak.C.O \
9Pak.C.E 9Pba.O 9Pba.E 9Pba.C.O 9Pba.C.E 9Pbb.O 9Pbb.E 9Pbb.C.O 9Pbb.C.E \
9Pbc.O 9Pbc.E 9Pbc.C.O 9Pbc.C.E 9Pbd.O 9Pbd.E 9Pbd.C.O 9Pbd.C.E 9Pbe.O 9Pbe.E \
9Pbe.C.O 9Pbe.C.E 9Pbf.O 9Pbf.E 9Pbf.C.O 9Pbf.C.E 9Pbg.O 9Pbg.E 9Pbg.C.O \
9Pbg.C.E 9Pbh.O 9Pbh.E 9Pbh.C.O 9Pbh.C.E 9Pbi.O 9Pbi.E 9Pbi.C.O 9Pbi.C.E \
9Pbj.O 9Pbj.E 9Pbj.C.O 9Pbj.C.E 9Pbk.O 9Pbk.E 9Pbk.C.O 9Pbk.C.E 9Pma.O 9Pma.E \
9Pmb.O 9Pmb.E 9Pmc.O 9Pmc.E 10wa.O 10wa.E 10wb.O 10wb.E 10wc.O 10wc.E 10wd.O \
10wd.E 10we.O 10we.E 10wf.O 10wf.E 10wg.O 10wg.E 10wh.O 10wh.E 10ma.O 10ma.E \
10mb.O 10mb.E 10mc.O 10mc.E 10md.O 10md.E 10me.O 10me.E 10mf.O 10mf.E 10mg.O \
10mg.E 10mh.O 10mh.E 10aa.O 10aa.E 10aa.C.O 10aa.C.E 10ab.O 10ab.E 10ab.C.O \
10ab.C.E 10ac.O 10ac.E 10ac.C.O 10ac.C.E 10ad.O 10ad.E 10ad.C.O 10ad.C.E \
10ba.O 10ba.E 10ba.C.O 10ba.C.E 10bb.O 10bb.E 10bb.C.O 10bb.C.E 10bc.O 10bc.E \
10bc.C.O 10bc.C.E 10bd.O 10bd.E 10bd.C.O 10bd.C.E 11sa.O 11sa.E 11sb.O 11sb.E \
11se.O 11se.E 11sf.O 11sf.E 11sg.O 11sg.E 11sh.O 11sh.E 12aa.O 12aa.E \
12aa.C.O 12aa.C.E 12ab.O 12ab.E 12ab.C.O 12ab.C.E 12ac.O 12ac.E 12ac.C.O \
12ac.C.E 12ba.O 12ba.E 12ba.C.O 12ba.C.E 12bb.O 12bb.E 12bb.C.O 12bb.C.E \
12bc.O 12bc.E 12bc.C.O 12bc.C.E 12ma.O 12ma.E 12mb.O 12mb.E 12mc.O 12mc.E \
12md.O 12md.E 13a.O 13a.E 13b.O 13b.E 13ba.O 13ba.E 13ba.C.O 13ba.C.E 13bb.O \
13bb.E 13bb.C.O 13bb.C.E 13bc.O 13bc.E 13bc.C.O 13bc.C.E 13bd.O 13bd.E \
13bd.C.O 13bd.C.E 13be.O 13be.E 13be.C.O 13be.C.E 13bf.O 13bf.E 13bf.C.O \
13bf.C.E 14a.O 14a.E 14a.C.O 14a.C.E 14b.O 14b.E 14b.C.O 14b.C.E 14c.O 14c.E \
14c.C.O 14c.C.E 14d.O 14d.E 14d.C.O 14d.C.E 14e.O 14e.E 14e.C.O 14e.C.E 14f.O \
14f.E 14f.C.O 14f.C.E 14g.O 14g.E 14g.C.O 14g.C.E 14h.O 14h.E 14h.C.O 14h.C.E \
14i.O 14i.E 14i.C.O 14i.C.E 14j.O 14j.E 14j.C.O 14j.C.E 14k.O 14k.E 14k.C.O \
14k.C.E 14l.O 14l.E 14l.C.O 14l.C.E 14m.O 14m.E 14m.C.O 14m.C.E i-opt-a.O \
i-opt-a.E i-opt-b.O i-opt-b.E ncut-a.O ncut-a.E ncut-b.O ncut-b.E margin-0.O \
margin-0.E dbl-sp-a.O dbl-sp-a.E dbl-sp-b.O dbl-sp-b.E narrow-1.O narrow-1.E \
col-last.O col-last.E col-02.O col-02.E col-02.C.O col-02.C.E col-2.O col-2.E \
col-2.C.O col-2.C.E col-long.O col-long.E col-long.C.O col-long.C.E col-0.O \
col-0.E col-0.C.O col-0.C.E col-inval.O col-inval.E col-inval.C.O \
col-inval.C.E neg-page.O neg-page.E
##test-files-end

EXTRA_DIST = Test.pm $x-tests $(explicit) $(maint_gen)
noinst_SCRIPTS = $x-tests
TESTS_ENVIRONMENT = \
  PATH="`pwd`/../../src$(PATH_SEPARATOR)$$PATH"

editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,$(srcdir),g'

TESTS = $x-tests

mk_script = $(srcdir)/../mk-script
$(srcdir)/$x-tests: $(mk_script) Test.pm Makefile.am
	test '$x' = test && prog=../../src/$x || prog=$x; \
	$(PERL) -I$(srcdir) -w -- $(mk_script) $(srcdir) $$prog > $@.n
	mv $@.n $@
	chmod 755 $@

MAINTAINERCLEANFILES = $x-tests $(maint_gen)
CLEANFILES = $(run_gen)

check: $(maint_gen)

$(srcdir)/Makefile.am: ../Makefile.am.in Test.pm $(mk_script)
	rm -f $@ $@t
	sed -n '1,/^##test-files-begin/p' $(srcdir)/../Makefile.am.in > $@t
	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//'`; \
	  echo "x = $$tool" >> $@t
	$(PERL) -I$(srcdir) -w -- $(mk_script) $(srcdir) --list >> $@t
	sed -n '/^##test-files-end/,$$p' $(srcdir)/../Makefile.am.in >> $@t
	mv $@t $@
