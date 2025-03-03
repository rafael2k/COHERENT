/*
 * dosshrink.c
 * 4/2/90
 *
 * Usage: dosshrink [ -cv ] xdevice part device
 * Options:
 *	-c	Check only, do not write changes
 *	-v	Print verbose description of actions
 * Default device is defined by INFILE in dos1.h.
 *
 * Restructure the given MSDOS filesystem
 * by shifting files forward to first available free space.
 *
 * Uses functions defined in dos1.c:
 *	cc -o dosshrink dosshrink.c dos1.c -f
 * Produces extra debugging output if compiled with DEBUG or FATDUMP defined.
 */

#include "dos1.h"
#include <sys/fdisk.h>
#include <sys/hdioctl.h>

#define	VERSION	"1.3"			/* Version id			*/
#define	USAGE	"Usage:\t/etc/dosshrink [ -cv ] xdevice part device\n"
#define	NBUF	128			/* interactive input buffer size */
#define offsetof(s,id)	((long)(&(((s *)0)->id)))

/* (unsigned int) clusters to (double) megabytes. */
#define	meg(cl)	(((double)(cl))*clsize*ssize/1000000.)

/* Consistency check cluster bitmap. */
#define	CHAR_BIT	8		/* Bits per char		*/
#define	isflag(n)	((bitmap[n/CHAR_BIT] >> (n%CHAR_BIT)) & 1)
#define	setflag(n)	bitmap[n/CHAR_BIT] |= 1 << (n%CHAR_BIT);

/* Conversions. */
#define	sec_to_c(sec)	((unsigned)((sec) / (heads * nspt)))
#define	sec_to_h(sec)	((unsigned)(((sec) / nspt) % heads))
#define	sec_to_s(sec)	((unsigned)(((sec) % nspt) + 1))

/* Clear the IBM AT console screen. */
#define	cls()	putchar(0x1B); putchar('c'); fflush(stdout)

/* Externals. */
extern 	double	atof();
extern	char	*calloc();
extern	char	*malloc();
extern	char	*memcpy();
extern	char	*memset();
extern	char	*realloc();

/* Functions. */
extern	void		checkdir();
extern	void		consistency();
#if	DEBUG
extern	void		dumpname();
#endif
extern	void		info();
extern	MDIR		*readdir();
extern	void		readpart();
extern	void		shuffle();
extern	unsigned int	shuffle1();
extern	MDIR		*writedir();
extern	void		writepart();

/* Globals. */
unsigned char	*bitmap;		/* Consistency check bitmap	*/
unsigned int	maxhidden;		/* Max hidden cluster		*/
unsigned int	nclusters;		/* Clusters written		*/
unsigned int	ndirs;			/* Directories written		*/
unsigned int	removed;		/* Number of clusters removed	*/
unsigned int	scluster;		/* First cluster to shuffle	*/

int
main(argc, argv) int argc; char *argv[];
{
	FDISK_S hd_part;
	int xfd, partno;
	register char c;
	register char *s;

	argv0 = argv[0];
	usagemsg = USAGE;
	while (argc > 1 && argv[1][0] == '-') {
		s = &argv[1][1];
		--argc;
		++argv;
		while ((c = *s++) != '\0') {
			if (c == 'c')
				++cflag;
			else if (c == 'v')
				++vflag;
			else if (c == 'V')
				fprintf(stderr, "%s: V%s\n", argv0, VERSION);
			else
				usage();
		}
	}
	if (argc != 4)
		usage();
	if ((xfd = open(argv[1], (cflag) ? 0 : 2)) < 0)
		fatal("cannot open partition table device \"%s\"", argv[1]);
	if ((partno = atoi(argv[2])) < 0 || partno >= NPARTN)
		fatal("invalid partition number %d\n", partno);
	readpart(xfd, partno, &hd_part);
	if ((fsfd = open(argv[3], (cflag) ? 0 : 2)) < 0)
		fatal("cannot open file system \"%s\"", argv[3]);
	info();
	if (scluster != 0) {
		if ((clbuf = malloc(clsize * ssize)) == NULL)
			fatal("cluster buffer allocation failed");
		shuffle(0, 0);			/* shuffle clusters */
		free(clbuf);
	}
	writepart(xfd, partno, &hd_part);	/* write new partition info */
	if (fatflag)
		writefat();			/* write new FAT */
	if (vflag)
		printf("%s %u directories and moved %u clusters\n",
			(cflag) ? "would have rewritten" : "rewrote",
			ndirs, nclusters);
	if (close(fsfd) == -1)
		fatal("cannot close file system \"%s\"", argv[3]);
	exit(0);
}

