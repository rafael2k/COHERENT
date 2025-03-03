/*
 *	trace1.c
 *
 *	The information contained herein is a trade secret of Mark Williams
 *	Company, and  is confidential information.  It is provided  under a
 *	license agreement,  and may be  copied or disclosed  only under the
 *	terms of  that agreement.  Any  reproduction or disclosure  of this
 *	material without the express written authorization of Mark Williams
 *	Company or persuant to the license agreement is unlawful.
 *
 *	COHERENT Version 2.3.35
 *	Copyright (c) 1982, 1983, 1984.
 *	An unpublished work by Mark Williams Company, Chicago.
 *	All rights reserved.
 *
 *	Initialization, command line parsing.
 */
#include <stdio.h>
#include <ctype.h>
#include <canon.h>
#include <l.out.h>
#include <sys/timeout.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <sys/uproc.h>
#include "trace.h"

static char *symName();

main(argc, argv)
char **argv;
{
	initialise();
	setup(argc, argv);
	signal(SIGINT, &armsint);
	signal(SIGQUIT, SIG_IGN);
	process();
}

/*
 * Leave.
 */
leave()
{
	nfree(ssymp);
	nfree(gblsymMap);
	killc();
	exit(0);
}

/*
 * Initialize segment formats and clear the breakpoint table.
 */
initialise()
{
	strcpy(segform[DSEG], "w");
	strcpy(segform[ISEG], "i");
	strcpy(segform[USEG], "w");
	bptinit();
}

/*
 * Setup arguments.
 */
setup(argc, argv)
char **argv;
{
	register char *cp;
	register int c;
	register int t;
	register int u;
	register int tflag;

	t = '\0';
	tflag = 0;

	/* process command line switches -[cdefkorst] */
	for (; argc>1; argc--, argv++) {
		cp = argv[1];
		if (*cp++ != '-')
			break;
		while ((c=*cp++) != '\0') {
			switch (c) {
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'k':
			case 'o':
				/* only one of [cdefko] is allowed */
				if (t != '\0')
					usage();
				t = c;
				continue;
			case 'r':
				rflag = 1;
				continue;
			case 's':
				sflag = 1;
				continue;
			case 't':
				tflag = 1;
				continue;
			default:
				usage();
			}
		}
	}
	switch (t) {
	case '\0':
		switch (argc) {
		case 1:
			setfh(DEFLT_OBJ, 3);
			setcore(DEFLT_AUX);
			break;
		case 2:
			setfh(argv[1], 3);
			break;
		case 3:
			setfh(argv[1], 3);
			setcore(argv[2]);
			break;
		default:
			usage();
		}
		break;
	case 'c':
		switch (argc) {
		case 1:
			setfh(DEFLT_OBJ, 3);
			setcore(DEFLT_AUX);
			break;
		case 2:
			setcore(argv[1]);
			break;
		case 3:
			setfh(argv[1], 3);
			setcore(argv[2]);
			break;
		default:
			usage();
		}
		break;
	case 'd':
		switch (argc) {
		case 1:
			setfh("/coherent", 3);
			setdump("/dev/dump");
			break;
		case 3:
			setfh(argv[1], 3);
			setdump(argv[2]);
			break;
		default:
			usage();
		}
		break;
	case 'e':
		if (argc < 2)
			usage();
		setfh(argv[1], 3);
		if (startup(&argv[1], NULL, NULL, 0)==0 || shiftup()==0)
			leave();
		break;
	case 'f':
		switch (argc) {
		case 2:
			setfile(argv[1]);
			break;
		case 3:
			setfh(argv[1], 1);
			setfile(argv[2]);
			break;
		default:
			usage();
		}
		break;
	case 'k':
		switch (argc) {
		case 1:
			setfh("/coherent", 1);
			setkmem("/dev/mem");
			break;
		case 2:
			setkmem(argv[1]);
			break;
		case 3:
			setfh(argv[1], 1);
			setkmem(argv[2]);
			break;
		default:
			usage();
		}
		break;
	case 'o':
		switch (argc) {
		case 1:
			setfh(DEFLT_OBJ, 3);
			break;
		case 2:
			setfh(argv[1], 3);
			break;
		case 3:
			setfh(argv[1], 1);
			setfh(argv[2], 2);
			break;
		default:
			usage();
		}
	}
	if (tflag) {
		if ((u=open("/dev/tty", 2)) < 0)
			panic("Cannot open /dev/tty");
		dup2(u, 0);
		dup2(u, 1);
		dup2(u, 2);
	}
}

/*
 * Generate a usage message.
 */
usage()
{
	panic("Usage: db [-cdefort] [object file] [auxiliary file]");
}

/*
 * Catch and flag interrupts.
 */
armsint()
{
	signal(SIGINT, &armsint);
	intflag++;
}

