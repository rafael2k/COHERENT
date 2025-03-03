/*
 *	Perform initialization for phases of fsck
 */
#include "fsck.h"
#include <mnttab.h>
#include <mtab.h>

int	fsfd;				/* file system file descriptor */
int	writeflg;			/* write ok or read-only flag */
daddr_t	fsize = SUPERI+1;		/* file system size in blocks  */
					/* Allows for read of Super Block */
int	isize;				/* First block not in inode list */
daddr_t	totfree;			/* Running total free block count */
unsigned	ninodes;		/* number of inodes */
int	numfiles;			/* number of files in file system */

char	superb[BSIZE];			/* Super Block */
struct filsys *sbp = superb;
int	sbpfix;				/* flag for super block fixes */

int	offsets[] = 			/* table of offsets for the levels */
		{ ND, ND+NI, ND+NI+NII, /* of indirection of blocks in the */
		  ND+NI+NII+NIII };     /* inode block list */

daddr_t	dupblck[DUPTBLSIZE];		/* duplicate referenced blocks */
int	totdups;			/* number of dup blocks so far */

#if !SMALLMODEL
unsigned short *linkPtr;
unsigned char  *flagptr, *blockPtr, *dupPtr;
#endif

init()
{
	int mode;
	struct stat status;
	unsigned short stmode;

	mode = (mdaction == NO) ? 0 : 2;

	if ( (fsfd = open(fsname, mode)) == (-1) ) {
		mode = 0;
		if ( (fsfd = open(fsname, mode)) == (-1) ) {
			nonfatal("Can't open: %s", fsname);
			return;
		}
	}

	if (mode == 0) {
		daction  = NO;
		writeflg = FALSE;
	} else {
		daction  = mdaction;
		writeflg = TRUE;
	}
	
	printf("%s: %s\n", fsname, (mode == 0) ? "(NO WRITE)" : "");

	if ( fstat(fsfd, &status) == (-1) ) {
		nonfatal("Can't stat: %s", fsname);
		return;
	}

	stmode = status.st_mode & S_IFMT;
	if ( (stmode != S_IFBLK) && (stmode != S_IFCHR) ) 
		switch(query("file is not a block or character device; OK?")){
		case YES:
			break;
		case NO:
			errflag = TRUE;
			return;
		}

	domount();
	sbpfix = FALSE;
	readsuper();
	checksuper();
	if ( !errflag )
		alloctables();
}

/*
 *	Read in the super block and canonicalize
 */

readsuper()
{
	register int i;

	bread((daddr_t)SUPERI, superb);
	sbp = superb;

	canshort(sbp->s_isize);
	candaddr(sbp->s_fsize);
	canshort(sbp->s_nfree);
	for(i=0; i<NICFREE; i++)
		candaddr(sbp->s_free[i]);
	canshort(sbp->s_ninode);
	for(i=0; i<NICINOD; i++)
		canino(sbp->s_inode[i]);
	cantime(sbp->s_time);
	candaddr(sbp->s_tfree);
	canino(sbp->s_tinode);
	canshort(sbp->s_m);
	canshort(sbp->s_n);
	canlong(sbp->s_unique);

}

/*
 *	Check super block for obvious inconsistencies
 */

checksuper()
{
	fsize = sbp->s_fsize;
	isize = sbp->s_isize;
	totfree = fsize - isize;
	
	if ( (isize<=INODEI) || (isize>=fsize) || (fsize<=INODEI) ) {
 		badsuper("Size check: fsize %U isize %u", fsize, isize);
		errflag = TRUE;
	}

	if ( sbp->s_nfree > NICFREE )
		badsuper("Too large free block count");

	if ( sbp->s_ninode > NICINOD )
		badsuper("Too large free i-node count");
}

/*
 *	Super Block Error Functions
 */
badsuper(x)
{
	printf("%r\n", &x);
	printf("fsck: %s: Bad Super Block: %u\n", fsname, SUPERI);
}

/*
 *	Allocate the necessary buffers for tables
 */