/*
 * Read an MSDOS directory and mark used clusters.
 * Read subdirectories recursively.
 */
void
checkdir(cluster, parent) unsigned int cluster, parent;
{
	register MDIR *mdp, *mp;
	MDIR *edp;
	register unsigned int n;
	int hflag;

	dbprintf(("checkdir(%u, %u)\n", cluster, parent));

	/* Scan through the directory. */
	for (mdp = mp = readdir(cluster, &edp); mdp < edp; mdp++) {
		if ((n = mdp->m_name[0]) == MFREE || n == MEMPTY)
			continue;		/* never used or erased */
		else if (n == MMDIR) {		/* "." or "..", hopefully */
			if ((n = mdp->m_name[1]) == MMDIR && mdp->m_cluster == parent)
				continue;
			else if (n == ' ' && mdp->m_cluster == cluster)
				continue;
			else
				fatal("checkdir: \"%c%c\" %u %u %u",
					MMDIR, n, mdp->m_cluster, cluster, parent);
		} else if (mdp->m_size == 0L && !isdir(mdp))	/* 0-size file */
			continue;
#if	DEBUG
		dumpname(mdp);
#endif
		hflag = ishidden(mdp);
		for (n = mdp->m_cluster; n <= CLMAX; n = getcluster(n)) {
			if (n < 2 || n > maxcluster)
				fatal("checkdir: bad cluster number %u", n);
			else if (isflag(n))
				fatal("cross-linked cluster %u", n);
			else
				setflag(n);
			if (hflag && n > maxhidden)
				maxhidden = n;
		}
		if (n <= CLBAD)
			fatal("unexpected 0x%x in file cluster chain", n);
		if (isdir(mdp))
			checkdir(mdp->m_cluster, cluster);
	}
	free(mp);
}

/*
 * Consistency check.
 */
void
consistency()
{
	register unsigned int i, n;

	if ((bitmap = calloc((maxcluster+1+CHAR_BIT-1)/CHAR_BIT, 1)) == NULL)
		fatal("bitmap allocation failed");
	checkdir(0, 0);				/* recursive starting at root */
	for (i = 2; i <= maxcluster; i++) {
		if (isflag(i))
			continue;		/* marked used by checkdir */
		n = fat[i];			/* FAT value */
		if (n == CLFREE || (n > CLMAX && n <= CLBAD))
			continue;		/* free, bad or reserved */
		else
			fatal("lost cluster %u, value %u", i, n);
	}
	free(bitmap);
}

#if	DEBUG
/*
 * Print an MS-DOS filename.
 */
void
dumpname(mdp) register MDIR *mdp;
{
	register char *cp;
	char buf[13];

	cp = lcname(buf, mdp->m_name, 8);
	if (mdp->m_ext[0] != ' ') {
		*cp++ = '.';
		cp = lcname(cp, mdp->m_ext, 3);
	}
	printf("%s:\n", buf);
}
#endif

/*
 * Print information about the MS-DOS file system.
 */
