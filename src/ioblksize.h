/* I/O block size definitions for coreutils
   Copyright (C) 1989-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Include this file _after_ system headers if possible.  */

/* sys/stat.h will already have been included by system.h. */
#include "stat-size.h"


/* As of Jul 2011, 64KiB is determined to be the minimium
   blksize to best minimize system call overhead.
   This can be tested with this script:

   for i in $(seq 0 10); do
     bs=$((1024*2**$i))
     printf "%7s=" $bs
     timeout --foreground -sINT 2 \
       dd bs=$bs if=/dev/zero of=/dev/null 2>&1 \
         | sed -n 's/.* \([0-9.]* [GM]B\/s\)/\1/p'
   done

   With the results shown for these systems:
   system-1 = 1.7GHz pentium-m with 400MHz DDR2 RAM, arch=i686
   system-2 = 2.1GHz i3-2310M with 1333MHz DDR3 RAM, arch=x86_64
   system-3 = 3.2GHz i7-970 with 1333MHz DDR3, arch=x86_64

   blksize  system-1   system-2   system-3
   ---------------------------------------
      1024  734 MB/s   1.7 GB/s   2.6 GB/s
      2048  1.3 GB/s   3.0 GB/s   4.4 GB/s
      4096  2.4 GB/s   5.1 GB/s   6.5 GB/s
      8192  3.5 GB/s   7.3 GB/s   8.5 GB/s
     16384  3.9 GB/s   9.4 GB/s  10.1 GB/s
     32768  5.2 GB/s   9.9 GB/s  11.1 GB/s
     65536  5.3 GB/s  11.2 GB/s  12.0 GB/s
    131072  5.5 GB/s  11.8 GB/s  12.3 GB/s
    262144  5.7 GB/s  11.6 GB/s  12.5 GB/s
    524288  5.7 GB/s  11.4 GB/s  12.5 GB/s
   1048576  5.8 GB/s  11.4 GB/s  12.6 GB/s

   Note that this is to minimize system call overhead.
   Other values may be appropriate to minimize file system
   or disk overhead.  For example on my current GNU/Linux system
   the readahead setting is 128KiB which was read using:

   file="."
   device=$(df -P --local "$file" | tail -n1 | cut -d' ' -f1)
   echo $(( $(blockdev --getra $device) * 512 ))

   However there isn't a portable way to get the above.
   In the future we could use the above method if available
   and default to io_blksize() if not.
 */
enum { IO_BUFSIZE = 64*1024 };
static inline size_t
io_blksize (struct stat sb)
{
  return MAX (IO_BUFSIZE, ST_BLKSIZE (sb));
}
