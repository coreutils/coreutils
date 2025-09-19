#!/usr/bin/perl
# Test 'env -S' feature

# Copyright (C) 2018-2025 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

use strict;

(my $program_name = $0) =~ s|.*/||;
my $prog = 'env';

my $env = "$ENV{abs_top_builddir}/src/env";
# Ensure no whitespace or other problematic chars in path
$env =~ m!^([-+\@\w./]+)$!
  or CuSkip::skip "unusual absolute builddir name; skipping this test\n";
$env = $1;

# We may depend on a library found in LD_LIBRARY_PATH, or an equivalent
# environment variable.  Skip the test if it is set since unsetting it may
# prevent us from running commands.
foreach my $var (qw(LD_LIBRARY_PATH LD_32_LIBRARY_PATH DYLD_LIBRARY_PATH
                    LIBPATH))
  {
    if (exists $ENV{$var})
      {
        CuSkip::skip ("programs may depend on $var being set; "
                      . "skipping this test\n");
      }
  }

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

# This envvar is somehow set at least on macOS 11.6, and would
# otherwise cause failure of q*, t* and more tests below.  Ignore it.
my $cf = '__CF_USER_TEXT_ENCODING';
exists $ENV{$cf} and $env .= " -u$cf";
# Likewise for these Cygwin env vars
$cf = 'SYSTEMROOT';
exists $ENV{$cf} and $env .= " -u$cf";
$cf = 'WINDIR';
exists $ENV{$cf} and $env .= " -u$cf";
# Likewise for these GNU/Hurd env vars
$cf = 'LD_ORIGIN_PATH';
exists $ENV{$cf} and $env .= " -u$cf";