void
info()
{
	static char buf[NBUF];
	register unsigned int n, i;
	unsigned int free, bad, reserved, eof, ordinary;
	unsigned int used, dosused, dosfree, minfree, maxused;

	cls();
	readfat();
	if (vflag)
		printf("heads=%u nspt=%u clsize=%u ssize=%u\n",
			heads, nspt, clsize, ssize);
	free = bad = reserved = eof = ordinary = maxused = 0;
	for (n = 2; n <= maxcluster; n++) {
#if	FATDUMP
		printf("fat[%u]=%u\n", n, getcluster(n));
#endif
		if ((i = getcluster(n)) == 0)
			++free;
		else if (i == CLBAD)
			++bad;
		else if (i > CLMAX && i < CLBAD)
			++reserved;
		else if (i == CLEOF) {
			++eof;
			maxused = n;
		} else {
			++ordinary;
			maxused = n;
		}
	}
	if (vflag) {
		printf("%u clusters: %u free, %u EOF, %u continuation, %u bad, %u reserved\n",
			maxcluster - 1, free, eof, ordinary, bad, reserved);
		printf("maxused=%u\n", maxused);
	}

	/* Make sure the file system is consistent with the FAT. */
	consistency();
	if (vflag)
		printf("maxhidden=%u\n", maxhidden);

	used = ordinary + eof;
	printf("The MS-DOS file system currently contains:\n");
	printf("\t%6.2f\tmegabytes used,\n", meg(used));
	printf("\t%6.2f\tmegabytes free,\n", meg(free));
	if (bad != 0)
		printf("\t%6.2f\tmegabytes in bad clusters,\n", meg(bad));
	if (reserved != 0)
		printf("\t%6.2f\tmegabytes reserved,\n", meg(reserved));
	printf("or\t%6.2f\tmegabytes total.\n", meg(maxcluster-1));
	if (maxhidden > used) {
		minfree = maxhidden - (used);
		printf("This program will not move hidden files, which are sometimes\n");
		printf("used by copy protection schemes.  Because of a hidden file,\n");
		printf("you must leave at least %.2f megabytes free for MS-DOS.\n",
			meg(minfree));
	} else
		minfree = 0;
	printf("\n");
again:
	printf("How many megabytes do you wish leave for use by MS-DOS? ");
	fflush(stdout);
	fgets(buf, NBUF, stdin);
	dosused = (unsigned)(atof(buf) * 1000000. / (clsize * ssize));
	dosfree = dosused - used;
	if (dosused < used || dosfree < minfree || dosfree > free) {
		printf("Please enter a value between %.2f and %.2f.\n",
			meg(used + minfree), meg(used + free));
		goto again;
	} else
		free -= dosfree;
	printf("The modified file system will contain %.2f free megabytes.\n",
		meg(dosfree));
	printf("The unused space after the file system will contain %.2f free megabytes.\n",
		meg(free));
	if (!cflag) {
		printf("Do you want to modify the MS-DOS file system [y]?");
		fgets(buf, NBUF, stdin);
		if (buf[0] != '\0' && buf[0] != '\n' && buf[0] != 'y')
			fatal("aborted");
	}

	/* Find the first cluster which needs to be shuffled. */
	removed = scluster = 0;
	for (n = maxcluster; free > 0 && n >= 2; n--) {
		++removed;
		if ((i = getcluster(n)) == CLFREE)
			--free;
		else if (i <= CLMAX || i == CLEOF) {
			--free;
			scluster = n;
		}
	}
	if (vflag)
		printf("removed=%u\nscluster=%u\n", removed, scluster);
}

/*
 * Read an MSDOS directory.
 * Return a pointer to the allocated directory buffer.
 * Store an end pointer through epp.
 */
MDIR *
readdir(cluster, epp) unsigned int cluster; MDIR **epp;
{
	register MDIR *mdp, *mp, *edp;
	register unsigned int n, c, blocks, files;
	static unsigned int rootfiles;

	if (cluster == 0) {
		if (rootfiles == 0) {
			files = bpb->b_files;
			blocks = dirsize;
		} else {
			files = rootfiles;
			blocks = (files * sizeof(MDIR) + ssize - 1) / ssize;
		}
	} else {
		for (c = cluster, blocks = 1; (c = getcluster(c)) <= CLMAX; ++blocks)
			;
		blocks *= clsize;
		files = blocks * BSIZE / sizeof(MDIR);
	}
	dbprintf(("readdir(cluster=%u) files=%u blocks=%u\n", cluster, files, blocks));
	if ((mp = (MDIR *)malloc(blocks * BSIZE)) == NULL)
		fatal("directory allocation failed");
	mdp = mp;
	if (cluster == 0)
		diskread(mdp, dirbase, blocks, "directory");
	else
		for (n = cluster; n <= CLMAX; n = getcluster(n)) {
			diskread(mdp, cltosec(n), clsize, "subdirectory");
			mdp += mdirsize;
		}

	/* To conserve space, free unused portion of directory. */
	for (mdp = mp; mdp < mp + files; mdp++)
		if ((n = mdp->m_name[0]) != MFREE && n != MEMPTY)
			edp = mdp;
	if (++edp != mp + files && realloc(mp, (char *)edp - (char *)mp) == NULL)
		fatal("realloc failed");
	if (cluster == 0)
		rootfiles = edp - mp;
	*epp = edp;
	return mp;
}

