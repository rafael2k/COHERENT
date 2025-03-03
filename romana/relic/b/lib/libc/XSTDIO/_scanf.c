/*
 * libc/stdio/_scanf.c
 * ANSI-compliant C standard i/o library internals.
 * _scanf()
 * ANSI 4.9.6.2.
 * Do the work for fscanf(), scanf(), sscanf().
 *
 * Implementation-defined behavior:
 *	"%[a-z]" conversion treats '-' as a range specification #if SCANFRANGE,
 *		 as a normal character #if !SCANFRANGE
 *	"%p" conversion expects input in printf format "%#.4X" or "%#.8X"
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#undef	isspace

/* Compile-time options. */
#define	LONGDOUBLE	0	/* iff sizeof(double) != sizeof(long double) */

#define	MAX_WIDTH	128	/* max width of conversion item */

int
_scanf(fp, format, args) FILE *fp; const char *format; va_list args;
{
	register int	fc, gc;
	register char *	cp;
	int		base, width, count, nitems, n, sflag, lflag, flag;
	long		val;
	double		d;
	Void *		vp;
	char		buf[MAX_WIDTH];

	for (nitems = count = 0; fc = *format++; ) {

		if (isspace(fc)) {		/* white space directive */
			while (isspace(gc = getc(fp)))
				++count;
			if (gc == EOF)
				break;
			ungetc(gc, fp);
			continue;
		} else if (fc != '%') {		/* ordinary character directive */
matchin:
			if ((gc=getc(fp)) != fc) {
				ungetc(gc, fp);
				break;
			}
			count++;
			continue;
		} else {			/* conversion directive */

			/* Process '*', field width, "hlL" flags. */
			flag = sflag = lflag = 0;
			if ((fc = *format++) == '*') {
				++sflag;
				fc = *format++;
			}
			if (isdigit(fc))
				for (width = 0; isdigit(fc); fc = *format++)
					width = width*10 + fc - '0';
			else
				width = -1;
			if (fc == 'h' || fc == 'l' || fc == 'L') {
				lflag = fc;
				fc = *format++;
			}

			/* Skip leading white space. */
			if (fc != '[' && fc != 'c' && fc != 'n') {
				while (isspace(gc = getc(fp)))
					++count;
				ungetc(gc, fp);
			}

			/* Perform a conversion. */
			/* Three generic cases: fixed, float, string. */
			switch (fc) {

			default:
			case '\0':
				break;

			case 'd':
				base = 10;
				++flag;			/* signed */
				goto fixed;

			case 'i':
				base = 0;
				++flag;			/* signed */
				goto fixed;

			case 'o':
				base = 8;
				goto fixed;

			case 'u':
				base = 10;
				goto fixed;

			case 'X':
			case 'x':
				base = 16;
fixed:
				if (width == -1 || width > MAX_WIDTH)
					width = MAX_WIDTH;
				if ((n = lscan(buf, fp, base, width, flag, &val)) == 0)
					break;
				count += n;
				if (sflag)
					continue;
				if (lflag == 'p')
					*(va_arg(args, Void **)) = (Void *)val;
				else if (lflag == 'l')
					*(va_arg(args, long *)) = val;
				else if (lflag == 'h')
					*(va_arg(args, short *)) = (short)val;
				else
					*(va_arg(args, int *)) = (int)val;
				nitems++;
				continue;

			case 'e':
			case 'f':
			case 'g':
			case 'E':
			case 'G':
				if (width == -1 || width > MAX_WIDTH)
					width = MAX_WIDTH;
				if ((n = _dscan(buf, fp, width, &d)) == 0)
					break;
				count += n;
				if (sflag)
					continue;
#if	LONGDOUBLE
				if (lflag == 'L')
					vp = (Void *)va_arg(args, long double *);
				else if (lflag == 'l')
#else
				if (lflag == 'l' || lflag == 'L')
#endif
					vp = (Void *)va_arg(args, double *);
				else
					vp = (Void *)va_arg(args, float *);
				_dassign(vp, &d, lflag);
				nitems++;
				continue;

			case 's':
scanin:
				if (!sflag)
					cp = va_arg(args, char *);
				for (n = 0; width < 0 || n < width; n++) {
					if ((gc = getc(fp)) == EOF)
						break;
					if ((fc == 's' && isspace(gc))
					 || (fc == '['
					   && flag != (strchr(buf, gc)==NULL))) {
						ungetc(gc, fp);
						break;
					}
					if (!sflag)
						*cp++ = gc;
				}
				if (n == 0)
					break;
				count += n;
				if (sflag)
					continue;
				if (fc != 'c')
					*cp = '\0';
				nitems++;
				continue;

			case '[':
				/* Set flag iff ^ specified. */
				flag = 0;
				if ((fc = *format++) == '^') {
					++flag;
					fc = *format++;
				}
				/* Copy scanlist into buf. */
				cp = buf;
				if (fc == ']') {
					*cp++ = fc;
					fc = *format++;
				}
				while (fc != '\0' && fc != ']') {
#if	SCANFRANGE
					/* Accept character ranges. */
					if (fc == '-' && cp != buf && *format != ']') {
						gc = *(cp-1);	/* start */
						fc = *format++;	/* end */
						if (gc > fc)
							--cp;	/* empty */
						else while (++gc <= fc)
							*cp++ = gc;
					} else
#endif
						*cp++ = fc;
					fc = *format++;
				}
				*cp = '\0';
				fc = '[';
				goto scanin;

			case 'c':
				if (width == -1)
					width = 1;
				goto scanin;

			case 'p':
#if	_LARGE
				width = 10;		/* "0x"+8 hex digits */
#else
				width = 6;		/* "0x"+4 hex digits */
#endif
				base = 16;
				lflag = 'p';
				goto fixed;

			case 'n':
				if (lflag == 'l')
					*(va_arg(args, long *)) = (long)count;
				else if (lflag == 'h')
					*(va_arg(args, short *)) = (short)count;
				else
					*(va_arg(args, int *)) = count;
				continue;

			case '%':
				goto matchin;
			}
			break;
		}
		break;
	}
	return (nitems==0 && feof(fp)) ? EOF : nitems;
}

