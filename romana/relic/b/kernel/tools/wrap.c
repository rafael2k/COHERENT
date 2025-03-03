/*
 * wrap.c - fold over lines > 80 characters to avoid truncation during printout
 *
 * usage:  wrap [-ttablen] [-wwraplen] < infile > outfile
 *   tab length defaults to 8
 *   wrap column defaults to 80
 */
#include <stdio.h>

#define WRAPLEN 80
#define TABLEN 8

main(argc, argv)
int argc;
char * argv[];
{
	/*
	 * get tab length from command line or default to TABLEN
	 * get wrap column from command line or default to WRAPLEN
	 */

	int chars_this_line = 0;
	int ch, wl = WRAPLEN, tl = TABLEN;
	int argnum;

	for (argnum = 1; argnum < argc; argnum++) {
		if (argv[argnum][0] == '-')
			switch (argv[argnum][1]) {
			case 't':
				tl = atoi(argv[argnum]+2);
				if (tl <= 0 || tl >= 256)
					tl = TABLEN;
				break;
			case 'w':
				wl = atoi(argv[argnum]+2);
				if (wl <= 0 || wl >= 256)
					wl = WRAPLEN;
				break;
			}
	}

	/*
	 * while (not at end of stdin)
	 *   get next character
	 *   if it's newline
	 *     reset chars_this_line to zero
	 *   else if it's backspace
	 *     if chars_this_line is > 0
	 *       decrement chars_this_line
	 *   else if it's tab
	 *     insert the right number of spaces
	 *   else
	 *     increment chars_this_line
	 *     if chars_this_line > WRAPLEN
	 *       write newline to stdout
	 *       decrement chars_this_line by WRAPLEN
	 *   write character to stdout
	 */
	while ((ch = getchar()) != EOF) {
		if (ch == '\n') {
			chars_this_line = 0;
			putchar(ch);
		} else if (ch == '\b') {
			if (chars_this_line)
				chars_this_line--;
			putchar(ch);
		} else if (ch == '\t') {
			do {
				chars_this_line++;
				if (chars_this_line > wl) {
					putchar('\n');
					chars_this_line -= wl;
				}
				putchar(' ');
			} while (chars_this_line % tl);
		} else {
			chars_this_line++;
			if (chars_this_line > wl) {
				putchar('\n');
				chars_this_line -= wl;
			}
			putchar(ch);
		}
	}
}
