/*********************************************************************
*                         COPYRIGHT NOTICE                           *
**********************************************************************
*        This software is copyright (C) 1982 by Pavel Curtis         *
*                                                                    *
*        Permission is granted to reproduce and distribute           *
*        this file by any means so long as no fee is charged         *
*        above a nominal handling fee and so long as this            *
*        notice is always included in the copies.                    *
*                                                                    *
*        Other rights are reserved except as explicitly granted      *
*        by written permission of the author.                        *
*                Pavel Curtis                                        *
*                Computer Science Dept.                              *
*                405 Upson Hall                                      *
*                Cornell University                                  *
*                Ithaca, NY 14853                                    *
*                                                                    *
*                Ph- (607) 256-4934                                  *
*                                                                    *
*                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
*                decvax!cornell!pavel       (UUCPnet)                *
*********************************************************************/

/*
 *	compiler.h - Global variables and structures for the terminfo
 *			compiler.
 *
 *  $Header: /src386/usr/bin/tic/RCS/compiler.h,v 1.1 92/03/13 10:45:54 bin Exp $
 *
 *  $Log:	compiler.h,v $
 * Revision 1.1  92/03/13  10:45:54  bin
 * Initial revision
 * 
Revision 2.1  82/10/25  14:46:04  pavel
Added Copyright Notice

Revision 2.0  82/10/24  15:17:20  pavel
Beta-one Test Release

Revision 1.3  82/08/23  22:30:09  pavel
The REAL Alpha-one Release Version

Revision 1.2  82/08/19  19:10:10  pavel
Alpha Test Release One

Revision 1.1  82/08/12  18:38:11  pavel
Initial revision

 *
 */

#include <stdio.h>

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define SINGLE			/* only one terminal (actually none) */

extern char *destination;	/* destination directory for object files */

long	start_time;		/* time at start of compilation */
long	time();

int	curr_line;		/* current line # in input */
long	curr_file_pos;		/* file offset of current line */

int	debug_level;		/* level of debugging output */

#define DEBUG(level, fmt, a1) \
		if (debug_level >= level)\
		    fprintf(stderr, fmt, a1);

	/*
	 *	These are the types of tokens returned by the scanner.
	 *	The first three are also used in the hash table of capability
	 *	names.  The scanner returns one of these values after loading
	 *	the specifics into the global structure curr_token.
	 *
	 */

#define BOOLEAN 0		/* Boolean capability */
#define NUMBER 1		/* Numeric capability */
#define STRING 2		/* String-valued capability */
#define CANCEL 3		/* Capability to be cancelled in following tc's */
#define NAMES  4		/* The names for a terminal type */

	/*
	 *	The global structure in which the specific parts of a
	 *	scanned token are returned.
	 *
	 */

struct token
{
	char	*tk_name;		/* name of capability */
	int	tk_valnumber;	/* value of capability (if a number) */
	char	*tk_valstring;	/* value of capability (if a string) */
};

struct token	curr_token;

	/*
	 *	The file comp_captab.c contains an array of these structures,
	 *	one per possible capability.  These are then made into a hash
	 *	table array of the same structures for use by the parser.
	 *
	 */

struct name_table_entry
{
	struct name_table_entry *nte_link;
	char	*nte_name;	/* name to hash on */
	int	nte_type;	/* BOOLEAN, NUMBER or STRING */
	short	nte_index;	/* index of associated variable in its array */
};

extern struct name_table_entry	cap_table[];
extern struct name_table_entry	*cap_hash_table[];

extern int	Captabsize;
extern int	Hashtabsize;

#define NOTFOUND	((struct name_table_entry *) 0)
	/*
	 *	Function types
	 *
	 */

struct name_table_entry	*find_entry();	/* look up entry in hash table */

char	next_char();
char	trans_string();