/*
 * Check for interrupts and clear flag.
 */
testint()
{
	register int n;

	if ((n=intflag) != 0)
		printe("Interrupt");
	intflag = 0;
	return (n);
}

/*
 * Assume the file, `np', is the name of an l.out.  If the bottom bit
 * of the flag is set, set up the symbol table.  If the next bit is set,
 * set up segmentation for the l.out.
 */
setlout(np, f)
char *np;
{
	struct ldheader ldh;

	fseek(lfp, 0L, 0L);
	if (fread(&ldh, sizeof(ldh), 1, lfp) != 1)
		panic("Cannot read object file");
	canlout(&ldh);
	if (ldh.l_magic != L_MAGIC)
		panic("Bad object file");
	hdrinfo.magic = ldh.l_magic;
	hdrinfo.defsegatt = DSA16;
	objflag = 1;
	if ((f&1) != 0) {
		sbase = sizeof(ldh);
		sbase += ldh.l_ssize[L_SHRI] + ldh.l_ssize[L_PRVI];
		sbase += ldh.l_ssize[L_SHRD] + ldh.l_ssize[L_PRVD];
		snsym = ldh.l_ssize[L_SYM] / sizeof(struct ldsym);
		if (snsym != 0) {
			sfp = lfp;
			if (sflag == 0)
				readsym(sfp);
		}
	}
	if ((f&2) != 0) {
		lfn = np;
		setaseg(&ldh);
	}
}

/*
 * Canonicalize an l.out header.
 */
canlout(ldp)
struct ldheader *ldp;
{
	register int i;

	canint(ldp->l_magic);
	canint(ldp->l_flag);
	canint(ldp->l_machine);
	canvaddr(ldp->l_entry);
	for (i=0; i<NLSEG; i++)
		cansize(ldp->l_ssize[i]);
}

/*
 * Read all global symbols into memory.
 */
readsym(fp)
FILE *fp;
{
	register int n, i;
	register SYM *sp;
	struct ldsym lds;
	struct LDSYM LDS;

	if ((ssymp=nalloc((int)snsym * sizeof(SYM))) == NULL)
		return;
	fseek(fp, (long)sbase, 0);
	for (n=snsym, sp=ssymp; n--; sp++) {
		if (fread(&lds, sizeof(struct ldsym), 1, fp) != 1) {
			nfree(ssymp);
			ssymp = NULL;
			return;
		}
		canint(lds.ls_type);
		canvaddr(lds.ls_addr);
		for (i=0; i<NCPLN; i++) 
			LDS.ls_id[i] = lds.ls_id[i];
		LDS.ls_type = lds.ls_type;
		LDS.ls_addr = (long)lds.ls_addr;

		sp->s_hash = hash(&LDS);
		sp->s_type = LDS.ls_type;
		sp->s_sval = LDS.ls_addr;
	}
}

/*
 * Hash a symbol name.
 */
hash(ldp)
struct LDSYM *ldp;
{
	register int h, n;
	register char *cp;

	h = 0;
	n = CCSSIZE;
	cp = ldp->ls_id;
	do {
		if (*cp != '_')
			h += *cp;
		if (*cp++ == '\0')
			break;
	} while (--n);
	return (h&0377);
}

/*
 * Set up segmentation for a core dump.  The registers are also read.
 */
setcore(np)
char *np;
{
	register unsigned i;
	register MAP *mp;
	register char *cp;
	register char *lcp;
	register fsize_t size;
	register fsize_t offt;
	char ucomm[10];
	SR usegs[NUSEG];

	cfp = openfil(np, rflag);
	fseek(cfp, (long)offset(uproc, u_comm[0]), 0);
	if ((cp=lfn)!=NULL && fread(ucomm, sizeof(ucomm), 1, cfp)==1) {
		lcp = cp;
		while (*cp) {
			if (*cp++ == '/')
				lcp = cp;
		}
		if (strncmp(lcp, ucomm, sizeof(ucomm)) != 0)
			printr("Core different from object");
	}
	fseek(cfp, (long)offset(uproc, u_segl[0]), 0);
	if (fread(usegs, sizeof(usegs), 1, cfp) != 1)
		panic("Bad core file");
	USPACE = setsmap(NULL, (fsize_t)0, (fsize_t)UPASIZE, (fsize_t)0,
			getf, putf, 1);
	mp = clrsmap(DSPACE, endpure);
	offt = usegs[0].sr_size;
#ifdef _I386
	usegs[SISTACK].sr_base -= usegs[SISTACK].sr_size;
#endif
	for (i=1; i<NUSEG; i++) {
		if (usegs[i].sr_segp == NULL)
			continue;
		if ((~usegs[i].sr_flag) & (SRFDUMP|SRFPMAP))
			continue;
		size = usegs[i].sr_size;
		mp = setsmap(mp, (fsize_t)usegs[i].sr_base, size, offt,
				getf, putf, 1);
		offt += size;
	}
	if (ISPACE == DSPACE)
		ISPACE = mp;
	DSPACE = mp;
	settrap();
	setregs();
}

