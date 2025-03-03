%{
/*
 * Set return status based on
 * various specified conditions,
 * mostly related to files.
 * Used mostly in shell files.
 */

#include <stdio.h>
#include <sys/stat.h>
#include <access.h>
#include "testnode.h"

#define	NPRIM	(sizeof(prims)/sizeof(prims[0]))
#define	NFNAME	500		/* size of filename buffer */
%}
%start	command

%union	{
	NODE *nodeptr;
	char *fname;
	}

%left	OR
%left	AND
%left	'!'
%token	_R _W _F _D _S _T _Z _N
%token	SEQ SNEQ
%token	_EQ _NE _GT _GE _LT _LE
%type	<nodeptr> exp
%token	<fname> STR
%%

command:
	exp '\n'		{ code = $1; return; }
	;

exp:
        '(' exp ')'		{ $$ = $2; }
      | '!' exp			{ $$ = bnode('!', $2, NULL); }
      | exp OR exp		{ $$ = bnode(OR, $1, $3); }
      | exp AND exp		{ $$ = bnode(AND, $1, $3); }
      | _R STR			{ $$ = lnode(xr, $2, NULL); }
      | _W STR			{ $$ = lnode(xw, $2, NULL); }
      | _F STR			{ $$ = lnode(xf, $2, NULL); }
      | _D STR			{ $$ = lnode(xd, $2, NULL); }
      | _S STR			{ $$ = lnode(xs, $2, NULL); }
      | _T STR			{ $$ = lnode(xt, $2, NULL); }
      | _T			{ $$ = lnode(xt, "1", NULL); }
      | _Z STR			{ $$ = lnode(xz, $2, NULL); }
      | _N STR			{ $$ = lnode(xn, $2, NULL); }
      | STR SEQ STR		{ $$ = lnode(xseq, $1, $3); }
      | STR SNEQ STR		{ $$ = lnode(xsneq, $1, $3); }
      | STR _EQ STR		{ $$ = lnode(xeq, $1, $3); }
      | STR _NE STR		{ $$ = lnode(xne, $1, $3); }
      | STR _GT STR		{ $$ = lnode(xgt, $1, $3); }
      | STR _GE STR		{ $$ = lnode(xge, $1, $3); }
      | STR _LT STR		{ $$ = lnode(xlt, $1, $3); }
      | STR _LE STR		{ $$ = lnode(xle, $1, $3); }
      | STR			{ $$ = lnode(xn, $1, NULL); }
	;

%%
struct	prim	{
	char	*p_name;
	int	p_lval;
	int	p_bin;
}	prims[] = {
	"-r", _R, 0,
	"-w", _W, 0,
	"-f", _F, 0,
	"-d", _D, 0,
	"-s", _S, 0,
	"-t", _T, 0,
	"-z", _Z, 0,
	"-n", _N, 0,
	"-eq", _EQ, 1,
	"-ne", _NE, 1,
	"-gt", _GT, 1,
	"-ge", _GE, 1,
	"-lt", _LT, 1,
	"-le", _LE, 1,
	"-o", OR, 1,
	"-a", AND, 1
};


char	**gav;
int	gac;

struct	stat	sb;

NODE	*code;

char	*next();
NODE	*bnode();
NODE	*lnode();
long	atol();
int	xr();
int	xw();
int	xf();
int	xd();
int	xs();
int	xt();
int	xz();
int	xn();
int	xseq();
int	xsneq();
int	xeq();
int	xne();
int	xgt();
int	xge();
int	xlt();

main(argc, argv)
char *argv[];
{
	gav = argv+1;
	gac = argc-1;
	if (argv[0][0]=='[' && argv[0][1]=='\0') {
		if (strcmp(argv[gac], "]") != 0)
			tsterr("unbalanced [..]");
		gac--;
	}
	if (gac == 0)
		exit(1);
	yyparse();
	exit(!execute(code));
}

/*
 * Lexical analyser
 */
yylex()
{
	static char laststr = 0; /* 1 if last token was string */
	register char *ap;
	register struct prim *pp;

	if ((yylval.fname = ap = next()) == NULL)
		return ('\n');

	if (*ap == '-') {
		for (pp = prims; pp < &prims[NPRIM]; pp++) {
			if (strcmp(pp->p_name, ap) == 0) {
				if (!laststr && pp->p_bin)
					break;
				laststr = 0;
				return (pp->p_lval);
			}
		}
	}
	else {
		laststr = 0;
		if (strcmp("!=", ap) == 0)
			return (SNEQ);

		if (ap[1] == '\0') {
			if (ap[0]==')') {
				laststr = 1;
				return(')');
			}
			if (ap[0]=='(' || ap[0]=='!')
				return (ap[0]);
			if (ap[0]=='=')
				return (SEQ);
		}
	}
	laststr = 1;
	return (STR);
}

