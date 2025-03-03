#ifndef	__COMMON__SIGINFO_H__
#define	__COMMON__SIGINFO_H__

/*
 * This internal header file contains definitions related to the internal data
 * type "__siginfo_t", an internal equivalent to the System V, Release 4 data
 * type "siginfo_t", but with a private name to avoid accidental reservation
 * of user-space names. In addition, this internal version of the structure is
 * not padded to 128 bytes as the user-level version is. Otherwise, the form
 * of the "siginfo" structure exactly parallels the definition supplied in the
 * System V, Release 4 ABI.
 */

#include <common/__caddr.h>
#include <common/__clock.h>
#include <common/__pid.h>
#include <common/_uid.h>

enum {
	__ILL_ILLOPC	= 1,		/* Illegal opcode */
	__ILL_ILLOPN,			/* Illegal operand */
	__ILL_ILLADR,			/* Illegal addressing mode */
	__ILL_ILLTRAP,			/* Illegal trap */
	__ILL_PRVOPC,			/* Priveleged opcode */
	__ILL_PRVREG,			/* Priveleded register */
	__ILL_COPROC,			/* Coprocessor error */
	__ILL_BADSTK,			/* Internal stack error */

	__FPE_INTDIV	= 1,		/* Integer divide by zero */
	__FPE_INTOVF,			/* Integer overflow */
	__FPE_FLTDIV,			/* Floating-point divide by zero */
	__FPE_FLTOVF,			/* Floating-point overflow */
	__FPE_FLTUND,			/* Floating-point underflow */
	__FPE_FLTRES,			/* Floating-point inexact result */
	__FPE_FLTINV,			/* Invalid floating-point operation */
	__FPE_FLTSUB,			/* Subscript out of range */

	__SEGV_MAPERR	= 1,		/* Address not mapped to object */
	__SEGV_ACCERR,			/* Invalid permissions for object */

	__BUS_ADRALN	= 1,		/* Invalid address alignment */
	__BUS_ADRERR,			/* Non-existent physical address */
	__BUS_OBJERR,			/* Object-specific hardware error */

	__TRAP_BRKPT	= 1,		/* Process breakpoint */
	__TRAP_TRACE,			/* Process trace trap */

	__CLD_EXITED	= 1,		/* Child has exited */
	__CLD_KILLED,			/* Child was killed */
	__CLD_DUMPED,			/* Child terminated abnormally */
	__CLD_TRAPPED,			/* Traced child has trapped */
	__CLD_STOPPED,			/* Child has stopped */
	__CLD_CONTINUED,		/* Stopped child had continued */

	__POLL_IN	= 1,		/* Data input available */
	__POLL_OUT,			/* Output buffers available */
	__POLL_MSG,			/* Input message available */
	__POLL_ERR,			/* I/O error */
	__POLL_PRI,			/* High-priority input available */
	__POLL_HUP			/* Device disconnected */
};

typedef struct __siginfo	__siginfo_t;

struct __siginfo {
	int		__si_signo;
	int		__si_code;
	int		__si_errno;
	union {
		struct {
			__pid_t		_pid;
			union {
				struct {
					n_uid_t		_uid;
				} _kill;
				struct {
					__clock_t	_utime;
					int		_status;
					__clock_t	_stime;
				} _cld;
			} _pdata;
		} _proc;
		struct {
			__caddr_t	_addr;
		} _fault;
		struct {
			int		_fd;
			long		_band;
		} _file;
	} _data;
};

#define	__si_pid	_data._proc._pid
#define	__si_status	_data._proc._pdata._cld._status
#define	__si_stime	_data._proc._pdata._cld._stime
#define	__si_utime	_data._proc._pdata._cld._utime
#define	__si_uid	_data._proc._pdata._kill._uid
#define	__si_addr	_data._fault._addr
#define	__si_fd		_data._file._fd
#define	__si_band	_data._file._band

#endif	/* ! defined (__COMMON__SIGINFO_H__) */