/*
 * Read an entry from the partition table.
 * Look for a valid signature.
 */
void
readpart(xfd, partno, pp) int xfd, partno; FDISK_S *pp;
{
	unsigned short sig;

	if (lseek(xfd, offsetof(HDISK_S, hd_partn[partno]), 0) == -1)
		fatal("partition table seek failed");
	else if (read(xfd, pp, sizeof(FDISK_S)) != sizeof(FDISK_S))
		fatal("partition table read failed");
	else if (lseek(xfd, offsetof(HDISK_S, hd_sig), 0) == -1)
		fatal("partition table seek failed");
	else if (read(xfd, &sig, sizeof sig) != sizeof sig)
		fatal("partition table read failed");
	else if (sig != HDSIG)
		fatal("invalid partition table signature %x", sig);
}

/*
 * Shuffle blocks, rewrite FAT and subdirectory entries
 * to shift all files to front of file system.
 * To keep life simple, this moves as few blocks as possible;
 * each free block at the front is replaced by a used block from the back.
 * Watch out for moving subdirectories (with "." and ".." entries)
 * and for the readonly flag (where nothing really changes on the file system).
 */
void
shuffle(cluster, parent) unsigned int cluster, parent;
{
	register MDIR *mdp, *mp, *edp;
	register unsigned int n, lastn, changed;

	dbprintf(("shuffle(%u, %u)\n", cluster, parent));
	/* Scan through the directory. */
	changed = 0;
	for (mdp = mp = readdir(cluster, &edp); mdp < edp; mdp++) {
		if ((n = mdp->m_name[0]) == MFREE || n == MEMPTY)
			continue;		/* empty or never used */
		else if (n == MMDIR) {		/* "." or ".." */
			/* Watch out for "." and "..", something may have moved. */
			if ((n = mdp->m_name[1]) == MMDIR) {
				if (mdp->m_cluster != parent) {
					mdp->m_cluster = parent;
					++changed;
				}
			} else if (n == ' ') {
				if (mdp->m_cluster != cluster) {
					mdp->m_cluster = cluster;
					++changed;
				}
			} else
				fatal("shuffle: \"%c%c\" botch", MMDIR, n);
			continue;
		} else if (mdp->m_size == 0L && !isdir(mdp))	/* 0-size file */
			continue;
		lastn = 0;
		for (n = mdp->m_cluster; n <= CLMAX; n = getcluster(n)) {
			if (n < 2 || n > maxcluster)
				fatal("shuffle: bad cluster number %u", n);
			if (n >= scluster) {
				n = shuffle1(n);
				if (lastn == 0) {
					mdp->m_cluster = n;
					++changed;
				} else
					putcluster(lastn, n);
			}
			lastn = n;
		}
		if (isdir(mdp))
			shuffle(mdp->m_cluster, cluster);
	}
	if (changed) {
		++ndirs;
		/* Rewrite the directory on the disk. */
		dbprintf(("rewrite disk directory cluster=%u\n", cluster));
		if (!cflag) {
			mdp = mp;
			if (cluster == 0)
				writedir(mdp, edp, dirbase, dirsize);
			else
				for (n = cluster; n <= CLMAX; n = getcluster(n))
					mdp = writedir(mdp, edp, cltosec(n), clsize);
		}
	}
	free(mp);
}

