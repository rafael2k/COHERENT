/*
 * libc/stdio/setvbuf.c
 * ANSI-compliant C standard i/o library.
 * setvbuf()
 * ANSI 4.9.5.6.
 * Set i/o buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include "stdio.int.h"

/*
 * The get and put function pointers in a FILE
 * initially contain &_fginit() and &_fpinit().
 * The buffering characteristics of the stream
 * are determined either by an explicit setvbuf()
 * before the first get or put on the stream
 * or by a setvbuf() call from finit() on the first get or put.
 * setvbuf() allocates a buffer if necessary and
 * initializes various fields in the FILE.
 *
 * For unbuffered streams, _cc == 0 and _bp == _cp == _dp == _ep == NULL.
 * For buffered streams:
 *	_cc == 0	nothing is buffered
 *	_cc > 0		_cc input or output characters are buffered
 *	_bp		base of buffer
 *	_cp		current buffer pointer
 *	_dp		data pointer
 *	_ep		end of buffer pointer
 * The old version of the stdio functions used
 *	_cc < 0		-(_cc) input characters are buffered
 *	_cc > 0		_cc output characters are buffered
 * which makes much more sense but is unfortunately not compatible with
 * the Unix version of stdio.h.  This stdio gets very confused if you
 * mix input and output calls without fflush() in between on a rw stream;
 * this is non-ANSI compliant but works right with the old version.
 */
int
setvbuf(stream, buf, mode, size) register FILE *stream; char *buf; int mode; size_t size;
{
	register _FILE2 *f2p;

#if	_ASCII
	register int isascii;

	isascii = stream->_ff2 & _FASCII;
#endif

	if (stream->_mode != _MODE_UNINIT)
		return 1;			/* Mode assigned, failure */

	/* Allocate a buffer if necessary. */
	if (mode != _IONBF && buf == NULL) {
		if ((buf = malloc(size)) == NULL)
			return 1;		/* Buffer allocation failure */
		stream->_ff2 |= _FFREEB;	/* Free buffer when closed */
	}

	f2p = stream->_f2p;
	switch(mode) {

	case _IOFBF:				/* Fully buffered. */
		stream->_ff1 |= _IOFBF;
		stream->_mode = _MODE_FBUF;
#if	_ASCII
		f2p->_pt = isascii ? &_fputba : &_fputb;
#else
		f2p->_pt = &_fputb;
#endif
		f2p->_dp = stream->_cp = buf + boffset(stream, size);
	lab:
#if	_ASCII
		f2p->_gt = isascii ? &_fgetba : &_fgetb;
#else
		f2p->_gt = &_fgetb;
#endif
		f2p->_bp = buf;
		f2p->_ep = buf + size;
		break;

	case _IOLBF:				/* Line buffered */
		stream->_ff1 |= _IOLBF;
		stream->_mode = _MODE_LBUF;
#if	_ASCII
		f2p->_pt = isascii ? &_fputta : &_fputt;
#else
		f2p->_pt = &_fputt;
#endif
		f2p->_dp = stream->_cp = buf;
		goto lab;

	case _IONBF:				/* Unbuffered */
		stream->_ff1 |= _IONBF;
		stream->_mode = _MODE_NBUF;
#if	_ASCII
		f2p->_gt = isascii ? &_fgetca : &_fgetc;
		f2p->_pt = isascii ? &_fputca : &_fputc;
#else
		f2p->_gt = &_fgetc;
		f2p->_pt = &_fputc;
#endif
		break;

	default:				/* Unrecognized mode */
		return 1;
	}

	if (stream->_ff1 & _FWONLY)
		f2p->_gt = &_fgete;
	if (stream->_ff1 & _FRONLY)
		f2p->_pt = &_fpute;
	stream->_cc = 0;
	return 0;				/* Success */
}

/*
 * Return the current offset of a stream.
 * This is an attempt to keep buffered file i/o properly aligned.
 * For example, a 200 byte file opened in append mode with a
 * default buffer size of 512 bytes will have boffset 200
 * and subsequent write()s will hopefully occur at 512-byte boundaries.
 * If the user calls setvbuf() explicitly with a strange buffer size
 * (not a multiple of the size for optimum disk i/o),
 * the user will get degraded performance.
 */
static
int
boffset(fp, size) register FILE *fp; register size_t size;
{
	register long offset;

	if ((offset=lseek(fileno(fp), 0L, SEEK_CUR)) == -1L || size == 0)
		return 0;
	return (unsigned)offset%(unsigned)size;
}

/* end of libc/stdio/setvbuf.c */
