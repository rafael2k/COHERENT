/*
 * Submit a listing to the local line
 * printer spooler.
 */

#include <stdio.h>
#include <pwd.h>
#include <signal.h>
#ifndef	TYPES_H
#include <sys/types.h>
#endif

/* Schizo is missing so -r doesn't check access privileges */
#define	NBAN	6		/* Maximum number of banners */
#define	NFNAME	400		/* Longest filename allowed */

char	*banners[NBAN];
char	tfspace[20];
char	cfname[20];
char	dfname[20];
char	*tfname;		/* Control file during lpr */
char	*wd;
char	*myuname;
char	*argv0;
FILE	*cfp;			/* Control file stream */
#ifdef LASER
char	spooldir[] = "/usr/spool/hpd";
char	lpd[] = "/usr/lib/hpd";
#else
char	spooldir[] = "/usr/spool/lpd";
char	lpd[] = "/usr/lib/lpd";
#endif
char	tmb[] = "Too many banners, `%s' ignored";

int	Bflag;		/* Suppress banners */	
int	cflag;		/* Generate a copy */
int	mflag;		/* Send notification */
int	rflag;		/* Remove when done */
int	banno = 4;	/* Current banner number */
int	myuid;

long	unique();
char	*absname();
char	*getwd();
char	*ctime();
char	*getlogin();
int	rmexit();

main(argc, argv)
char *argv[];
{
	register char *ap;
	register int i, j;

	argv0 = argv[0];
	signal(SIGINT, rmexit);
	signal(SIGHUP, rmexit);
	for (i=1; i<argc; i++) {
		if (*argv[i] != '-')
			break;
		for (ap = &argv[i][1]; *ap != '\0'; ap++)
			switch (*ap) {
			case 'B':
				++Bflag;
				break;

			case 'b':
				if (++i >= argc)
					lperr("Missing banner");
				if (banno >= NBAN)
					fprintf(stderr, tmb, argv[i]);
				else
					banners[banno++] = argv[i];
				break;

			case 'c':
				cflag = 1;
				break;

			case 'm':
				mflag = 1;
				break;

			case 'n':
				mflag = 0;
				break;

			case 'r':
				cflag = rflag = 1;
				break;

			default:
				usage();
			}
	}
	lprinit(argc, argv);
	if (Bflag)
		fprintf(cfp, "B\n");
	for (j=i; j<argc; j++)
		if (banno < NBAN)
			banners[banno++] = argv[j];
		else
			break;
	for (j=0; j<banno; j++)
		fprintf(cfp, "L%s\n", banners[j]);
	if (i == argc)
		lpr(NULL);
	else
		for (; i<argc; i++)
			lpr(argv[i]);
	lprterm();
	for (i=3; i<_NFILE; i++)
		close(i);
	close(0);
	execl(lpd, lpd, NULL);
	lperr("Cannot find daemon `%s'", lpd);
}

/*
 * Initialise the control file.
 * Set up the banners.
 */
lprinit(ac, av)
char **av;
{
	time_t time(), xtime;
	register int i;
	register struct passwd *pwp;
	register char *s;

	myuid = getuid();
	wd = getwd();
	if (chdir(spooldir) < 0)
		lperr("bad directory `%s'", spooldir);
	tfname = tfspace;
	sprintf(tfname, "tf%ld", unique());
	strcpy(cfname, tfname);
	cfname[0] = 'c';
	if ((cfp = fopen(tfname, "w")) == NULL)
		lperr("cannot create control file");
	putc('D', cfp);
	for (i=0; i<ac; i++)
		fprintf(cfp, "%s%c", av[i], i==ac-1 ? '\n' : ' ');
	banners[0] = "\35\36\37";
	banners[1] = "Coherent";		/* Mark Williams logo */
	xtime = time(NULL);
	s = ctime(&xtime);
	s[16] = '\0';
	banners[2] = s;
	if ((myuname = getlogin()) == NULL)
		if ((pwp = getpwuid(myuid))==NULL)
			myuname = "Guess who?";
	if (myuname == NULL)
		banners[3] = "Guess who?"; else
		banners[3] = myuname;
}

/*
 * Called for each file to put
 * an entry for it in the control
 * file and do any copies to the
 * spool area.
 * The NULL `file' is stdin.
 */
lpr(file)
char *file;
{
	FILE *ifp;
	char *abfile;

	if (file == NULL)
		ifp = stdin;
	else {
		abfile = absname(file);
		if ((ifp = fopen(abfile, "r")) == NULL)
			lperr("cannot open %s", file);
	}
	if (cflag || file==NULL) {
		copy(ifp);
		fprintf(cfp, "A%s\nU%s\n", dfname, dfname);
		dfname[0] = '\0';
	} else
		fprintf(cfp, "A%s\n", abfile);
	if (rflag && file!=NULL) {
		if (unperm(abfile)<0 || unlink(abfile)<0)
			lperr("Cannot unlink %s", file);
	}
	if (file != NULL)
		fclose(ifp);
}

/*
 * Given a file, see if we have permissions to unlink it.
 */
unperm(np)
register char *np;
{
	register char *cp;
	char name[NFNAME];

	cp = name;
	while ((*cp++=*np++) != '\0')
		;
	while (*--cp != '/') {
		if (cp == name) {
			*cp = '.';
			break;
		}
	}
	cp[1] = '\0';
	return (access(name, 2));
}

/*
 * Clean up and close out
 * control file.
 */
lprterm()
{
	if (mflag)
		if (myuname == NULL)
			fprintf(cfp, "M%d\n", myuid); else
			fprintf(cfp, "M%s\n", myuname);
	fflush(cfp);
	if (ferror(cfp))
		lperr("Control file I/O error");
	fclose(cfp);
	link(tfname, cfname);
	unlink(tfname);
	tfname = NULL;
}

/*
 * Copy the data to a data file
 * in the spool directory.
 */
copy(inf)
register FILE *inf;
{
	register int c;
	register FILE *outf;

	sprintf(dfname, "df%ld", unique());
	if ((outf = fopen(dfname, "w")) == NULL)
		lperr("cannot create data file");
	while ((c = getc(inf)) != EOF)
		putc(c, outf);
	fflush(outf);
	if (ferror(outf))
		lperr("data file I/O error");
	fclose(outf);
}

/*
 * Return an absolute pathname
 * for the given filename.
 * (Uses the variable `wd' which
 * contains the process's working
 * directory.)
 */
char *
absname(fn)
char *fn;
{
	static char name[NFNAME];

	if (*fn == '/')
		return (fn);
	sprintf(name, "%s/%s", wd, fn);
	return (name);
}

usage()
{
	fprintf(stderr, "Usage: %s [-Bcmnr] [-b banner] [file ...]\n", argv0);
	rmexit(1);
}

/* VARARGS */
lperr(x)
{
	fprintf(stderr, "%s: %r\n", argv0, &x);
	rmexit(1);
}

/*
 * Exit, removing any files that
 * are left lying around.
 */
rmexit(s)
{
	if (tfname != NULL)
		unlink(tfname);
	if (dfname[0] != '\0')
		unlink(dfname);
	exit(s);
}
