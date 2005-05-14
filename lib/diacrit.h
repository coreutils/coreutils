/* Diacritics processing for a few character codes.
   Copyright (C) 1990, 1991, 1992, 1993 Free Software Foundation, Inc.
   François Pinard <pinard@iro.umontreal.ca>, 1988.

   All this file is a temporary hack, waiting for locales in GNU.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

extern const char diacrit_base[]; /* characters without diacritics */
extern const char diacrit_diac[]; /* diacritic code for each character */

/* Returns CHAR without its diacritic.  CHAR is known to be alphabetic.  */
#define tobase(Char) (diacrit_base[(unsigned char) (Char)])

/* Returns a diacritic code for CHAR.  CHAR is known to be alphabetic.  */
#define todiac(Char) (diacrit_diac[(unsigned char) (Char)])