yyerror()
{
	fprintf(stderr, "Test expression syntax error\n");
	usage();
}

/*
 * Return the next argument from the arg list.
 */
char *
next()
{
	if (gac < 1)
		return (NULL);
	gac--;
	return (*gav++);
}

/*
 * Build an expression tree node (non-leaf)
 */
NODE *
bnode(op, left, right)
int op;
NODE *left, *right;
{
	register NODE *np;
	char *malloc();

	if ((np = (NODE *)malloc(sizeof (NODE))) == NULL)
		tsterr("Out of space");
	np->n_un.n_op = op;
	np->n_left = left;
	np->n_right = right;
	return (np);
}

/*
 * Build a leaf node in expression tree.
 */
NODE *
lnode(fn, str1, str2)
int (*fn)();
char *str1, *str2;
{
	register NODE *np;
	char *malloc();

	if ((np = (NODE *)malloc(sizeof (NODE))) == NULL)
		tsterr("Out of space");
	np->n_left = np->n_right = NULL;
	np->n_un.n_fun = fn;
	np->n_s1 = str1;
	np->n_s2 = str2;
	return (np);
}

/*
 * Execute compiled code.
 */
execute(np)
register NODE *np;
{
	if (np->n_left != NULL)
		switch (np->n_un.n_op) {
		case AND:
			if (execute(np->n_left) && execute(np->n_right))
				return (1);
			return (0);

		case OR:
			if (execute(np->n_left) || execute(np->n_right))
				return (1);
			return (0);

		case '!':
			return (!execute(np->n_left));

		default:
			tsterr("Panic: bad tree (op %d)", np->n_un.n_op);
		}
	else
		return ((*np->n_un.n_fun)(np));
	/* NOTREACHED */
}

/*
 * Check to see if the file exists
 * and if readable.
 */
xr(np)
NODE *np;
{
	return (access(np->n_s1, AREAD) >= 0);
}

/*
 * Check if the file exists and is
 * writeable.
 */
xw(np)
NODE *np;
{
	return (access(np->n_s1, AWRITE) >= 0);
}

/*
 * Check if the file exists and is not
 * a directory.
 */
xf(np)
NODE *np;
{
	return (stat(np->n_s1, &sb)>=0 && (sb.st_mode&S_IFMT)!=S_IFDIR);
}

/*
 * Check to see if the file exists
 * and is a directory.
 */
xd(np)
NODE *np;
{
	return (stat(np->n_s1, &sb)>=0 && (sb.st_mode&S_IFMT)==S_IFDIR);
}

/*
 * Check to see if the file exists
 * and has a non-zero size.
 */
xs(np)
NODE *np;
{
	return (stat(np->n_s1, &sb)>=0 && sb.st_size>0);
}

/*
 * Check to see if the file
 * descriptor is associated
 * with a terminal.
 */
xt(np)
NODE *np;
{
	return (isatty(atoi(np->n_s1)));
}

/*
 * True if the length of the given
 * string is zero.
 */
xz(np)
NODE *np;
{
	return (np->n_s1[0] == '\0');
}

/*
 * True if the length of the given
 * string is non-zero.
 */
xn(np)
NODE *np;
{
	return (np->n_s1[0] != '\0');
}

/*
 * True if the two strings are
 * lexicographically equal.
 */
xseq(np)
register NODE *np;
{
	return (strcmp(np->n_s1, np->n_s2) == 0);
}

/*
 * True if the two strings are
 * lexicographically unequal.
 */
xsneq(np)
register NODE *np;
{
	return (strcmp(np->n_s1, np->n_s2) != 0);
}

/*
 * True if the two numbers are
 * equal.
 */
xeq(np)
register NODE *np;
{
	return (atol(np->n_s1) == atol(np->n_s2));
}

/*
 * True if the two numbers are
 * not equal.
 */
xne(np)
register NODE *np;
{
	return (atol(np->n_s1) != atol(np->n_s2));
}

/*
 * True if the first number is
 * greater than the second.
 */
xgt(np)
register NODE *np;
{
	return (atol(np->n_s1) > atol(np->n_s2));
}

/*
 * True if the first number is
 * greater than or equal to the second.
 */
xge(np)
register NODE *np;
{
	return (atol(np->n_s1) >= atol(np->n_s2));
}

/*
 * True if the first number is
 * less than the second.
 */
xlt(np)
register NODE *np;
{
	return (atol(np->n_s1) < atol(np->n_s2));
}

/*
 * True if the first number is
 * less than or equal to the second.
 */
xle(np)
register NODE *np;
{
	return (atol(np->n_s1) <= atol(np->n_s2));
}

/*
 * Error messages.
 */
/* VARARGS */
tsterr(x)
{
	fprintf(stderr, "test: %r\n", &x);
	exit(1);
}

usage()
{
	fprintf(stderr, "Usage: test expression\n");
	exit(1);
}
