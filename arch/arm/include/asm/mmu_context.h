/*
 *  arch/arm/include/asm/mmu_context.h
 *
 *  Copyright (C) 1996 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Changelog:
 *   27-06-1996	RMK	Created
 */
#ifndef __ASM_ARM_MMU_CONTEXT_H
#define __ASM_ARM_MMU_CONTEXT_H

#include <linux/compiler.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
#include <asm/cachetype.h>
#include <asm/proc-fns.h>
#include <asm-generic/mm_hooks.h>

void __check_kvm_seq(struct mm_struct *mm);

#ifdef CONFIG_CPU_HAS_ASID

void check_and_switch_context(struct mm_struct *mm, struct task_struct *tsk);
#define init_new_context(tsk,mm)	({ atomic64_set(&mm->context.id, 0); 0; })

#ifdef CONFIG_ARM_ERRATA_798181
void a15_erratum_get_cpumask(int this_cpu, struct mm_struct *mm,
			     cpumask_t *mask);
#else  /* !CONFIG_ARM_ERRATA_798181 */
static inline void a15_erratum_get_cpumask(int this_cpu, struct mm_struct *mm,
					   cpumask_t *mask)
{
}
#endif /* CONFIG_ARM_ERRATA_798181 */

#else	/* !CONFIG_CPU_HAS_ASID */

static inline void check_and_switch_context(struct mm_struct *mm,
					    struct task_struct *tsk)
{
#ifdef CONFIG_MMU
	if (unlikely(mm->context.kvm_seq != init_mm.context.kvm_seq))
		__check_kvm_seq(mm);
	cpu_switch_mm(mm->pgd, mm);
#endif
}

#define init_new_context(tsk,mm)	0

#define finish_arch_post_lock_switch()	do { } while (0)

#endif	/* CONFIG_CPU_HAS_ASID */

#define destroy_context(mm)		do { } while(0)
#define activate_mm(prev,next)		switch_mm(prev, next, NULL)

/*
 * This is called when "tsk" is about to enter lazy TLB mode.
 *
 * mm:  describes the currently active mm context
 * tsk: task which is entering lazy tlb
 * cpu: cpu number which is entering lazy tlb
 *
 * tsk->mm will be NULL
 */
static inline void
enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk)
{
}

/*
 * This is the actual mm switch as far as the scheduler
 * is concerned.  No registers are touched.  We avoid
 * calling the CPU specific function when the mm hasn't
 * actually changed.
 */
static inline void
switch_mm(struct mm_struct *prev, struct mm_struct *next,
	  struct task_struct *tsk)
{
#ifdef CONFIG_MMU
	unsigned int cpu = smp_processor_id();

#ifdef CONFIG_SMP
	/* check for possible thread migration */
	if (!cpumask_empty(mm_cpumask(next)) &&
	    !cpumask_test_cpu(cpu, mm_cpumask(next)))
		__flush_icache_all();
#endif
	if (!cpumask_test_and_set_cpu(cpu, mm_cpumask(next)) || prev != next) {
		check_and_switch_context(next, tsk);
		if (cache_is_vivt())
			cpumask_clear_cpu(cpu, mm_cpumask(prev));
	}
#endif
}

#define deactivate_mm(tsk,mm)	do { } while (0)

#endif
