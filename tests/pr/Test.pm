# -*-perl-*-
package Test;
require 5.002;
use strict;

my @tv = (

# -b option is no longer an official option. But it's still working to
# get a downward compatibility. Now (version 1.19m or newer) -COLUMN
# only is equivalent to -b -COLUMN.
#
# test_name options input-file-name(s) expected-output-group-file-name
#                                                         expected-return-code
# -------------------------------------------------------------------------
# Following tests with "FF set" don't contain a complete set of all
# combinations of options and FF-arrangements
#
# One FF at start of file (one empty page)
['1a', '', [\'0Ft'], [\'0F'], 0],
['1b', '', [\'0Fnt'], [\'0F'], 0],
['1c', '+3', [\'0Ft'], [\'3-0F'], 0],
['1d', '+3 -f', [\'0Ft'], [\'3f-0F'], 0],
['1e', '-a -3', [\'0Ft'], [\'a3-0F'], 0],
['1f', '-a -3 -f', [\'0Ft'], [\'a3f-0F'], 0],
['1g', '-a -3 -f', [\'0Fnt'], [\'a3f-0F'], 0],
['1h', '+3 -a -3 -f', [\'0Ft'], [\'3a3f-0F'], 0],
['1i', '-b -3', [\'0Ft'], [\'b3-0F'], 0],
['1j', '-b -3 -f', [\'0Ft'], [\'b3f-0F'], 0],
['1k', '-b -3 -f', [\'0Fnt'], [\'b3f-0F'], 0],
['1l', '+3 -b -3 -f', [\'0Ft'], [\'3b3f-0F'], 0],
# Two FF at start of file (two empty page)
['2a', '', [\'0FFt'], [\'0FF'], 0],
['2b', '', [\'0FnFnt'], [\'0FF'], 0],
['2c', '-a -3 -f', [\'0FFt'], [\'a3f-0FF'], 0],
['2d', '-a -3 -f', [\'0FFnt'], [\'a3f-0FF'], 0],
['2e', '-b -3 -f', [\'0FFt'], [\'b3f-0FF'], 0],
['2f', '-b -3 -f', [\'0FFnt'], [\'b3f-0FF'], 0],
['2g', '-b -3 -f', [\'0FnFt'], [\'b3f-0FF'], 0],
['2h', '-b -3 -f', [\'0FnFnt'], [\'b3f-0FF'], 0],
['2i', '+3 -b -3 -f', [\'0FFt'], [\'3b3f-0FF'], 0],
['2j', '+3 -b -3 -f', [\'0FFnt'], [\'3b3f-0FF'], 0],
#
# FFs in text (none / one / two empty page(s))
['3a', '', [\'FFn'], [\'FF'], 0],
['3b', '', [\'FnFn'], [\'FF'], 0],
['3c', '+3', [\'FFn'], [\'3-FF'], 0],
['3d', '+3 -f', [\'FFn'], [\'3f-FF'], 0],
['3e', '-a -3 -f', [\'FFn'], [\'a3f-FF'], 0],
['3f', '-a -3 -f', [\'FFtn'], [\'a3f-FF'], 0],
['3g', '-b -3 -f', [\'FFn'], [\'b3f-FF'], 0],
['3h', '-b -3 -f', [\'FFtn'], [\'b3f-FF'], 0],
['3i', '-b -3 -f', [\'FnFn'], [\'b3f-FF'], 0],
['3j', '-b -3 -f', [\'tFFn'], [\'b3f-FF'], 0],
['3k', '-b -3 -f', [\'tFnFt'], [\'b3f-FF'], 0],
['3l', '+3 -b -3 -f', [\'FFn'], [\'3b3f-FF'], 0],
['3m', '+3 -b -3 -f', [\'FFtn'], [\'3b3f-FF'], 0],
# A full page printed (lines_left_on_page = 0) without a FF found.
# Avoid an extra empty page if a FF follows in the next input line.
['3la', '-l 24', [\'FFn'], [\'l24-FF'], 0],
['3lb', '-l 24', [\'FFtn'], [\'l24-FF'], 0],
['3lc', '-l 24', [\'FnFn'], [\'l24-FF'], 0],
['3ld', '-l 24', [\'tFFn'], [\'l24-FF'], 0],
['3le', '-l 24', [\'tFnFt'], [\'l24-FF'], 0],
['3lf', '-l 24', [\'tFFt'], [\'l24-FF'], 0],
['3aa', '-a -2 -l 17', [\'FFn'], [\'a2l17-FF'], 0],
['3ab', '-a -2 -l 17', [\'FFtn'], [\'a2l17-FF'], 0],
['3ac', '-a -2 -l 17', [\'FnFn'], [\'a2l17-FF'], 0],
['3ad', '-a -2 -l 17', [\'tFFn'], [\'a2l17-FF'], 0],
['3ae', '-a -2 -l 17', [\'tFnFt'], [\'a2l17-FF'], 0],
['3af', '-a -2 -l 17', [\'tFFt'], [\'a2l17-FF'], 0],
['3ag', '-a -2 -l 15', [\'FFn'], [\'a2l15-FF'], 0],
['3ah', '-a -2 -l 15', [\'FFtn'], [\'a2l15-FF'], 0],
['3ai', '-a -2 -l 15', [\'FnFn'], [\'a2l15-FF'], 0],
['3aj', '-a -2 -l 15', [\'tFFn'], [\'a2l15-FF'], 0],
['3ak', '-a -2 -l 15', [\'tFnFt'], [\'a2l15-FF'], 0],
['3ba', '-b -2 -l 17', [\'FFn'], [\'b2l17-FF'], 0],
['3bb', '-b -2 -l 17', [\'FFtn'], [\'b2l17-FF'], 0],
['3bc', '-b -2 -l 17', [\'FnFn'], [\'b2l17-FF'], 0],
['3bd', '-b -2 -l 17', [\'tFFn'], [\'b2l17-FF'], 0],
['3be', '-b -2 -l 17', [\'tFnFt'], [\'b2l17-FF'], 0],
['3bf', '-b -2 -l 17', [\'tFFt'], [\'b2l17-FF'], 0],
['3bg', '-b -2 -l 15', [\'FFn'], [\'b2l15-FF'], 0],
['3bh', '-b -2 -l 15', [\'FFtn'], [\'b2l15-FF'], 0],
['3bi', '-b -2 -l 15', [\'FnFn'], [\'b2l15-FF'], 0],
['3bj', '-b -2 -l 15', [\'tFFn'], [\'b2l15-FF'], 0],
['3bk', '-b -2 -l 15', [\'tFnFt'], [\'b2l15-FF'], 0],
['3Pa', '+4 -l 24', [\'FFn'], [\'4l24-FF'], 0],
['3Pb', '+4 -l 24', [\'FFtn'], [\'4l24-FF'], 0],
['3Pc', '+4 -l 24', [\'FnFn'], [\'4l24-FF'], 0],
['3Pd', '+4 -l 24', [\'tFFn'], [\'4l24-FF'], 0],
['3Pe', '+4 -l 24', [\'tFnFt'], [\'4l24-FF'], 0],
['3Pf', '+4 -l 24', [\'tFFt'], [\'4l24-FF'], 0],
['3Pg', '+4:7 -l 24', [\'tFFt'], [\'4-7l24-FF'], 0],
['3Paa', '+3 -a -2 -l 17', [\'FFn'], [\'3a2l17-FF'], 0],
['3Pab', '+3 -a -2 -l 17', [\'FFtn'], [\'3a2l17-FF'], 0],
['3Pac', '+3 -a -2 -l 17', [\'FnFn'], [\'3a2l17-FF'], 0],
['3Pad', '+3 -a -2 -l 17', [\'tFFn'], [\'3a2l17-FF'], 0],
['3Pae', '+3 -a -2 -l 17', [\'tFnFt'], [\'3a2l17-FF'], 0],
['3Paf', '+3 -a -2 -l 17', [\'tFFt'], [\'3a2l17-FF'], 0],
['3Pba', '+3 -b -2 -l 17', [\'FFn'], [\'3b2l17-FF'], 0],
['3Pbb', '+3 -b -2 -l 17', [\'FFtn'], [\'3b2l17-FF'], 0],
['3Pbc', '+3 -b -2 -l 17', [\'FnFn'], [\'3b2l17-FF'], 0],
['3Pbd', '+3 -b -2 -l 17', [\'tFFn'], [\'3b2l17-FF'], 0],
['3Pbe', '+3 -b -2 -l 17', [\'tFnFt'], [\'3b2l17-FF'], 0],
['3Pbf', '+3 -b -2 -l 17', [\'tFFt'], [\'3b2l17-FF'], 0],
#
# Without "FF set by hand"
['4a', '-l 24', [\'tn'], [\'l24-t'], 0],
['4b', '-l 17 -f', [\'tn'], [\'l17f-t'], 0],
['4c', '+3 -l 24', [\'tn'], [\'3l24-t'], 0],
['4d', '+3 -l 17 -f', [\'tn'], [\'3l17f-t'], 0],
['4e', '+3:5 -l 17 -f', [\'tn'], [\'3-5l17f-t'], 0],
['4f', '-a -3 -l 15', [\'tn'], [\'a3l15-t'], 0],
['4g', '-a -3 -l 8 -f', [\'tn'], [\'a3l8f-t'], 0],
['4h', '+3 -a -3 -l 15', [\'tn'], [\'3a3l15-t'], 0],
['4i', '+3 -a -3 -l 8 -f', [\'tn'], [\'3a3l8f-t'], 0],
['4j', '-b -3 -l 15', [\'tn'], [\'b3l15-t'], 0],
['4k', '-b -3 -l 8 -f', [\'tn'], [\'b3l8f-t'], 0],
['4l', '+3 -b -3 -l 15', [\'tn'], [\'3b3l15-t'], 0],
['4m', '+3 -b -3 -l 8 -f', [\'tn'], [\'3b3l8f-t'], 0],
#
# Merge input files (-m option)
['5a', '-m -l 24', [\'tn', \'tn'], [\'ml24-t'], 0],
['5b', '-m -l 17 -f', [\'tn', \'tn'], [\'ml17f-t'], 0],
['5c', '+3 -m -l 24', [\'tn', \'tn'], [\'3ml24-t'], 0],
['5d', '+3 -m -l 17 -f', [\'tn', \'tn'], [\'3ml17f-t'], 0],
['5e', '-m -l 17 -f', [\'0Ft', \'0Ft'], [\'ml17f-0F'], 0],
['5f', '-m -l 17 -f', [\'0Ft', \'0Fnt'], [\'ml17f-0F'], 0],
['5g', '-m -l 17 -f', [\'tn', \'0Ft'], [\'ml17f-t-0F'], 0],
# A full page printed (lines_left_on_page = 0) without a FF found.
# Avoid an extra empty page if a FF follows in the next input line.
['5ma', '-m -l 24', [\'tFFn', \'tFnFt'], [\'ml24-FF'], 0],
['5mb', '-m -l 24', [\'tFFn', \'FFn'], [\'ml24-FF'], 0],
['5mc', '-m -l 24', [\'tn', \'FFn'], [\'ml24-t-FF'], 0],
['5md', '-m -l 20', [\'FFn', \'tn'], [\'ml20-FF-t'], 0],
['5Pma', '+3 -m -l 24', [\'FFn', \'FnFn'], [\'3ml24-FF'], 0],
['5Pmb', '+3 -m -l 24', [\'tFFn', \'FFn'], [\'3ml24-FF'], 0],
['5Pmc', '+3 -m -l 24', [\'tn', \'FFn'], [\'3ml24-t-FF'], 0],
#
# Do not print header and footer but retain "FF set by Hand"
# (-t option)
['6a', '-t -l 24', [\'tn'], [\'t-t'], 0],
['6b', '-t -l 17 -f', [\'tn'], [\'t-t'], 0],
['6c', '-t -l 17 -f', [\'tFFt-bl'], [\'t-bl'], 0],
['6d', '-t -l 24', [\'0FnFnt'], [\'t-0FF'], 0],
['6e', '-t -l 24', [\'FFn'], [\'t-FF'], 0],
['6f', '-t -l 24', [\'FnFn'], [\'t-FF'], 0],
['6g', '-t -l 17 -f', [\'FFn'], [\'t-FF'], 0],
['6aa', '-t -a -3', [\'0FnFnt'], [\'ta3-0FF'], 0],
['6ab', '-t -a -3', [\'FFn'], [\'ta3-FF'], 0],
['6ac', '-t -a -3 -l 24', [\'FnFn'], [\'ta3-FF'], 0],
['6ba', '-t -b -3', [\'0FnFnt'], [\'tb3-0FF'], 0],
['6bb', '-t -b -3', [\'FFn'], [\'tb3-FF'], 0],
['6bc', '-t -b -3 -l 24', [\'FnFn'], [\'tb3-FF'], 0],
#
# Do not print header and footer nor "FF set by Hand" (-T option)
['7a', '-T -l 24', [\'tn'], [\'T-t'], 0],
['7b', '-T -l 17 -f', [\'tn'], [\'T-t'], 0],
['7c', '-T -l 17 -f', [\'tFFt-bl'], [\'T-bl'], 0],
['7d', '-T -l 24', [\'0FnFnt'], [\'T-0FF'], 0],
['7e', '-T -l 24', [\'FFn'], [\'T-FF'], 0],
['7f', '-T -l 24', [\'FnFn'], [\'T-FF'], 0],
['7g', '-T -l 17 -f', [\'FFn'], [\'T-FF'], 0],
['7aa', '-T -a -3', [\'0FnFnt'], [\'Ta3-0FF'], 0],
['7ab', '-T -a -3', [\'FFn'], [\'Ta3-FF'], 0],
['7ac', '-T -a -3 -l 24', [\'FnFn'], [\'Ta3-FF'], 0],
['7ba', '-T -b -3', [\'0FnFnt'], [\'Tb3-0FF'], 0],
['7bb', '-T -b -3', [\'FFn'], [\'Tb3-FF'], 0],
['7bc', '-T -b -3 -l 24', [\'FnFn'], [\'Tb3-FF'], 0],
#
# lhs-truncation of header
# pr-1.19m: Text line truncation only with column output
#
# numbering lines (-n  -N option)
# skip pages (+FIRST_PAGE[:LAST_PAGE] option)
['9a', '-n.3 -l 17 -f', [\'tFFt-bl'], [\'nl17f-bl'], 0],
['9b', '-n.3 -N 15 -l 17 -f', [\'tFFt-bl'], [\'nN15l17f-bl'], 0],
['9Pa', '-n.3 +2 -l 17 -f', [\'tFFt-bl'], [\'n+2l17f-bl'], 0],
['9Pb', '-n.3 +3 -l 17 -f', [\'tFFt-bl'], [\'n+3l17f-bl'], 0],
['9Pc', '-n.3 -N 1 +3 -l 17 -f', [\'tFFt-bl'], [\'nN1+3l17f-bl'], 0],
['9Pe', '-n.3 +2 -l 17 -f', [\'0FFt'], [\'n+2l17f-0FF'], 0],
['9Pf', '-n.3 +2 -l 17 -f', [\'0FFnt'], [\'n+2l17f-0FF'], 0],
['9Pg', '-n.3 +2 -l 17 -f', [\'0FnFt'], [\'n+2l17f-0FF'], 0],
['9Ph', '-n.3 +2 -l 17 -f', [\'0FnFnt'], [\'n+2l17f-0FF'], 0],
['9Pi', '-n.3 +2:5 -l 17 -f', [\'0FFt'], [\'n+2-5l17f-0FF'], 0],
['9Pj', '-n.3 +3 -l 17 -f', [\'0FFt'], [\'n+3l17f-0FF'], 0],
['9Pk', '-n.3 +3 -l 17 -f', [\'0FFnt'], [\'n+3l17f-0FF'], 0],
['9Pl', '-n.3 +3 -l 17 -f', [\'0FnFt'], [\'n+3l17f-0FF'], 0],
['9Pm', '-n.3 +3 -l 17 -f', [\'0FnFnt'], [\'n+3l17f-0FF'], 0],
['9Pn', '-n.3 +7 -l 24', [\'FFn'], [\'n+7l24-FF'], 0],
['9Po', '-n.3 +7 -l 24', [\'FFtn'], [\'n+7l24-FF'], 0],
['9Pp', '-n.3 +7 -l 24', [\'FnFn'], [\'n+7l24-FF'], 0],
['9Pq', '-n.3 +3:7 -l 24', [\'FnFn'], [\'n+3-7l24-FF'], 0],
['9Pr', '-n.3 +8 -l 20', [\'tFFn'], [\'n+8l20-FF'], 0],
['9Ps', '-n.3 +8 -l 20', [\'tFnFt'], [\'n+8l20-FF'], 0],
['9Pt', '-n.3 +8 -l 20', [\'tFFt'], [\'n+8l20-FF'], 0],
['9Paa', '-n.3 +5 -a -3 -l 6 -f', [\'0FFt'], [\'n+5a3l6f-0FF'], 0],
['9Pab', '-n.3 +5 -a -3 -l 6 -f', [\'0FFnt'], [\'n+5a3l6f-0FF'], 0],
['9Pac', '-n.3 +5 -a -3 -l 6 -f', [\'0FnFt'], [\'n+5a3l6f-0FF'], 0],
['9Pad', '-n.3 +5 -a -3 -l 6 -f', [\'0FnFnt'], [\'n+5a3l6f-0FF'], 0],
['9Pae', '-n.3 +6 -a -2 -l 17', [\'FFn'], [\'n+6a2l17-FF'], 0],
['9Paf', '-n.3 +6 -a -2 -l 17', [\'FFtn'], [\'n+6a2l17-FF'], 0],
['9Pag', '-n.3 +6 -a -2 -l 17', [\'FnFn'], [\'n+6a2l17-FF'], 0],
['9Pah', '-n.3 +6 -a -2 -l 17', [\'tFFn'], [\'n+6a2l17-FF'], 0],
['9Pai', '-n.3 +6 -a -2 -l 17', [\'tFnFt'], [\'n+6a2l17-FF'], 0],
['9Paj', '-n.3 +6 -a -2 -l 17', [\'tFFt'], [\'n+6a2l17-FF'], 0],
['9Pak', '-n.3 +4:8 -a -2 -l 17', [\'tFFt'], [\'n+4-8a2l17-FF'], 0],
['9Pba', '-n.3 +4 -b -2 -l 10 -f', [\'0FFt'], [\'n+4b2l10f-0FF'], 0],
['9Pbb', '-n.3 +4 -b -2 -l 10 -f', [\'0FFnt'], [\'n+4b2l10f-0FF'], 0],
['9Pbc', '-n.3 +4 -b -2 -l 10 -f', [\'0FnFt'], [\'n+4b2l10f-0FF'], 0],
['9Pbd', '-n.3 +4 -b -2 -l 10 -f', [\'0FnFnt'], [\'n+4b2l10f-0FF'], 0],
['9Pbe', '-n.3 +6 -b -3 -l 6 -f', [\'FFn'], [\'n+6b3l6f-FF'], 0],
['9Pbf', '-n.3 +6 -b -3 -l 6 -f', [\'FFtn'], [\'n+6b3l6f-FF'], 0],
['9Pbg', '-n.3 +6 -b -3 -l 6 -f', [\'FnFn'], [\'n+6b3l6f-FF'], 0],
['9Pbh', '-n.3 +6 -b -3 -l 6 -f', [\'tFFn'], [\'n+6b3l6f-FF'], 0],
['9Pbi', '-n.3 +6 -b -3 -l 6 -f', [\'tFnFt'], [\'n+6b3l6f-FF'], 0],
['9Pbj', '-n.3 +6 -b -3 -l 6 -f', [\'tFFt'], [\'n+6b3l6f-FF'], 0],
['9Pbk', '-n.3 +5:8 -b -3 -l 10 -f', [\'FnFn'], [\'n+5-8b3l10f-FF'], 0],
['9Pma', '-n.3 +3 -m -l 13 -f', [\'tFFt-bl', \'FnFn'], [\'n+3ml13f-bl-FF'], 0],
['9Pmb', '-n.3 +3 -m -l 17 -f', [\'tFFt-bl', \'tn'], [\'n+3ml17f-bl-tn'], 0],
['9Pmc', '-n.3 +3 -m -l 17 -f', [\'tn', \'tFFt-bl'], [\'n+3ml17f-tn-bl'], 0],
#
# line truncation  column alignment; header line truncation
# -w PAGE_WIDTH [-j] options
['10a', '-w 72 -j', [\'0FnFnt'], [\'w72j-0FF'], 0],
['10b', '-w 48 -l17 -f', [\'tFFt-lm'], [\'w48l17f-lm'], 0],
['10c', '-w 26 -l17 -f', [\'tFFt-lm'], [\'w26l17f-lm'], 0],
['10d', '-w 25 -l17 -f', [\'tFFt-lm'], [\'w25l17f-lm'], 0],
['10e', '-w 20 -l17 -f', [\'tFFt-lm'], [\'w20l17f-lm'], 0],
['10ma', '-m -l 17 -f', [\'tFFt-lm', \'loli'], [\'ml17f-lm-lo'], 0],
['10mb', '-w 35 -m -l 17 -f', [\'tFFt-lm', \'loli'], [\'w35ml17f-lm-lo'], 0],
['10mc', '-j -m -l 17 -f', [\'tFFt-lm', \'loli'], [\'jml17f-lm-lo'], 0],
['10md', '-w 35 -j -m -l 17 -f', [\'tFFt-lm', \'loli'], [\'w35jml17f-lmlo'], 0],
['10me', '-n.3 -j -m -l 17 -f', [\'tFFt-lm', \'tFFt-lm', \'loli'], [\'njml17f-lm-lm-lo'], 0],
['10mf', '-n.3 -j -m -l 17 -f', [\'tFFt-lm', \'loli', \'tFFt-lm'], [\'njml17f-lm-lo-lm'], 0],
['10aa', '-a -3 -l 17 -f', [\'tFFt-lm'], [\'a3l17f-lm'], 0],
['10ab', '-w 35 -a -3 -l 17 -f', [\'tFFt-lm'], [\'w35a3l17f-lm'], 0],
['10ac', '-j -a -3 -l 17 -f', [\'tFFt-lm'], [\'ja3l17f-lm'], 0],
['10ad', '-w 35 -j -a -3 -l 17 -f', [\'tFFt-lm'], [\'w35ja3l17f-lm'], 0],
['10ba', '-b -3 -l 17 -f', [\'tFFt-lm'], [\'b3l17f-lm'], 0],
['10bb', '-w 35 -b -3 -l 17 -f', [\'tFFt-lm'], [\'w35b3l17f-lm'], 0],
['10bc', '-j -b -3 -l 17 -f', [\'tFFt-lm'], [\'jb3l17f-lm'], 0],
['10bd', '-w 35 -j -b -3 -l 17 -f', [\'tFFt-lm'], [\'w35jb3l17f-lm'], 0],
#
# merge files (-m option)  use separator string (-s option)
['11a', '-n.3 -s:--: -m -l 13 -f', [\'tFFt-bl', \'FnFn'], [\'nsml13-bl-FF'], 0],
['11b', '-n.3 -s:--: -m -l 17 -f', [\'tFFt-bl', \'FnFn'], [\'nsml17-bl-FF'], 0],
['11e', '-n.3 -s:--: -m -l 13 -f', [\'tn', \'tn', \'FnFn'], [\'nsml13-t-t-FF'], 0],
['11f', '-n.3 -s:--: -m -l 17 -f', [\'tn', \'tn', \'FnFn'], [\'nsml17-t-t-FF'], 0],
['11g', '-n.3 -s:--: -m -l 13 -f', [\'tn', \'tn', \'FnFn', \'FnFn'], [\'nsml13-t-t-FF-FF'], 0],
['11h', '-n.3 -s:--: -m -l 17 -f', [\'tn', \'tn', \'FnFn', \'FnFn'], [\'nsml17-t-t-FF-FF'], 0],
#
# left margin (-o option) and separator string (-s option)
['12aa', '-o3 -a -3 -l17 -f', [\'tn'], [\'o3a3l17f-tn'], 0],
['12ab', '-o3 -a -3 -s:--: -l17 -f', [\'tn'], [\'o3a3sl17f-tn'], 0],
['12ac', '-o3 -a -3 -s:--: -n. -l17 -f', [\'tn'], [\'o3a3snl17f-tn'], 0],
['12ba', '-o3 -b -3 -l17 -f', [\'tn'], [\'o3b3l17f-tn'], 0],
['12bb', '-o3 -b -3 -s:--: -l17 -f', [\'tn'], [\'o3b3sl17f-tn'], 0],
['12bc', '-o3 -b -3 -s:--: -n. -l17 -f', [\'tn'], [\'o3b3snl17f-tn'], 0],
['12ma', '-o3 -m -l17 -f', [\'tFFt-bl', \'tn'], [\'o3ml17f-bl-tn'], 0],
['12mb', '-o3 -m -s:--: -l17 -f', [\'tFFt-bl', \'tn'], [\'o3msl17f-bl-tn'], 0],
['12mc', '-o3 -m -s:--: -n. -l17 -f', [\'tFFt-bl', \'tn'], [\'o3msnl17f-bl-tn'], 0],
['12md', '-o3 -j -m -l17 -f', [\'tFFt-lm', \'loli'], [\'o3jml17f-lm-lo'], 0],

);
#']]);

sub test_vector
{
  my $common_option_prefix = '--test -h x';

  my @new_tv;
  my $t;
  foreach $t (@tv)
    {
      my ($test_name, $flags, $in, $exp, $ret) = @$t;

      # Prepend the common options to $FLAGS.
      my $sep = ($flags ? ' ' : '');
      $flags = "$common_option_prefix$sep$flags";
      push (@new_tv, [$test_name, $flags, $in, $exp, $ret]);
    }

  return @new_tv;
}

1;