/*
 * Read an ASCII number in given 'base'
 * containing at most 'width' characters from 'fp' into 'buf'.
 * Evaluate it using strtol()/strtoul() and store value through *lp.
 * Return the number of characters read.
 * The pre-ANSI source for this routine evaluated the number directly.
 * ANSI 4.9.6.2 does not explicitly say anything about overflow,
 * but it implies that integer conversions work like strtol()/strtoul(),
 * so this scans the number into a buffer and uses the function.
 * The routine implements a finite state machine with 8 states:
 *	(0)	Initial state, base == 0
 *	(1)	After sign, base == 0
 *	(2)	After '0', base == 0
 *	(3)	Initial state, base != 0 && base != 16
 *	(4)	After sign, base != 0 && base != 16
 *	(5)	After digit, base != 0
 *	(6)	Initial state, base == 16
 *	(7)	After sign, base == 16
 *	(8)	After '0', base == 16
 * This cannot be done correctly with the single character pushback
 * guaranteed by ungetc(); for example, rejecting "+y", "0xy" or "+0xy"
 * requires two character pushback.
 */
static
int
lscan(buf, fp, base, width, flag, lp)
char *	buf;
FILE *	fp;
int	base, width, flag;
long *	lp;
{
	register int c, state, count;
	register char *cp;

	cp = buf;
	state = (base == 0) ? 0 : (base == 16) ? 6 : 3;
	for (count = 0; count < width; ++count) {
		*cp++ = c = getc(fp);
		switch(c) {
		case '+':
		case '-':
			if (state != 0 && state != 3 && state != 6)
				break;
			++state;
			continue;
		case '0':
			if (state <= 1)
				state = 2;
			else if (state == 2) {
				base = 8;
				state = 5;
			} else if (state <= 5 || state == 8)
				state = 5;
			else
				state = 8;
			continue;
		case 'x':
		case 'X':
			if (state == 2) {
				base = 16;
				state = 5;
			} else if (state == 8)
				state = 5;
			else
				break;
			continue;
		default:
			if (state <= 2) {
				if (isdigit(c)) {
					base = 10;
					state = 5;
				} else
					break;
				continue;
			}
			if ((isdigit(c) && c-'0' < base)
			 || (islower(c) && c-'a'+10 < base)
			 || (isupper(c) && c-'A'+10 < base))
				state = 5;
			else
				break;
			continue;
		}
		--cp;
		ungetc(c, fp);
		break;
	}
	*cp = '\0';
	if (flag)
		*lp = strtol(buf, (char **)NULL, base);
	else
		*lp = strtoul(buf, (char **)NULL, base);
	return (int)(cp - buf);
}

/* end of libc/stdio/_scanf.c */
