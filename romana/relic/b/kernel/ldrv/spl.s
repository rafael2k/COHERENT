/ $Header: /kernel/kersrc/ldrv/RCS/spl.s,v 1.1 92/07/17 15:28:09 bin Exp Locker: bin $
/
/	The  information  contained herein  is a trade secret  of INETCO
/	Systems, and is confidential information.   It is provided under
/	a license agreement,  and may be copied or disclosed  only under
/	the terms of that agreement.   Any reproduction or disclosure of
/	this  material  without  the express  written  authorization  of
/	INETCO Systems or persuant to the license agreement is unlawful.
/
/	Copyright (c) 1988
/	An unpublished work by INETCO Systems, Ltd.
/	All rights reserved.
/
/ $Log:	spl.s,v $
/ Revision 1.1  92/07/17  15:28:09  bin
/ Initial revision
/
/ Revision 1.1	88/03/24  16:31:16	src
/ Initial revision
/ 
/
////////

////////
/
/ Change interrupt flag.  Previous value is returned.
/
////////
	.globl	spl_

spl_:
	pop	ax			/ ip
	pop	bx			/ psw
	push	bx
	push	bx			/ push psw, cs, ip for iret
	push	cs
	push	ax
	pushf				/ old psw
	pop	ax
	iret
