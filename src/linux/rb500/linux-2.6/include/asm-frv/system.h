/* system.h: FR-V CPU control definitions
 *
 * Copyright (C) 2003 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef _ASM_SYSTEM_H
#define _ASM_SYSTEM_H

#include <linux/config.h> /* get configuration macros */
#include <linux/linkage.h>
#include <asm/atomic.h>

struct thread_struct;

/*
 * switch_to(prev, next) should switch from task `prev' to `next'
 * `prev' will never be the same as `next'.
 * The `mb' is to tell GCC not to cache `current' across this call.
 */
extern asmlinkage
struct task_struct *__switch_to(struct thread_struct *prev_thread,
				struct thread_struct *next_thread,
				struct task_struct *prev);

#define switch_to(prev, next, last)					\
do {									\
	(prev)->thread.sched_lr =					\
		(unsigned long) __builtin_return_address(0);		\
	(last) = __switch_to(&(prev)->thread, &(next)->thread, (prev));	\
	mb();								\
} while(0)

/*
 * interrupt flag manipulation
 * - use virtual interrupt management since touching the PSR is slow
 *   - ICC2.Z: T if interrupts virtually disabled
 *   - ICC2.C: F if interrupts really disabled
 * - if Z==1 upon interrupt:
 *   - C is set to 0
 *   - interrupts are really disabled
 *   - entry.S returns immediately
 * - uses TIHI (TRAP if Z==0 && C==0) #2 to really reenable interrupts
 *   - if taken, the trap:
 *     - sets ICC2.C
 *     - enables interrupts
 */
#define local_irq_disable()					\
do {								\
	/* set Z flag, but don't change the C flag */		\
	asm volatile("	andcc	gr0,gr0,gr0,icc2	\n"	\
		     :						\
		     :						\
		     : "memory", "icc2"				\
		     );						\
} while(0)

#define local_irq_enable()					\
do {								\
	/* clear Z flag and then test the C flag */		\
	asm volatile("  oricc	gr0,#1,gr0,icc2		\n"	\
		     "	tihi	icc2,gr0,#2		\n"	\
		     :						\
		     :						\
		     : "memory", "icc2"				\
		     );						\
} while(0)

#define local_save_flags(flags)					\
do {								\
	typecheck(unsigned long, flags);			\
	asm volatile("movsg ccr,%0"				\
		     : "=r"(flags)				\
		     :						\
		     : "memory");				\
								\
	/* shift ICC2.Z to bit 0 */				\
	flags >>= 26;						\
								\
	/* make flags 1 if interrupts disabled, 0 otherwise */	\
	flags &= 1UL;						\
} while(0)

#define irqs_disabled() \
	({unsigned long flags; local_save_flags(flags); flags; })

#define	local_irq_save(flags)			\
do {						\
	typecheck(unsigned long, flags);	\
	local_save_flags(flags);		\
	local_irq_disable();			\
} while(0)

#define	local_irq_restore(flags)					\
do {									\
	typecheck(unsigned long, flags);				\
									\
	/* load the Z flag by turning 1 if disabled into 0 if disabled	\
	 * and thus setting the Z flag but not the C flag */		\
	asm volatile("  xoricc	%0,#1,gr0,icc2		\n"		\
		     /* then test Z=0 and C=0 */			\
		     "	tihi	icc2,gr0,#2		\n"		\
		     :							\
		     : "r"(flags)					\
		     : "memory", "icc2"					\
		     );							\
									\
} while(0)

/*
 * real interrupt flag manipulation
 */
#define __local_irq_disable()				\
do {							\
	unsigned long psr;				\
	asm volatile("	movsg	psr,%0		\n"	\
		     "	andi	%0,%2,%0	\n"	\
		     "	ori	%0,%1,%0	\n"	\
		     "	movgs	%0,psr		\n"	\
		     : "=r"(psr)			\
		     : "i" (PSR_PIL_14), "i" (~PSR_PIL)	\
		     : "memory");			\
} while(0)

#define __local_irq_enable()				\
do {							\
	unsigned long psr;				\
	asm volatile("	movsg	psr,%0		\n"	\
		     "	andi	%0,%1,%0	\n"	\
		     "	movgs	%0,psr		\n"	\
		     : "=r"(psr)			\
		     : "i" (~PSR_PIL)			\
		     : "memory");			\
} while(0)

#define __local_save_flags(flags)		\
do {						\
	typecheck(unsigned long, flags);	\
	asm("movsg psr,%0"			\
	    : "=r"(flags)			\
	    :					\
	    : "memory");			\
} while(0)

#define	__local_irq_save(flags)				\
do {							\
	unsigned long npsr;				\
	typecheck(unsigned long, flags);		\
	asm volatile("	movsg	psr,%0		\n"	\
		     "	andi	%0,%3,%1	\n"	\
		     "	ori	%1,%2,%1	\n"	\
		     "	movgs	%1,psr		\n"	\
		     : "=r"(flags), "=r"(npsr)		\
		     : "i" (PSR_PIL_14), "i" (~PSR_PIL)	\
		     : "memory");			\
} while(0)

#define	__local_irq_restore(flags)			\
do {							\
	typecheck(unsigned long, flags);		\
	asm volatile("	movgs	%0,psr		\n"	\
		     :					\
		     : "r" (flags)			\
		     : "memory");			\
} while(0)

#define __irqs_disabled() \
	((__get_PSR() & PSR_PIL) >= PSR_PIL_14)

/*
 * Force strict CPU ordering.
 */
#define nop()			asm volatile ("nop"::)
#define mb()			asm volatile ("membar" : : :"memory")
#define rmb()			asm volatile ("membar" : : :"memory")
#define wmb()			asm volatile ("membar" : : :"memory")
#define set_mb(var, value)	do { var = value; mb(); } while (0)
#define set_wmb(var, value)	do { var = value; wmb(); } while (0)

#define smp_mb()		mb()
#define smp_rmb()		rmb()
#define smp_wmb()		wmb()

#define read_barrier_depends()		do {} while(0)
#define smp_read_barrier_depends()	read_barrier_depends()

#define HARD_RESET_NOW()			\
do {						\
	cli();					\
} while(1)

extern void die_if_kernel(const char *, ...) __attribute__((format(printf, 1, 2)));
extern void free_initmem(void);

#define arch_align_stack(x) (x)

#endif /* _ASM_SYSTEM_H */
