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
dbl-sp-b.X narrow-1.I narrow-1.X col-last.I col-last.X
run_gen = 1a.O 1a.E 1b.O 1b.E 1c.O 1c.E 1d.O 1d.E 1e.O 1e.E 1f.O 1f.E 1g.O \
1g.E 1h.O 1h.E 1i.O 1i.E 1j.O 1j.E 1k.O 1k.E 1l.O 1l.E 2a.O 2a.E 2b.O 2b.E \
2c.O 2c.E 2d.O 2d.E 2e.O 2e.E 2f.O 2f.E 2g.O 2g.E 2h.O 2h.E 2i.O 2i.E 2j.O \
2j.E 3a.O 3a.E 3b.O 3b.E 3c.O 3c.E 3d.O 3d.E 3e.O 3e.E 3f.O 3f.E 3g.O 3g.E \
3h.O 3h.E 3i.O 3i.E 3j.O 3j.E 3k.O 3k.E 3l.O 3l.E 3m.O 3m.E 3la.O 3la.E 3lb.O \
3lb.E 3lc.O 3lc.E 3ld.O 3ld.E 3le.O 3le.E 3lf.O 3lf.E 3aa.O 3aa.E 3ab.O 3ab.E \
3ac.O 3ac.E 3ad.O 3ad.E 3ae.O 3ae.E 3af.O 3af.E 3ag.O 3ag.E 3ah.O 3ah.E 3ai.O \
3ai.E 3aj.O 3aj.E 3ak.O 3ak.E 3ba.O 3ba.E 3bb.O 3bb.E 3bc.O 3bc.E 3bd.O 3bd.E \
3be.O 3be.E 3bf.O 3bf.E 3bg.O 3bg.E 3bh.O 3bh.E 3bi.O 3bi.E 3bj.O 3bj.E 3bk.O \
3bk.E 3Pa.O 3Pa.E 3Pb.O 3Pb.E 3Pc.O 3Pc.E 3Pd.O 3Pd.E 3Pe.O 3Pe.E 3Pf.O 3Pf.E \
3Pg.O 3Pg.E 3Paa.O 3Paa.E 3Pab.O 3Pab.E 3Pac.O 3Pac.E 3Pad.O 3Pad.E 3Pae.O \
3Pae.E 3Paf.O 3Paf.E 3Pba.O 3Pba.E 3Pbb.O 3Pbb.E 3Pbc.O 3Pbc.E 3Pbd.O 3Pbd.E \
3Pbe.O 3Pbe.E 3Pbf.O 3Pbf.E 4a.O 4a.E 4b.O 4b.E 4c.O 4c.E 4d.O 4d.E 4e.O 4e.E \
4f.O 4f.E 4g.O 4g.E 4h.O 4h.E 4i.O 4i.E 4j.O 4j.E 4k.O 4k.E 4l.O 4l.E 4m.O \
4m.E 5a.O 5a.E 5b.O 5b.E 5c.O 5c.E 5d.O 5d.E 5e.O 5e.E 5f.O 5f.E 5g.O 5g.E \
5ma.O 5ma.E 5mb.O 5mb.E 5mc.O 5mc.E 5md.O 5md.E 5Pma.O 5Pma.E 5Pmb.O 5Pmb.E \
5Pmc.O 5Pmc.E 6a.O 6a.E 6b.O 6b.E 6c.O 6c.E 6d.O 6d.E 6e.O 6e.E 6f.O 6f.E \
6g.O 6g.E 6aa.O 6aa.E 6ab.O 6ab.E 6ac.O 6ac.E 6ba.O 6ba.E 6bb.O 6bb.E 6bc.O \
6bc.E 7a.O 7a.E 7b.O 7b.E 7c.O 7c.E 7d.O 7d.E 7e.O 7e.E 7f.O 7f.E 7g.O 7g.E \
7aa.O 7aa.E 7ab.O 7ab.E 7ac.O 7ac.E 7ba.O 7ba.E 7bb.O 7bb.E 7bc.O 7bc.E 9a.O \
9a.E 9b.O 9b.E 9Pa.O 9Pa.E 9Pb.O 9Pb.E 9Pc.O 9Pc.E 9Pe.O 9Pe.E 9Pf.O 9Pf.E \
9Pg.O 9Pg.E 9Ph.O 9Ph.E 9Pi.O 9Pi.E 9Pj.O 9Pj.E 9Pk.O 9Pk.E 9Pl.O 9Pl.E 9Pm.O \
9Pm.E 9Pn.O 9Pn.E 9Po.O 9Po.E 9Pp.O 9Pp.E 9Pq.O 9Pq.E 9Pr.O 9Pr.E 9Ps.O 9Ps.E \
9Pt.O 9Pt.E 9Paa.O 9Paa.E 9Pab.O 9Pab.E 9Pac.O 9Pac.E 9Pad.O 9Pad.E 9Pae.O \
9Pae.E 9Paf.O 9Paf.E 9Pag.O 9Pag.E 9Pah.O 9Pah.E 9Pai.O 9Pai.E 9Paj.O 9Paj.E \
9Pak.O 9Pak.E 9Pba.O 9Pba.E 9Pbb.O 9Pbb.E 9Pbc.O 9Pbc.E 9Pbd.O 9Pbd.E 9Pbe.O \
9Pbe.E 9Pbf.O 9Pbf.E 9Pbg.O 9Pbg.E 9Pbh.O 9Pbh.E 9Pbi.O 9Pbi.E 9Pbj.O 9Pbj.E \
9Pbk.O 9Pbk.E 9Pma.O 9Pma.E 9Pmb.O 9Pmb.E 9Pmc.O 9Pmc.E 10wa.O 10wa.E 10wb.O \
10wb.E 10wc.O 10wc.E 10wd.O 10wd.E 10we.O 10we.E 10wf.O 10wf.E 10wg.O 10wg.E \
10wh.O 10wh.E 10ma.O 10ma.E 10mb.O 10mb.E 10mc.O 10mc.E 10md.O 10md.E 10me.O \
10me.E 10mf.O 10mf.E 10mg.O 10mg.E 10mh.O 10mh.E 10aa.O 10aa.E 10ab.O 10ab.E \
10ac.O 10ac.E 10ad.O 10ad.E 10ba.O 10ba.E 10bb.O 10bb.E 10bc.O 10bc.E 10bd.O \
10bd.E 11sa.O 11sa.E 11sb.O 11sb.E 11se.O 11se.E 11sf.O 11sf.E 11sg.O 11sg.E \
11sh.O 11sh.E 12aa.O 12aa.E 12ab.O 12ab.E 12ac.O 12ac.E 12ba.O 12ba.E 12bb.O \
12bb.E 12bc.O 12bc.E 12ma.O 12ma.E 12mb.O 12mb.E 12mc.O 12mc.E 12md.O 12md.E \
13a.O 13a.E 13b.O 13b.E 13ba.O 13ba.E 13bb.O 13bb.E 13bc.O 13bc.E 13bd.O \
13bd.E 13be.O 13be.E 13bf.O 13bf.E 14a.O 14a.E 14b.O 14b.E 14c.O 14c.E 14d.O \
14d.E 14e.O 14e.E 14f.O 14f.E 14g.O 14g.E 14h.O 14h.E 14i.O 14i.E 14j.O 14j.E \
14k.O 14k.E 14l.O 14l.E 14m.O 14m.E i-opt-a.O i-opt-a.E i-opt-b.O i-opt-b.E \
ncut-a.O ncut-a.E ncut-b.O ncut-b.E margin-0.O margin-0.E dbl-sp-a.O \
dbl-sp-a.E dbl-sp-b.O dbl-sp-b.E narrow-1.O narrow-1.E col-last.O col-last.E
##test-files-end

EXTRA_DIST = Test.pm $x-tests $(explicit) $(maint_gen)
noinst_SCRIPTS = $x-tests

PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''srcdir''@,@srcdir@,g'

TESTS = $x-tests

mk_script = $(srcdir)/../mk-script
$(srcdir)/$x-tests: $(mk_script) Test.pm
	$(PERL) -I$(srcdir) -w -- $(mk_script) ../../src/$x > $@.n
	mv $@.n $@
	chmod 755 $@

MAINTAINERCLEANFILES = $x-tests $(maint_gen)
CLEANFILES = $(run_gen)

check: $(maint_gen)

Makefile.am: ../Makefile.am.in Test.pm $(mk_script)
	rm -f $@ $@t
	sed -n '1,/^##test-files-begin/p' $< > $@t
	tool=`echo $(subdir)|sed 's/^tests.//;s/-test//'`; \
	  echo "x = $$tool" >> $@t
	$(PERL) -I$(srcdir) -w -- $(mk_script) --list >> $@t
	sed -n '/^##test-files-end/,$$p' $< >> $@t
	mv $@t $@
