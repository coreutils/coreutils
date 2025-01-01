'\" Copyright (C) 2018-2025 Free Software Foundation, Inc.
'\"
'\" This is free software.  You may redistribute copies of it under the terms
'\" of the GNU General Public License <https://www.gnu.org/licenses/gpl.html>.
'\" There is NO WARRANTY, to the extent permitted by law.
[NAME]
basenc \- Encode/decode data and print to standard output
[DESCRIPTION]
.\" Add any additional description here
[ENCODING EXAMPLES]
.PP
.nf
.RS
$ printf '\\376\\117\\202' | basenc \-\-base64
/k+C

$ printf '\\376\\117\\202' | basenc \-\-base64url
_k-C

$ printf '\\376\\117\\202' | basenc \-\-base32
7ZHYE===

$ printf '\\376\\117\\202' | basenc \-\-base32hex
VP7O4===

$ printf '\\376\\117\\202' | basenc \-\-base16
FE4F82

$ printf '\\376\\117\\202' | basenc \-\-base2lsbf
011111111111001001000001

$ printf '\\376\\117\\202' | basenc \-\-base2msbf
111111100100111110000010

$ printf '\\376\\117\\202\\000' | basenc \-\-z85
@.FaC
.RE
.fi