my @Tests =
    (
     # Test combination of -S and regular arguments
     ['1', q[-ufoo    A=B FOO=AR  sh -c 'echo $A$FOO'],      {OUT=>"BAR"}],
     ['2', q[-ufoo -S'A=B FOO=AR  sh -c "echo \\$A\\$FOO"'], {OUT=>"BAR"}],
     ['3', q[-ufoo -S'A=B FOO=AR' sh -c 'echo $A$FOO'],      {OUT=>"BAR"}],
     ['4', q[-ufoo -S'A=B' FOO=AR sh -c 'echo $A$FOO'],      {OUT=>"BAR"}],
     ['5', q[-S'-ufoo A=B FOO=AR sh -c "echo \\$A\\$FOO"'],  {OUT=>"BAR"}],

     # Test quoting inside -S
     ['q1', q[-S'-i A="B C" ]."$env'",       {OUT=>"A=B C"}],
     ['q2', q[-S"-i A='B C' ]."$env\"",       {OUT=>"A=B C"}],
     ['q3', q[-S"-i A=\"B C\" ]."$env\"",     {OUT=>"A=B C"}],
     # Test backslash-quoting inside quoting inside -S
     ['q4', q[-S'-i A="B \" C" ]."$env'",    {OUT=>'A=B " C'}],
     ['q5', q[-S"-i A='B \\' C' ]."$env\"",   {OUT=>"A=B ' C"}],
     # Single-quotes in double-quotes and vice-versa
     ['q6', q[-S'-i A="B'"'"'C" ]."$env'",   {OUT=>"A=B'C"}],
     ['q7', q[-S"-i A='B\\"C' ]."$env\"",     {OUT=>'A=B"C'}],

     # Test tab and space (note: tab here is expanded by perl
     # and sent to the shell as ASCII 0x9 inside single-quotes).
     ['t1', qq[-S'-i\tA="B \tC" $env'],    {OUT=>"A=B \tC"}],
     # Here '\\t' is not interpolated by perl/shell, passed as two characters
     # (backslash, 't') to env, resulting in one argument ("A<tab>B").
     ['t2',  qq[-S'printf x%sx\\n A\\tB'],    {OUT=>"xA\tBx"}],
     # Here '\t' is interpolated by perl, passed as literal tab (ASCII 0x9)
     # to env, resulting in two arguments ("A" <whitespace> "B").
     ['t3',  qq[-S'printf x%sx\\n A\tB'],     {OUT=>"xAx\nxBx"}],
     ['t4',  qq[-S'printf x%sx\\n A \t B'],   {OUT=>"xAx\nxBx"}],
     # Ensure \v\f\r\n treated like other whitespace.
     # From 8.30 - 8.32 these would introduce arguments to printf,
     # and also crash ASAN builds with out of bounds access.
     ['t5',  qq[-S'printf x%sx\\n A \t B \013\f\r\n'],   {OUT=>"xAx\nxBx"}],


     # Test empty strings
     ['m1', qq[-i -S""    A=B $env],       {OUT=>"A=B"}],
     ['m2', qq[-i -S"  \t" A=B $env],      {OUT=>"A=B"}],

     # Test escape sequences.
     # note: in the following, there is no interpolation by perl due
     # to q[], and no interpolation by the shell due to single-quotes.
     # env will receive the backslash character followed by t/f/r/n/v.
     # Also: Perl does not recognize "\v", so use "\013" for vertical tab.
     ['e1', q[-i -S'A="B\tC" ]."$env'",    {OUT=>"A=B\tC"}],
     ['e2', q[-i -S'A="B\fC" ]."$env'",    {OUT=>"A=B\fC"}],
     ['e3', q[-i -S'A="B\rC" ]."$env'",    {OUT=>"A=B\rC"}],
     ['e4', q[-i -S'A="B\nC" ]."$env'",    {OUT=>"A=B\nC"}],
     ['e5', q[-i -S'A="B\vC" ]."$env'",    {OUT=>"A=B\013C"}],
     ['e6', q[-i -S'A="B\$C" ]."$env'",    {OUT=>'A=B$C'}],
     ['e7', q[-i -S'A=B\$C ]."$env'",      {OUT=>'A=B$C'}],
     ['e8', q[-i -S'A="B\#C" ]."$env'",    {OUT=>'A=B#C'}],
     ['e9', q[-i -S'A="B\\\\C" ]."$env'",  {OUT=>'A=B\\C'}],
     ['e10',q[-i -S"A='B\\\\\\\\C' ]."$env\"",  {OUT=>'A=B\\C'}],

     # Escape in single-quoted string - passed as-is
     # (the multiple pairs of backslashes are to survive two interpolations:
     #  by perl and then by the shell due to double-quotes).
     ['e11',q[-i -S"A='B\\\\tC' ]."$env\"",    {OUT=>'A=B\tC'}],
     ['e12',q[-i -S"A='B\\\\#C' ]."$env\"",    {OUT=>'A=B\#C'}],
     ['e13',q[-i -S"A='B\\\\\\$C' ]."$env\"",  {OUT=>'A=B\$C'}],
     ['e14',q[-i -S"A='B\\\\\\"C' ]."$env\"",  {OUT=>'A=B\"C'}],

     # Special escape sequences:
     # \_ in double-quotes is a space - result is just one envvar 'A'
     ['e20', q[-i -S'A="B\_C=D" ]."$env'",    {OUT=>'A=B C=D'}],
     # \_ outside double-quotes is arg separator, the command to
     # execute should be 'env env'
     ['e21', q[-i -S'A=B]."\\_$env\\_$env'",    {OUT=>"A=B"}],

     # Test -C inside -S
     ['c1',  q["-S-C/ pwd"], {OUT=>"/"}],
     ['c2',  q["-S -C / pwd"], {OUT=>"/"}],
     ['c3',  q["-S --ch'dir='/ pwd"], {OUT=>"/"}],

     # Test -u inside and outside -S
     # u1,u2 - establish a baseline, without -S
     ['u1',  q[      sh -c 'echo =$FOO='], {ENV=>"FOO=BAR"}, {OUT=>"=BAR="}],
     ['u2',  q[-uFOO sh -c 'echo =$FOO='], {ENV=>"FOO=BAR"}, {OUT=>"=="}],
     # u3,u4: ${FOO} expanded by env itself before executing sh.
     #        \\$FOO expanded by sh.
     # ${FOO} should have value of the original environment
     # and \\$FOO should be unset, regardless where -uFOO is used.
     # 'u3' behavior differs from FreeBSD's but deemed preferable, in
     # https://lists.gnu.org/r/coreutils/2018-04/msg00014.html
     ['u3',  q[-uFOO -S'sh -c "echo x${FOO}x =\\$FOO="'],
      {ENV=>"FOO=BAR"}, {OUT=>"xBARx =="}],
     ['u4',  q[-S'-uFOO sh -c "echo x${FOO}x =\\$FOO="'],
      {ENV=>"FOO=BAR"}, {OUT=>"xBARx =="}],

     # Test ENVVAR expansion
     ['v1', q[-i -S'A=${FOO}     ]."$env'", {ENV=>"FOO=BAR"}, {OUT=>"A=BAR"}],
     ['v2', q[-i -S'A=x${FOO}x   ]."$env'", {ENV=>"FOO=BAR"}, {OUT=>"A=xBARx"}],
     ['v3', q[-i -S'A=x${FOO}x   ]."$env'", {ENV=>"FOO="},    {OUT=>"A=xx"}],
     ['v4', q[-i -S'A=x${FOO}x   ]."$env'",                   {OUT=>"A=xx"}],
     ['v5', q[-i -S'A="x${FOO}x" ]."$env'", {ENV=>"FOO=BAR"}, {OUT=>"A=xBARx"}],
     ['v6', q[-i -S'${FOO}=A     ]."$env'", {ENV=>"FOO=BAR"}, {OUT=>"BAR=A"}],
     # No expansion inside single-quotes
     ['v7', q[-i -S"A='x\${FOO}x' ]."$env\"",              {OUT=>'A=x${FOO}x'}],
     ['v8', q[-i -S'A="${_FOO}" ]."$env'",   {ENV=>"_FOO=BAR"}, {OUT=>"A=BAR"}],
     ['v9', q[-i -S'A="${F_OO}" ]."$env'",   {ENV=>"F_OO=BAR"}, {OUT=>"A=BAR"}],
     ['v10', q[-i -S'A="${FOO1}" ]."$env'",  {ENV=>"FOO1=BAR"}, {OUT=>"A=BAR"}],

     # Test end-of-string '#" and '\c'
     ['d1', q[-i -S'A=B #C=D'    ]."$env",  {OUT=>"A=B"}],
     ['d2', q[-i -S'#A=B C=D'   ]."$env",   {OUT=>""}],
     ['d3', q[-i -S'A=B#'   ]."$env",       {OUT=>"A=B#"}],
     ['d4', q[-i -S'A=B #'   ]."$env",      {OUT=>"A=B"}],

     ['d5', q[-i -S'A=B\cC=D'  ]."$env",    {OUT=>"A=B"}],
     ['d6', q[-i -S'\cA=B C=D' ]."$env",    {OUT=>""}],
     ['d7', q[-i -S'A=B\c'     ]."$env",    {OUT=>"A=B"}],
     ['d8', q[-i -S'A=B \c'    ]."$env",    {OUT=>"A=B"}],

     ['d10', q[-S'echo FOO #BAR'],      {OUT=>"FOO"}],
     ['d11', q[-S'echo FOO \\#BAR'],    {OUT=>"FOO #BAR"}],
     ['d12', q[-S'echo FOO#BAR'],       {OUT=>"FOO#BAR"}],

     # Test underscore as space/separator in double/single/no quotes
     ['s1',  q[-S'printf x%sx\\n "A\\_B"'],   {OUT=>"xA Bx"}],
     ['s2',  q[-S"printf x%sx\\n 'A\\_B'"],   {OUT=>"xA\\_Bx"}],
     ['s3',  q[-S"printf x%sx\\n A\\_B"],     {OUT=>"xAx\nxBx"}],
     ['s4',  q[-S"printf x%sx\\n A B"],       {OUT=>"xAx\nxBx"}],
     ['s5',  q[-S"printf x%sx\\n A  B"],      {OUT=>"xAx\nxBx"}],
     # test underscore/spaces variations -
     # ensure they don't generate empty arguments.
     ['s6',  q[-S"\\_printf x%sx\\n FOO"],          {OUT=>"xFOOx"}],
     ['s7',  q[-S"printf x%sx\\n FOO\\_"],          {OUT=>"xFOOx"}],
     ['s8',  q[-S"\\_printf x%sx\\n FOO\\_"],       {OUT=>"xFOOx"}],
     ['s9',  q[-S"\\_\\_printf x%sx\\n FOO\\_\\_"], {OUT=>"xFOOx"}],
     ['s10', q[-S" printf x%sx\\n FOO"],            {OUT=>"xFOOx"}],
     ['s11', q[-S"printf x%sx\\n FOO "],            {OUT=>"xFOOx"}],
     ['s12', q[-S" printf x%sx\\n FOO "],           {OUT=>"xFOOx"}],
     ['s13', q[-S"  printf x%sx\\n FOO  "],         {OUT=>"xFOOx"}],
     ['s14', q[-S'printf\\_x%sx\\n\\_FOO'],         {OUT=>"xFOOx"}],
     ['s15', q[-S"printf x%sx\\n \\_ FOO"],         {OUT=>"xFOOx"}],
     ['s16', q[-S"printf x%sx\\n\\_ \\_FOO"],       {OUT=>"xFOOx"}],
     ['s17', q[-S"\\_ \\_  printf x%sx\\n FOO \\_ \\_"], {OUT=>"xFOOx"}],

     # Check for empty quotes
     ['eq1',  q[-S'printf x%sx\\n A "" B'], {OUT=>"xAx\nxx\nxBx"}],
     ['eq2',  q[-S'printf x%sx\\n A"" B'],  {OUT=>"xAx\nxBx"}],
     ['eq3',  q[-S'printf x%sx\\n A""B'],   {OUT=>"xABx"}],
     ['eq4',  q[-S'printf x%sx\\n A ""B'],  {OUT=>"xAx\nxBx"}],
     ['eq5',  q[-S'printf x%sx\\n ""'],     {OUT=>"xx"}],
     ['eq6',  q[-S'printf x%sx\\n "" '],    {OUT=>"xx"}],
     ['eq10', q[-S"printf x%sx\\n A '' B"], {OUT=>"xAx\nxx\nxBx"}],
     ['eq11', q[-S"printf x%sx\\n A'' B"],  {OUT=>"xAx\nxBx"}],
     ['eq12', q[-S"printf x%sx\\n A''B"],   {OUT=>"xABx"}],
     ['eq13', q[-S"printf x%sx\\n A ''B"],  {OUT=>"xAx\nxBx"}],
     ['eq14', q[-S'printf x%sx\\n ""'],     {OUT=>"xx"}],
     ['eq15', q[-S'printf x%sx\\n "" '],    {OUT=>"xx"}],

     # extreme example - such as could be found on a #! line.
     ['p10', q[-S"\\_ \\_perl\_-w\_-T\_-e\_'print \"hello\n\";'\\_ \\_"],
      {OUT=>"hello"}],

     # Test Error Conditions
     ['err1', q[-S'"\\c"'], {EXIT=>125},
      {ERR=>"$prog: '\\c' must not appear in double-quoted -S string\n"}],
     ['err2', q[-S'A=B\\'], {EXIT=>125},
      {ERR=>"$prog: invalid backslash at end of string in -S\n"}],
     ['err3', q[-S'"A=B\\"'], {EXIT=>125},
      {ERR=>"$prog: no terminating quote in -S string\n"}],
     ['err4', q[-S"'A=B\\\\'"], {EXIT=>125},
      {ERR=>"$prog: no terminating quote in -S string\n"}],
     ['err5', q[-S'A=B\\q'], {EXIT=>125},
      {ERR=>"$prog: invalid sequence '\\q' in -S\n"}],
     ['err6', q[-S'A=$B'], {EXIT=>125},
      {ERR=>"$prog: only \${VARNAME} expansion is supported, error at: \$B\n"}],
     ['err7', q[-S'A=${B'], {EXIT=>125},
      {ERR=>"$prog: only \${VARNAME} expansion is supported, " .
           "error at: \${B\n"}],
     ['err8', q[-S'A=${B%B}'], {EXIT=>125},
      {ERR=>"$prog: only \${VARNAME} expansion is supported, " .
           "error at: \${B%B}\n"}],
     ['err9', q[-S'A=${9B}'], {EXIT=>125},
      {ERR=>"$prog: only \${VARNAME} expansion is supported, " .
           "error at: \${9B}\n"}],

     # Test incorrect shebang usage (extraneous whitespace).
     ['err_sp2', q['-v -S cat -n'], {EXIT=>125},
      {ERR=>"env: invalid option -- ' '\n" .
            "env: use -[v]S to pass options in shebang lines\n" .
           "Try 'env --help' for more information.\n"}],
     ['err_sp3', q['-v	-S cat -n'], {EXIT=>125}, # embedded tab after -v
      {ERR=>"env: invalid option -- '\t'\n" .
            "env: use -[v]S to pass options in shebang lines\n" .
           "Try 'env --help' for more information.\n"}],

     # Also diagnose incorrect shebang usage when failing to exec.
     # This typically happens with:
     #
     #   $ cat xxx
     #   #!env -v -S cat -n
     #
     #   $ ./xxx
     #
     # in which case:
     #   argv[0] = env
     #   argv[1] = '-v -S cat -n'
     #   argv[2] = './xxx'
     ['err_sp5', q['cat -n' ./xxx], {EXIT=>127},
      {ERR=>"env: 'cat -n': No such file or directory\n" .
            "env: use -[v]S to pass options in shebang lines\n"}],

     ['err_sp6', q['cat -n' ./xxx arg], {EXIT=>127},
      {ERR=>"env: 'cat -n': No such file or directory\n" .
            "env: use -[v]S to pass options in shebang lines\n"}],
    );

# Append a newline to end of each expected 'OUT' string.
my $t;
foreach $t (@Tests)
  {
    my $arg1 = $t->[1];
    my $e;
    foreach $e (@$t)
      {
        $e->{OUT} .= "\n"
            if ref $e eq 'HASH' and exists $e->{OUT} and length($e->{OUT})>0;
      }
  }

# Repeat above tests with "--debug" option (but discard STDERR).
my @new;
foreach my $t (@Tests)
{
    #skip tests that are expected to fail
    next if $t->[0] =~ /^err/;

    my @new_t = @$t;
    my $test_name = shift @new_t;
    my $args = shift @new_t;
    push @new, ["$test_name-debug",
                "--debug " . $args,
                @new_t,
                {ERR_SUBST => 's/.*//ms'}];
}
push @Tests, @new;

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;