/*
 * Find out signal sent to traced process.
 */
settrap()
{
	int f;

	trapstr = "???";
	add = PTRACE_SIG;
	if (getb(USEG, (char *)&f, sizeof(f)) == 0) {
		printr("Cannot read signal in traced process");
		return 0;
	}
	if (f<=0 || f>NSIG) {
		printr("Bad signal in traced process");
		return 0;
	}
	trapstr = signame[f-1];
	return f;
}

/*
 * Set up segmentation for an ordinary file.
 * This is really easy.
 */
setfile(np)
char *np;
{
	lfp = openfil(np, rflag);
	DSPACE = setsmap(NULL, (fsize_t)0, (fsize_t)LI, (fsize_t)0,
		getf, putf, 0);
	ISPACE = DSPACE;
}

/*
 * Open the given file.  If `rflag' is set, the file is opened for
 * read only.
 */
FILE *
openfil(np, rflag)
char *np;
{
	register FILE *fp;

	if (rflag) {
		if ((fp=fopen(np, "r")) != NULL)
			return (fp);
	} else {
		if ((fp=fopen(np, "r+w")) != NULL)
			return (fp);
		if ((fp=fopen(np, "r")) != NULL) {
			printr("%s: opened read only", np);
			return (fp);
		}
	}
	panic("Cannot open %s", np);
}

/*
 * Initialise an element of a segment map.
 */
MAP * setsmap(next, base, size, offt, getf, putf, segi)
MAP *next;
fsize_t base;
fsize_t size;
fsize_t offt;
int (*getf)();
int (*putf)();
{
	register MAP *mp;

	mp = (MAP *)nalloc(sizeof(MAP));
	mp->m_next = next;
	mp->m_base = base;
	mp->m_bend = base+size;
	mp->m_offt = offt;
	mp->m_getf = getf;
	mp->m_putf = putf;
	mp->m_segi = segi;
	return (mp);
}

/*
 * Clear a section of a segment map.
 */
MAP * clrsmap(mp, np)
register MAP *mp;
register MAP *np;
{
	while (mp != np) {
		nfree(mp);
		mp = mp->m_next;
	}
	return (mp);
}

/*
 * Clear all maps.
 */
clramap()
{
	register int n;

	n = 0;
	if (segmapl[0] == segmapl[1])
		segmapl[n++] = NULL;
	while (n < NSEGM) {
		segmapl[n] = clrsmap(segmapl[n], NULL);
		n++;
	}
}


/******************************************************************************
 *
 * Note some #defines in these include files interferes with
 * items in preceeding include files. Hence the strange
 * program order.
 *
 ******************************************************************************/

#include <coff.h>

char	*read_str_tab();
SCNHDR	*read_scns();

/*
 * Setup object file.
 *
 * Arguments:
 *	np = file name
 *	f is bit mapped
 *		set 1's bit if reading symbol table info from file "np"
 *		set 2's bit if reading segment info from file "np"
 */
setfh (np,f)
char	*np;
{
	FILEHDR coffh;
	
	lfp = openfil(np, (f&2) ? rflag : 1);
	if (fread(&coffh, sizeof(coffh), 1, lfp) != 1)
		panic("Cannot read object file");
	if (coffh.f_magic == C_386_MAGIC)
		setcoff(lfp, &coffh, np, f);
	else	setlout(np, f);
}

setcoff(fp, coffhp, np, f)
FILE	*fp;
FILEHDR	*coffhp;
char	*np;
int	f;
{
	hdrinfo.magic = coffhp->f_magic;
	hdrinfo.defsegatt = DSA32;
	objflag = 1;
	if (f&1) {
		if (coffhp->f_nsyms) {
			sbase = (fsize_t)coffhp->f_symptr;
			snsym = (fsize_t)coffhp->f_nsyms; 
			if (snsym != 0L) {
				sfp = fp;
				if (sflag == 0) {
					readCoffSym(fp, coffhp);
				}
			}
		}
	}
	if (f&2) {
		lfn = np;
		setcoffseg(fp, coffhp);
	}
}

