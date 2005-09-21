/* Define the magic numbers as given by statfs(2).
   Please send additions to bug-coreutils@gnu.org and meskes@debian.org.
   This file is generated automatically from ./stat.c. */

#if defined __linux__
# define S_MAGIC_AFFS 0xADFF
# define S_MAGIC_DEVPTS 0x1CD1
# define S_MAGIC_EXT 0x137D
# define S_MAGIC_EXT2_OLD 0xEF51
# define S_MAGIC_EXT2 0xEF53
# define S_MAGIC_JFS 0x3153464a
# define S_MAGIC_XFS 0x58465342
# define S_MAGIC_HPFS 0xF995E849
# define S_MAGIC_ISOFS 0x9660
# define S_MAGIC_ISOFS_WIN 0x4000
# define S_MAGIC_ISOFS_R_WIN 0x4004
# define S_MAGIC_MINIX 0x137F
# define S_MAGIC_MINIX_30 0x138F
# define S_MAGIC_MINIX_V2 0x2468
# define S_MAGIC_MINIX_V2_30 0x2478
# define S_MAGIC_MSDOS 0x4d44
# define S_MAGIC_FAT 0x4006
# define S_MAGIC_NCP 0x564c
# define S_MAGIC_NFS 0x6969
# define S_MAGIC_PROC 0x9fa0
# define S_MAGIC_SMB 0x517B
# define S_MAGIC_XENIX 0x012FF7B4
# define S_MAGIC_SYSV4 0x012FF7B5
# define S_MAGIC_SYSV2 0x012FF7B6
# define S_MAGIC_COH 0x012FF7B7
# define S_MAGIC_UFS 0x00011954
# define S_MAGIC_XIAFS 0x012FD16D
# define S_MAGIC_NTFS 0x5346544e
# define S_MAGIC_TMPFS 0x1021994
# define S_MAGIC_REISERFS 0x52654973
# define S_MAGIC_CRAMFS 0x28cd3d45
# define S_MAGIC_ROMFS 0x7275
# define S_MAGIC_RAMFS 0x858458f6
# define S_MAGIC_SQUASHFS 0x73717368
# define S_MAGIC_SYSFS 0x62656572
#elif defined __GNU__
# include <hurd/hurd_types.h>
#endif