alloctables()
{
	unsigned num, numl;

	numl = 0;
	if (!fflag) {
		ninodes = (unsigned) (isize - INODEI) * INOPB;
		numl = (unsigned) ninodes;
	}
	
	num = (unsigned) ((fsize+NBPC-1)/NBPC) * sizeof(char);
#if SMALLMODEL
	initV(numl, numl, num, num); /* virtual allocation */
#else
	if (NULL != linkPtr) {
		free(linkPtr);
		free(flagPtr);
		free(blockPtr);
		free(dupPtr);
	}
	linkPtr  = calloc(numl, sizeof(unsigned short));
	flagPtr  = calloc(numl, sizeof(unsigned char));
	blockPtr = calloc(num,  sizeof(unsigned char));
	dupPtr   = calloc(num,  sizeof(unsigned char));
#endif
	numfiles =		/* number of files in the file system */
	totdups = 0;		/* num dup blocks */
}

/*
 *	domount() checks the files /etc/mnttab and /etc/mtab to find
 * 	out where the file system is mounted (if at all).  It then 
 *	reports where (and when if possible) it was last mounted.
 */
char *mnttab_name = "/etc/mnttab";
char *mtab_name	  = "/etc/mtab";
char *bttime_name = "/etc/boottime";

domount()
{
	int fd;
	struct mnttab mnttab;
	struct mtab mtab;
	register char *name;
	struct stat sbuf;
	time_t boottime;
	int nothing();
	char devname[MNTNSIZ+6] = "/dev/";

	mounted = FALSE;
	name = fsname;				/* strip off all initial */
	while (*name++ != '\0') ;		/* directories in fsname */
	while ( (*--name != '/') && (name>fsname) ) ;
	if (*name == '/') name++;

	if ( stat(bttime_name, &sbuf) == -1 )
		boottime = (time_t)0;
	else
		boottime = sbuf.st_mtime;
				
	if ( (fd=open(mnttab_name, 0)) != (-1) ) {
		while ( read(fd, &mnttab, sizeof(mnttab))==sizeof(mnttab) ) 
			if ( mnttab.mt_dev[0] && mnttab.mt_filsys[0] ) {
				strncpy(&devname[5], mnttab.mt_filsys, MNTNSIZ);
				devname[MNTNSIZ+5] = '\0';
				statit(devname, nothing);
				if (stats.st_rdev != fsysrdev)
					continue;
				statit(mnttab.mt_dev, nothing);
				mounted = (stats.st_dev == fsysrdev);
				prmnttab(&mnttab, boottime);
				return;
			}
	}
	if ( (fd=open(mtab_name, 0)) != (-1) ) {
		while ( read(fd, &mtab, sizeof(mtab))==sizeof(mtab) ) 
			if ( mtab.mt_name[0] && mtab.mt_special[0] ) {
				strncpy(&devname[5], mtab.mt_special, MNAMSIZ);
				devname[MNAMSIZ+5] = '\0';
				statit(devname, nothing);
				if (stats.st_rdev != fsysrdev)
					continue;
				statit(mtab.mt_name, nothing);
				mounted = (stats.st_dev == fsysrdev);
				prmtab(&mtab);
				return;
			}
	}

	if (fsysrdev == rootdev)
		mounted = TRUE;
	else
		mounted = FALSE;

}

prmnttab(ptr, boottime)
struct mnttab *ptr;
time_t boottime;
{
	if (mounted) {
		if (boottime < ptr->mt_time)
			boottime = ptr->mt_time;
		printf("%s mounted on %.*s as of %s", fsname, MNTNSIZ, 
					ptr->mt_dev, ctime(&boottime));
	} else
		printf("%s unmounted.  Last mounted on %.*s -- %s",
						fsname, MNTNSIZ, ptr->mt_dev,
							ctime(&ptr->mt_time));
}


prmtab(ptr)
struct mtab *ptr;
{
	if (mounted)
		printf("%s mounted on %.*s\n", fsname, MNAMSIZ, ptr->mt_name);
	else
		printf("%s unmounted.  Last mounted on %.*s\n", fsname,
							MNAMSIZ, ptr->mt_name);
}

nothing()
{}
