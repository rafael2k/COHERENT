/*
 * Common routines to read and write words
 * in the compressed dictionary for the
 * simple (but complete) version of spell.
 */

#include <stdio.h>
#include <ctype.h>

/*
 * The coding is as follows:
 * 1-26 are letters, 27 is `-', 28 is apostrophe.
 * 0 is the pad character at word's end.
 * 29 is optional `s', 30-31 is unused.
 * They are packed with 3 per 16 bits (canonically)
 * and the 16th bit is the end-of-word marker.
 * This is all done independent of byte-ordering or
 * word size.  All that is required is at least 8-bit
 * bytes (chars) and at least 16-bit words (ints).
 */

static	char	compress[256];	/* Initialised in init */

static	char	expand[] = {
	'\0',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
	'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
	'y', 'z', '-','\'', '#','\0','\0'
};

#define	EWORD	0100000		/* End of word */
#define	NWORD	50		/* Length of a word */
#define	NCLONE	500		/* Number of clones to save (maximum) */
#define	NSPACE	2000		/* Size of string space for plurals */

static	char	word[NWORD];	/* Static space returned by getword() */
static	char	space[NSPACE];
static	char	*spacep = space;
static	char	*clones[NCLONE];	/* Table of saved plurals */
static	int	nclone;

/*
 * Initialise the compressed table.
 * This is independent of character set
 * representation (e.g. ascii or ebcdic)
 * and only has to be called if putword is used.
 */
init()
{
	compress['-'] = 27;
	compress['\'']= 28;
	compress['#'] = 29;
	compress['a'] = compress['A'] = 1;
	compress['b'] = compress['B'] = 2;
	compress['c'] = compress['C'] = 3;
	compress['d'] = compress['D'] = 4;
	compress['e'] = compress['E'] = 5;
	compress['f'] = compress['F'] = 6;
	compress['g'] = compress['G'] = 7;
	compress['h'] = compress['H'] = 8;
	compress['i'] = compress['I'] = 9;
	compress['j'] = compress['J'] = 10;
	compress['k'] = compress['K'] = 11;
	compress['l'] = compress['L'] = 12;
	compress['m'] = compress['M'] = 13;
	compress['n'] = compress['N'] = 14;
	compress['o'] = compress['O'] = 15;
	compress['p'] = compress['P'] = 16;
	compress['q'] = compress['Q'] = 17;
	compress['r'] = compress['R'] = 18;
	compress['s'] = compress['S'] = 19;
	compress['t'] = compress['T'] = 20;
	compress['u'] = compress['U'] = 21;
	compress['v'] = compress['V'] = 22;
	compress['w'] = compress['W'] = 23;
	compress['x'] = compress['X'] = 24;
	compress['y'] = compress['Y'] = 25;
	compress['z'] = compress['Z'] = 26;
}

/*
 * Read a word from the dictionary,
 * one `dfp'.  Return pointer to
 * word or NULL on EOF.
 * `*' words are cloned into the singular
 * and plural. The plural is saved until
 * it comes up in collating order.
 */
char *
getword(dfp)
FILE *dfp;
{
	register char *wp;
	register int t = EWORD;
	static int clone;
	static int saved;

	if (saved) {
		saved = 0;
	} else {
		clone = 0;
		wp = word;
		for (;;) {
			register int c;

			if ((c = getc(dfp)) == EOF)
				break;
			t = c;
			if ((c = getc(dfp)) == EOF)
				goto bad;
			t <<= 8;
			t |= c;
			if ((*wp++ = expand[t&037]) == '#')
				clone++;
			if ((*wp++ = expand[(t>>5)&037]) == '#')
				clone++;
			if ((*wp++ = expand[(t>>10)&037]) == '#')
				clone++;
			if (t & EWORD)
				break;
		}
		if (wp == word)
			return (NULL);
		if ((t&EWORD) == 0)
			goto bad;
		if (clone)
			while (wp > word)
				if (*--wp == '#')
					break;
		*wp = '\0';
	}
	if (nclone && strcmp(word, clones[nclone-1])>0) {
		wp = clones[--nclone];
		saved++;
		return (spacep = wp);
	} else if (clone) {
		plural(word);
		clone = 0;
	}
	return (word);
bad:
	fprintf(stderr, "spell: bad dictionary format\n");
	exit(1);
}

/*
 * Pluralise the word, by adding an 's' to it.
 * Save a copy away in the list of pending plurals
 * (`clones').
 */
static
plural(s)
char *s;
{
	register char *cp;
	register char *as;

	for (cp=s; *cp++ != '\0'; )
		;
	*--cp = 's';
	cp++;
	*cp++ = '\0';
	if (nclone >= NCLONE) {
		fprintf(stderr, "spell: too many saved plurals\n");
		exit(1);
	}
	as = spacep;
	if ((spacep += cp-s) >= &space[NSPACE]) {
		fprintf(stderr, "spell: out of memory for plurals\n");
		exit(1);
	}
	strcpy(as, s);
	clones[nclone++] = as;
	cp[-2] = '\0';		/* Restore original word */
}

/*
 * Put out the ascii word, onto the
 * dictionary file in compressed form.
 */
putword(wp, dfp)
register char *wp;
FILE *dfp;
{
	register int l;

	while ((l = compress[*wp++]) != 0)
		putlet(l, dfp);
	putlet(0, dfp);
}

/*
 * Put out a single compressed letter.
 * 0 Marks end of word.
 */
putlet(l, fp)
register int l;
register FILE *fp;
{
	static int n = 0;
	static int word = 0;

	if (l != 0) {
		word |= l<<(n*5);
		if (++n < 3)
			return;
	} else
		word |= EWORD;
	putc(word>>8, fp);
	putc(word&0377, fp);
	n = word = 0;
	word = 0;
}
