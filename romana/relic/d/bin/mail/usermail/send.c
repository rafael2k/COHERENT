/*
 *	The code that passes an individual message to the mail system.
 */

#include "mail.h"

#define SITENAMELEN	32	/* max length of sitename */
#define NODENAME	"/etc/uucpname"
#define DOMAINNAME	"/etc/domain"
char	domain [64];

extern	char	*strtok();
static	char	**allist;
static	char	**tolist;
static	char	**cclist = NULL;

char	mysite[SITENAMELEN];	/* this host's uucpname */

extern	char	*temp;
extern	int	myuid;		/* User-id of mail user */
extern	int	mygid;		/* Group-id of mail user */
extern	char	cmdname[];	/* Command for x{en,de}code filter */
				/* and for tail recursion to uumail */
				/* and for editor recursion */
extern	char	*editname;	/* name of editor	   */
extern	char	*askcc;		/* Ask for CC: list? (YES/NO) */
extern	int	callmexmail;	/* Xmail modifier present */
extern	char	wrerr[];
extern	char	myname[];	/* User name */
extern	char	myfullname[];	/* full user name */
extern	int	mflag;		/* `You have mail.' message to recipient */
extern	int	verbflag;	/* verbose flag */
extern	char	templ[];	/* Temp file name template */
extern	char	mydead[];	/* $HOME/dead.letter */
extern	char	mysig[];
extern	char	nosave[];
extern	int	callmermail;

static	int	eflag;		/* Edit this mail */
static	int	senderr;
struct	tm	*tp;
char	toerr[] = "Cannot create temporary file\n";
char	header[BUFSIZ];		/* Message header */
char	boxname[256];		/* Destination mailbox */
char	remotefrom [32];	/* "remote from uucp" */

char	nosend[] = "Can't send mail to '%s'\n";
char	nopubk[] = "Can't send xmail to '%s'\n";
static	int fromtty;

/*
 * Send the message found on
 * the file pointer to the list
 * of people (argv style) with
 * a NULL pointer at the end.
 * The message is copied to a temp-file
 * from position `start' to `end' (or EOF).
 */
send2(fp, users, start, end, asksubj)
FILE *fp;
register char **users;
fsize_t start, end;
int	asksubj;
{
	char	**getcc(), **listpp;
	FILE 	*xfp, *tfp, *sigfp;
	time_t	curtime;

	uucpname();
	domainname();
	senderr = 0;
	temp = templ;

	fromtty = isatty(fileno(stdin));
	if ((tfp = fopen(temp, "w")) != NULL) {
		fclose(tfp);
		if ((tfp = fopen(temp, "r+w")) == NULL)
			merr(toerr);
	} else
		merr(toerr);
	chown(temp, myuid, mygid);
	unlink(temp);
	temp = NULL;
	fseek(fp, start, 0);
	end -= start;

	eflag = 0;
	if (fromtty && asksubj && !callmermail) {
		fprintf(stdout, "Subject: ");
		fflush(stdout);
		if (fgets(msgline, NLINE, fp) != NULL) {
			if (strlen(msgline) > 1)
				fprintf(tfp, "Subject: %s", msgline);
		}
	}

	if (intcheck()) {
		fclose(tfp);
		return(1);
	}

	for (;;) {
		if (fgets(msgline, NLINE, fp) == NULL)
			break;
		if (fp == stdin)
			if ((strcmp(".\n", msgline)==0))
				break;
			else if ((strcmp("?\n", msgline)==0)) {
				eflag = 1;
				break;
			}
		fputs(msgline, tfp);
		if ( (end-=strlen(msgline)) <= 0 )
			break;
	}

	if (intcheck()) {
		fclose(tfp);
		return(1);
	}

	if (!callmermail && (sigfp = fopen(mysig, "r")) != NULL) {
		fputs("\n", tfp);
		while (fgets(msgline, NLINE, sigfp) != NULL)
			fputs(msgline, tfp);
		fclose(sigfp);
	}

	/*
	 * Now, see if user wants to edit the message
	 */

	if (eflag) {
		xfp = tfp;
		temp = templ;
		if ((tfp = fopen(temp, "wr")) == NULL)
			merr(toerr);
		chown(temp, myuid, mygid);
		mcopy(xfp, tfp, (fsize_t)0, (fsize_t)MAXLONG, 0);
		fclose(xfp);
		sprintf(cmdname, "exec %s %s", editname, templ);
		system(cmdname);
		unlink(temp);
		temp = NULL;
	}

	/*
	 * if empty message, bug out.
	 */

	if (ftell(tfp) == 0) {
		fclose(tfp);
		return(1);
	}

	/*
	 * Now see if a copy list is requested.
	 */

	tolist = users;
	cclist = (askcc && fromtty && !callmermail) ? getcc(): NULL;
	allist = listcat(tolist, cclist);
	
	if ( verbflag ) {
		mmsg(
	"Recipient List:\n\n");
		for (listpp=allist; *listpp != NULL; listpp++)
			mmsg("\t%s\n", *listpp);
	}

	/*
	 * Now send the message.
	 */

	time(&curtime);
	tp = localtime(&curtime);

	if (callmexmail)
		xsend(allist, tfp);
	else
		usend(allist, tfp);

	return( senderr );
}

