#!/bin/sh
#
# Copyright (C) 2019-2024 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
#
# Written by Assaf Gordon and Bernhard Voelker.


# To build all versions since 5.0 (if possible):
# git tag \
#   | grep '^v[5678]' \
#   | sed 's/^v//' \
#   | sort -V \
#   | while read f; do \
#       ./build-older-versions.sh $f 2>&1 \
#          | tee build/build.$f.log ; \
#     done

PREFIX="${PREFIX:=$HOME/old-coreutils}"

base=$(basename "$0")

die()
{
  echo "$base: error: $*" >&2
  exit 1
}

warn()
{
  echo "$base: warning: $*" >&2
}

get_patch_file()
{
  case "$1" in
    5.0)  echo coreutils-5.0-on-glibc-2.28.diff ;;
    5.97|6.[345679]) echo coreutils-5.97-on-glibc-2.28.diff ;;
    6.10) echo coreutils-6.10-on-glibc-2.28.diff ;;
    6.11) echo coreutils-6.11-on-glibc-2.28.diff ;;
    6.12) echo coreutils-6.12-on-glibc-2.28.diff ;;
    7.[23456]|8.[123]) echo coreutils-7.2-on-glibc-2.28.diff ;;
    8.[456789]|8.1[012]) echo coreutils-8.4-on-glibc-2.28.diff ;;
    8.1[3456]) echo coreutils-8.13-on-glibc-2.28.diff ;;
    8.17) echo coreutils-8.17-on-glibc-2.28.diff ;;
    8.1[89]|8.2[0123]) echo coreutils-8.18-on-glibc-2.28.diff ;;
    8.2[456789]) echo coreutils-8.24-on-glibc-2.28.diff ;;
    8.[3456789]*) warn "patch not needed for version '$1'" ;;
    5.[12]*|5.9*)  die "version '$1' does not have a patch (yet) " \
                       "use versions 5.0 or 5.97" ;;
    7.1)  die "version '$1' does not have a patch (yet)" \
                       "use versions 6.12 or 7.2" ;;
    5*|6*|7*|8*) die "non-existing version" ;;
    *) die "unknown version" ;;
  esac
}

get_url()
{
  _base_url="https://ftp.gnu.org/gnu/coreutils/coreutils-$1.tar"
  case "$1" in
    5.*|6.*|7.*) echo "$_base_url.gz" ;;
    8.*)         echo "$_base_url.xz" ;;
    *) die "unknown version" ;;
  esac
}

##
## Setup
##
test -n "$1" \
  || die "missing coreutils version to build (e.g. '6.12')"

cd $(dirname "$0")

patch_file=$(get_patch_file "$1") \
  || die "cannot build version '$1'"

# Test for the patch file if the above returned one.
if test "$patch_file"; then
  test -e "$patch_file" \
    || die "internal error: patch file '$patch_file' does not exist"
fi

url=$(get_url "$1")
tarball=$(basename "$url")

mkdir -p "build" \
  && cd "build" \
  || die "creating version build dir 'build' failed"

##
## Download tarball (if needed)
##
if ! test -e "$tarball" ; then
   wget -O "$tarball.t" "$url" \
        && mv "$tarball.t" "$tarball" \
        || die "failed to download '$url'"
fi

##
## Extract tarball (if needed)
##
srcdir=${tarball%.tar.*}
if ! test -d "$srcdir" ; then
    tar -xvf "$tarball" || die "failed to extract '$tarball'"
fi

##
## Patch (if needed)
##
cd "$srcdir" \
  || die "changing directory to '$srcdir' failed"

# Patch will fail if it was already applied (using "--forward" turns
# that into a no-op). So don't check for failure.
# Is there a way to differentiate between 'already applied' and
# 'failed to apply' ?
test "$patch_file" \
  && patch --ignore-whitespace --batch --forward -p1 < "../../$patch_file"

##
## Configure
##
version="${srcdir#coreutils}" # note:  this keeps the '-' in '$version'
vprefix="$PREFIX/coreutils$version"
if ! test -e "Makefile" ; then
  ./configure \
    --program-suffix="$version" \
    --prefix="$vprefix" \
    || die "failed to run configure in 'build/$srcdir/'"
fi

##
## Build
##
make -j4 \
  || die "build failed in 'build/$srcdir'"

##
## Install
##
make install \
  || die "make-install failed in 'build/$srcdir' (to '$vprefix')"


# Create convenience links for the executables and manpages in common directory.
(
  mkdir -p "$PREFIX/bin" "$PREFIX/man/man1" \
    || die "creating common bin or man directory failed"
  cd $vprefix/bin \
    || die "changing directory to just-installed 'bin' directory failed"
  for f in *; do
    ln -snvf "../coreutils$version/bin/$f" "$PREFIX/bin/$f" \
      || die "creating symlink of executable '$f' failed"
  done

  share=  # older versions do not have 'share'.
  cd "$vprefix/share/man/man1" 2>/dev/null \
    && share='/share' \
    || cd "$vprefix/man/man1" \
    || die "changing directory to just-installed 'man/man1' directory failed"
  for f in *; do
    ln -snfv "../../coreutils$version$share/man/man1/$f" "$PREFIX/man/man1/$f" \
      || die "creating symlink of man page '$f' failed"
  done
) || exit 1

# Build and install PDF (if possible).
if make SUBDIRS=. pdf; then
  make SUBDIRS=. install-pdf \
    || die "make-install-pdf failed in 'build/$srcdir' (to '$vprefix')"
else
  echo "$0: no PDF available"
fi

# Print summary
cat<<EOF


=================================================================

GNU Coreutils$version successfully installed.

Source code in $PWD/build/$srcdir
Installed in $vprefix

symlinks for all programs (all versions) in $PREFIX/bin
manual pages for all programs in $PREFIX/share/man/man1

Run the following command to add all programs to your \$PATH:

    export PATH=\$PATH:\$HOME/old-coreutils/bin
    export MANPATH=\$MANPATH:\$HOME/old-coreutils/man

EOF