/*
 * Shuffle cluster n to the first free cluster.
 * Return the new cluster number.
 * The directory or FAT entry which points to n gets updated by the caller.
 */
unsigned int
shuffle1(n) unsigned int n;
{
	register unsigned int i;

	++nclusters;
	for (i = 2; i <= maxcluster; i++)
		if (getcluster(i) == CLFREE)
			break;
	if (i > maxcluster)
		fatal("no free blocks in shuffle1");
	dbprintf(("cluster %u -> %u\n", n, i));
	putcluster(i, getcluster(n));		/* next cl to new FAT entry */
	if (cflag)
		return n;			/* return old value */
	putcluster(n, CLFREE);			/* free current FAT entry */
	diskread(clbuf, cltosec(n), clsize, "cluster");
	diskwrite(clbuf, cltosec(i), clsize, "cluster");	/* copy cluster */
	return i;
}

/*
 * Rewrite a directory.
 * The directory entries start at mdp and end at edp.
 * The target is nsecs sectors starting at sector n.
 * Pad with zeros if necessary.
 * Return a pointer past the last byte written from mdp.
 * Use the cluster buffer clbuf to write up to one cluster at a time.
 * This is all necessary only because of the code which realloc's
 * directories, to save space; otherwise, it would be a diskwrite() call.
 */
MDIR *
writedir(mdp, edp, n, nsecs) register MDIR *mdp, *edp; unsigned int n, nsecs;
{
	unsigned int secs, nb, mb, s1, s2;

	diskseek(n);
	while (nsecs != 0) {
		mb = (char *)edp - (char *)mdp;		/* bytes at mdp */
		secs = (nsecs <= clsize) ? nsecs : clsize;	/* sectors */
		nb = secs * ssize;			/* bytes in clbuf */
		s1 = (mb <= nb) ? mb : nb;		/* mdp bytes */
		s2 = nb - s1;				/* zero bytes */
		if (s1 != 0)
			memcpy(clbuf, mdp, s1);
		if (s2 != 0)
			memset(clbuf + s1, 0, s2);
		if (write(fsfd, clbuf, nb) != nb)
			fatal("directory write error");
		nsecs -= secs;
		mdp += s1;
	}
	return mdp;
}

/*
 * Write an updated partition table entry.
 * Also update the sector count in the BPB.
 */
void
writepart(xfd, partno, pp) int xfd, partno; FDISK_S *pp;
{
	unsigned int c, h, s;
	unsigned long size, end;
	int ofatbytes;

	if (cflag)
		return;

	size = pp->p_size;			/* old size */
	size -= clsize * (long) removed;	/* new size */
	end = pp->p_base + size - 1;		/* new end */
	c = sec_to_c(end);
	h = sec_to_h(end);
	s = sec_to_s(end);

	/* Extend paritition to nearest track boundary. */
	if (s != nspt) {
		size += nspt - s;
		end += nspt - s;
		s = nspt;
	}

	/* Update the size and end parameters in the partition table. */
	pp->p_size = size;
	pp->p_ecyl = c & 0xFF;
	pp->p_ehd = h;
	pp->p_esec = ((c >> 2) & CYLMASK) | s;

	/* Write the updated partition table entry. */
	if (lseek(xfd, offsetof(HDISK_S, hd_partn[partno]), 0) == -1)
		fatal("partition table seek failed");
	else if (write(xfd, pp, sizeof(FDISK_S)) != sizeof(FDISK_S))
		fatal("partition table read failed");

	/* Update the BPB sector count. */
	if (lseek(fsfd, (long)BPBOFF + offsetof(BPB, b_sectors), 0) == -1)
		fatal("BPB seek failed");
	s = size;
	if (write(fsfd, &s, sizeof s) != sizeof s)
		fatal("BPB write failed");	

	/*
	 * If the partition shrinks enough,
	 * 2-byte FAT entries become 1.5-byte entries.
	 */
	ofatbytes = fatbytes;
	fatbytes = (size / clsize > (FATMASK & CLMAX)) ? 2 : 1;
	if (ofatbytes != fatbytes)
		++fatflag;		/* rewrite the FAT */
}

/* end of dosshrink.c */