usend(users, tfp)
char **users;
FILE *tfp;
{
	FILE *xfp;
	char *name;
	char **ulist, **listpp;
	char command[CMDLINE];
	char *arpadate();
	FILE *popen();

	logdump("Date: %s  From: %s!%s (%s)\n",
			arpadate(tp), mysite, myname, myfullname);
	for (ulist = users; (name=*ulist) != NULL; ulist++)
		logdump("To: \"%s\"\n", name);

	/* Build the command line to the delivery program.  */
	if ( callmexmail ) {
		strcpy(command, XDELIVER); /* usually "/bin/rxmail" */
	} else {
		strcpy(command, DELIVER); /* usually "/bin/rmail" */
	} /* if ( callmexmail ) */

	/* Add the list of recipients to the command line.  */
	for (listpp = users; *listpp != NULL; listpp++) {
		strcat(command, " ");
		strcat(command, *listpp);
	}
	
	/* NB:  I think there may be a security problem
	   here, since popen ought to exec $SHELL.  Check this.  */
	/* Now actually call the mail delivery agent.  */
	if ( (xfp = popen(command, "w")) == NULL ) {
		/* Report a send error and return.  */
	}

	/* Pump the headers into the delivery agent. */
	build_header(xfp);

	/* Pump the message into the delivery agent.  */
	rewind(tfp);
	mcopy(tfp, xfp, (fsize_t)0, (fsize_t)MAXLONG, 0);

	fclose(xfp);
	fclose(tfp);
}

xsend(users, tfp) char **users; FILE *tfp;
{
	register char **ulist;
	register char *cp;
	register struct passwd *pwp;
	FILE *xfp;

	for (ulist = users; *ulist!=NULL; ulist++) {
		rewind(tfp);
		sprintf(boxname, "%s%s", SPOOLDIR, *ulist);
		sprintf(cmdname, "xencode %s >> %s", *ulist, boxname);
		if (index(*ulist, '!') != NULL
		 || (pwp = getpwnam(*ulist)) == NULL) {
			mmsg(nosend, *ulist);
			continue;
		}
		if (xaccess(*ulist) == 0) {
			mmsg(nopubk, *ulist);
			continue;
		}
		mlock(pwp->pw_uid);
		if ((xfp = fopen(boxname, "a")) == NULL) {
			mmsg(nosend, *ulist);
			munlock();
			continue;
		}
		chown(boxname, pwp->pw_uid, pwp->pw_gid);
		fprintf(xfp, "From xmail %s %s\n", cp,
			tzname[tp->tm_isdst ? 1 : 0]);
		fclose(xfp);
		if ((xfp = popen(cmdname, "w")) == NULL) {
			mmsg("Can't pipe to xencode\n");
			continue;
		}
		if (fwrite(header, strlen(header), 1, xfp) != 1
		 || mcopy(tfp, xfp, (fsize_t)0, (fsize_t)MAXLONG, 0)) {
			merr(wrerr, cmdname);
		}
		pclose(xfp);
		munlock();
	}
	fclose(tfp);
}

uucpname()
{
	FILE *uufile;

	if (NULL == (uufile = fopen(NODENAME, "r"))) {
		strcpy(mysite, "<unknown>");
		return;
	}
	fgets(mysite, sizeof mysite, uufile);
	mysite[strlen(mysite) - 1] = '\0';	/* remove '\n' */
	fclose(uufile);
	return;
}

domainname()
{
	FILE *domfile;
	if((domfile = fopen(DOMAINNAME, "r")) == NULL) {
		strcpy(domain, ".UNKNOWN");
		return;
	}
	fgets(domain, sizeof domain, domfile);
	domain [strlen(domain) - 1] = '\0';
	fclose (domfile);
	return;
}

char *
arpadate(tp)
struct	tm	*tp;
{
	static	char arpabuf [64];
	static	char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	sprintf(arpabuf, "%d %c%c%c %d %02d:%02d:%02d",
		tp->tm_mday,
		months[tp->tm_mon * 3],
		months[tp->tm_mon * 3 + 1],
		months[tp->tm_mon * 3 + 2],
		tp->tm_year,
		tp->tm_hour, tp->tm_min, tp->tm_sec);
	return arpabuf;
}

char *
msgid(tp)
struct	tm	*tp;
{
	static	char	msgidbuf [32];
	sprintf(msgidbuf, "%02d%02d%02d%02d%02d",
	tp->tm_year, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min);
	return msgidbuf;
}

build_header(xfp)
FILE	*xfp;
{
	char	**ulist;
	int	processid;

	/* Generate a Message-Id.  */
	processid = getpid();
	sprintf(header, "Message-Id: <%s.AA%d.V%s@%s.%s>\n",
		msgid(tp), processid, revnop(), mysite, domain);
	if (fwrite(header, strlen(header), 1, xfp) != 1)
		return 0;

	/* generate Date: and From: lines.  */
	sprintf(header, "Date: %s\nFrom: %s@%s.%s (%s)\n",
		arpadate(tp), myname, mysite, domain, myfullname);
	if (fwrite(header, strlen(header), 1, xfp) != 1)
		return 0;

	/* Generate the full To: lines.  We just spit addresses out
	   verbatum, with no cleanup.  */
	strcpy(header, "To:");
	for (ulist=tolist; *ulist != NULL; ulist++) {
		strcat(header, " ");
		strcat(header, *ulist);
	}
	strcat(header, "\n");
	if (fwrite(header, strlen(header), 1, xfp) != 1)
		return 0;

	/* Generate a Cc: line if needed.  */
	if (cclist[0] != NULL) {
		strcpy(header, "Cc:");
		for (ulist=cclist; *ulist != NULL; ulist++) {
			strcat(header, " ");
			strcat(header, *ulist);
		}
		strcat(header, "\n");
		if (fwrite(header, strlen(header), 1, xfp) != 1)
			return 0;
	}
	return 1;
}