readCoffSym(fp, coffhp)
FILE	*fp;
FILEHDR	*coffhp;
{
	SYMENT		coffsym;
	struct	LDSYM	lds;
	SYM		*sp;
	long		n;
	char		*str_tabp;
	SCNHDR		*scnhp;


	str_tabp = read_str_tab(fp, coffhp->f_nsyms, coffhp->f_symptr);

	scnhp = read_scns(fp, coffhp->f_nscns
				, sizeof(FILEHDR)+coffhp->f_opthdr);

	/* could get entry point here FIX 386 */

	if (scnhp == NULL)
		return;

	if ((gblsymMap=nalloc((int)coffhp->f_nsyms * sizeof(off_t))) == NULL) {
		printf("Not enough memory for the global symbol map.\n");
		return;
	}
	if ((ssymp=nalloc((int)coffhp->f_nsyms * sizeof(SYM))) == NULL) {
		printf("Not enough memory for the coff symbol header section.\n");
		return;
	}

	fseek(fp, coffhp->f_symptr, 0);

	for (n=coffhp->f_nsyms, sp=ssymp; n--; ) {
		if (fread(&coffsym, sizeof(SYMENT), 1, fp) != 1) {
			printf("Cannot read coff.h symbols\n");
			nfree(ssymp);
			ssymp = NULL;
			return;
		}

		switch (coffsym.n_sclass) {
		case C_EXT:
		case C_EXTDEF:
			break;
		default:
			continue;
		}

		coff_to_lout(&coffsym, &lds, str_tabp, scnhp);

		sp->s_hash = hash(&lds);
		sp->s_type = lds.ls_type;
		sp->s_sval = lds.ls_addr;

		gblsymMap[sp-ssymp] = snsym - n - 1;
		sp++;
	}
	gblsymMap=(off_t *)realloc( gblsymMap, (sp-ssymp)*sizeof(off_t) );	
	ssymp=(SYM *)realloc( ssymp, (sp-ssymp)*sizeof(SYM) );	
	sngblsym = sp - ssymp;
	if (str_tabp) nfree(str_tabp);
	nfree(scnhp);
}

char	*
read_str_tab(fp, nsyms, symptr)
FILE	*fp;
long	nsyms;
long	symptr;
{
	long		strtab_len, symtab_len;
	unsigned	len;
	char	*	strTabp;

	strtab_len = 0;
	strTabp = NULL;
	len = symtab_len = sizeof(SYMENT) * nsyms;	
	if (!len)
		return(NULL);

	if (symtab_len != len) {
		panic("Cannot process small model");
	}
	fseek(fp, symptr+len, 0);
	if (1 != fread(&strtab_len, sizeof(strtab_len), 1, fp)) {
		strtab_len = 0;
	}
	if (!strtab_len)
		return(NULL);

	len = strtab_len -= 4;
	if (len != strtab_len)
		panic("Cannot process small model");
	if ( (strTabp = nalloc((int)len)) == NULL ) {
		printf("Not enough memory for the symbol string table.\n");
		return(NULL);
	}
	if ( fread(strTabp, len, 1, fp) != 1) {
		printf("Unable to read symbol string table.\n");
		return(NULL);
	}

	return(strTabp);
}

SCNHDR	*
read_scns(fp, nscns, scnh_off)
FILE	*fp;
unsigned short	nscns, scnh_off;
{
	int 	len;
	SCNHDR	*scnhp;

	len = (int)(nscns*sizeof(SCNHDR));
	if ( (scnhp = nalloc(len)) == NULL ) {
		printf("Not enough memory for the symbol string table.\n");
		return(NULL);
	}
	fseek(fp, scnh_off, 0);
	if ( fread(scnhp, len, 1, fp) != 1) {
		printf("Unable to read section headers.\n");
		return(NULL);
	}
	return(scnhp);
}

coff_to_lout(coffsymp, ldsp, str_tabp, scnhp)
SYMENT		*coffsymp;
struct	LDSYM	*ldsp;
char		*str_tabp;
SCNHDR		*scnhp;
{
	char	*cp;
	char	w1[SYMNMLEN+1];
	
	cp = symName(coffsymp, w1, str_tabp);
	memcpy(ldsp->ls_id, cp, NCPLN-1);
	ldsp->ls_id[NCPLN-1] = '\0';

	switch((int)scnhp[coffsymp->n_scnum-1].s_flags) {
	case STYP_TEXT:
		ldsp->ls_type = L_GLOBAL + L_SHRI;
		break;
	case STYP_DATA:
		ldsp->ls_type = L_GLOBAL + L_PRVD;
		break;
	case STYP_BSS:
		ldsp->ls_type = L_GLOBAL + L_BSSD;
		break;
	default:
		ldsp->ls_type = -1;
		panic("Unallowed section flag in coff header");
	}

	ldsp->ls_addr = (long)coffsymp->n_value;
}

/*
 * Symbol name.
 */
static char *
symName(sym, work, str_tabp)
SYMENT	*sym;
char	*work;
char	*str_tabp;
{
	if (!sym->n_zeroes) {
		return (str_tabp + sym->n_offset - 4);
	}

	/* make sure it's zero terminated */
	memcpy(work, sym->n_name, SYMNMLEN);
	work[SYMNMLEN] = '\0';
	return (work);
}

/* end of trace1.c */
