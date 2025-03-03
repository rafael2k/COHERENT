/* (-lgl
 * 	COHERENT Version 3.0
 * 	Copyright (c) 1982, 1990 by Mark Williams Company.
 * 	All rights reserved. May not be copied without permission.
 -lgl) */
/*
 * This is the format of the header at the start of every archive member.
 * This is not the same as V7.
 * To prevent confusion the magic number is different.
 */

#ifndef	__AR_H__
#define	__AR_H__

/*
 * Be warned! This header use the magic DIRSIZ #define which represents the
 * size of a filename in the old V6 filesystem (which has been kept in the
 * S5 and Coherent filesystems). Thus, the "ar" command and all commands
 * which use this header will die horribly if use with filesystems that
 * support longer names.
 */

#include <sys/types.h>

#if	! DIRSIZ
# define	DIRSIZ		14
#endif

#define OLD_ARMAG	0177535			/* Magic number */

struct	old_ar_hdr {
	char	ar_name[DIRSIZ];	/* Member name */
	time_t	ar_date;		/* Time inserted */
	short	ar_gid;			/* Group id */
	short	ar_uid;			/* User id */
	short	ar_mode;		/* Mode */
	fsize_t	ar_size;		/* File size */
};
/*
 * Name of header module for ranlib
 */
#ifndef	HDRNAME
#define	HDRNAME "__.SYMDEF"
#endif
/*
 * Header module is list of all global defined symbols
 * in all load modules
 */
#ifndef	__L_OUT_H__
#include <n.out.h>
#endif

typedef	struct	ar_sym {
	char	ar_id[NCPLN];		/* symbol name */
	fsize_t	ar_off;			/* offset of load module */
} ar_sym;				/* ...from end of header module */

#endif	/* ! defined (__AR_H__) */
