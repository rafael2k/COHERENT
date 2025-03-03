/*
 * Chmod -- change the mode of
 * files (both symbolic and octal)
 * I really don't like the syntax though.
 */

#include <stdio.h>
#include <sys/stat.h>

/* Masks by types of permissions */
#define	AEXEC	(S_IEXEC|(S_IEXEC>>3)|(S_IEXEC>>6))
#define	AREAD	(S_IREAD|(S_IREAD>>3)|(S_IREAD>>6))
#define	AWRITE	(S_IWRITE|(S_IWRITE>>3)|(S_IWRITE>>6))
#define	ASUID	(S_ISUID|S_ISGID)
#define	ATEXT	S_ISVTX

/* Masks by types of users */
#define	AOWN	(S_ISUID|S_ISVTX|S_IREAD|S_IWRITE|S_IEXEC)
#define	AGRP	(S_ISGID|S_ISVTX|(S_IREAD>>3)|(S_IWRITE>>3)|(S_IEXEC>>3))
#define	AOTH	(S_ISVTX|(S_IREAD>>6)|(S_IWRITE>>6)|(S_IEXEC>>6))
#define	AALL	(AOWN|AGRP|AOTH)

short	int	uid;

char	stickwarn[] = "\
chmod: Warning: non-super user may not set sticky bit\n\
";

main(argc, argv)
char *argv[];
{
	register int i;
	register char *fn;
	int status = 0;

	if (argc < 3)
		usage();
	uid = getuid();
	for (i=2; i<argc; i++) {
		fn = argv[i];
		if (chmod(fn, readmode(argv[1], fn)) < 0) {
			perror(fn);
			status = 2;
		}
	}
	exit (status);
}

/*
 * Read in the symbolic mode and
 * set the variables `who', `op',
 * and `mode'.
 * Knows about the old octal modes as well.
 */
readmode(s, file)
register char *s;
char *file;
{
	register int c;
	register int mode;
	register int op;
	register int m1, m2;
	struct stat sb;

	mode = 0;
	if (*s>='0' && *s<='7') {
		while (*s != '\0') {
			if (*s<'0' || *s>'7')
				omusage();
			mode = (mode<<3) | *s++-'0';
		}
		checkmode(mode);
		return (mode);
	}
	sb.st_mode = 0;
	stat(file, &sb);
	mode = sb.st_mode;
newsym:
	m1 = 0;
	for (;;) {
		switch (*s++) {
		case 'u':
			m1 |= AOWN;
			continue;

		case 'g':
			m1 |= AGRP;
			continue;

		case 'o':
			m1 |= AOTH;
			continue;

		case 'a':
			m1 |= AALL;
			continue;

		default:
			s--;
			break;
		}
		break;
	}
	if (m1 == 0) {
		m1 = AALL&~getumask();
	}
newop:
	if ((c = *s++)=='=' || c=='+' || c=='-')
		op = c;
	else
		smusage();
	m2 = 0;
	for (;;) {
		switch (*s++) {
		case 'r':
			m2 |= AREAD;
			continue;

		case 'w':
			m2 |= AWRITE;
			continue;

		case 'x':
			m2 |= AEXEC;
			continue;

		case 's':
			m2 |= ASUID;
			continue;

		case 't':
			m2 |= ATEXT;
			continue;

		case 'u':
			m2 |= mrepl(mode&AOWN);
			continue;

		case 'g':
			m2 |= mrepl((mode&AGRP)<<3);
			continue;

		case 'o':
			m2 |= mrepl((mode&AOTH)<<6);
			continue;

		default:
			s--;
			break;
		}
		break;
	}
	switch (op) {
	case '-':
		mode &= ~(m1&m2);
		break;

	case '+':
		mode |= m1&m2;
		break;

	case '=':
		mode = (mode&~m1) | (m1&m2);
		break;
	}
	if (*s == '\0') {
		checkmode(mode);
		return (mode);
	}
	if (*s=='+' || *s=='-' || *s=='=')
		goto newop;
	if (*s++ == ',')
		goto newsym;
	smusage();
}

/*
 * Get the value of the umask setting.
 */
getumask()
{
	register int omask;

	omask = umask(0);
	umask(omask);
	return (omask);
}

/*
 * Check the mode to see if any problem
 * bits are on.  For now, this is
 * S_ISVTX for non-super-users.
 */
checkmode(mode)
register int mode;
{
	static int beenhere;

	if (!beenhere && uid!=0 && mode&S_ISVTX) {
		fprintf(stderr, stickwarn);
		beenhere++;
	}
}

/*
 * Replicate the 3-bits of the mode from
 * the owner position to all positions.
 */
mrepl(m)
register int m;
{
	register int m1;

	m1 = m&AOWN;
	m = m1 | (m1>>3) | (m1>>6);
	if (m1 & S_ISUID)
		m |= S_ISGID;
	return (m);
}

usage()
{
	fprintf(stderr, "Usage: chmod mode file ...\n");
	exit(1);
}

smusage()
{
	fprintf(stderr, "chmod: badly formed symbolic mode\n");
	exit(1);
}

omusage()
{
	fprintf(stderr, "chmod: badly formed octal mode\n");
	exit(1);
}



