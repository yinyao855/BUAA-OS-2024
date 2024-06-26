#include <bitops.h>
#include <env.h>
#include <pmap.h>

extern int sys_ukill(u_int, int);

/* Lab 2 Key Code "tlb_invalidate" */
/* Overview:
 *   Invalidate the TLB entry with specified 'asid' and virtual address 'va'.
 *
 * Hint:
 *   Construct a new Entry HI and call 'tlb_out' to flush TLB.
 *   'tlb_out' is defined in mm/tlb_asm.S
 */
void tlb_invalidate(u_int asid, u_long va) {
	tlb_out((va & ~GENMASK(PGSHIFT, 0)) | (asid & (NASID - 1)));
}
/* End of Key Code "tlb_invalidate" */

static void passive_alloc(u_int va, Pde *pgdir, u_int asid) {
	struct Page *p = NULL;

	if (va < UTEMP) {
		sys_ukill(0, SIGSEGV);
		// panic("address too low");
	}

	if (va >= USTACKTOP && va < USTACKTOP + PAGE_SIZE) {
		panic("invalid memory");
	}

	if (va >= UENVS && va < UPAGES) {
		panic("envs zone");
	}

	if (va >= UPAGES && va < UVPT) {
		panic("pages zone");
	}

	if (va >= ULIM) {
		panic("kernel address");
	}

	panic_on(page_alloc(&p));
	panic_on(page_insert(pgdir, asid, p, PTE_ADDR(va), (va >= UVPT && va < ULIM) ? 0 : PTE_D));
}

/* Overview:
 *  Refill TLB.
 */
void _do_tlb_refill(u_long *pentrylo, u_int va, u_int asid) {
	tlb_invalidate(asid, va);
	Pte *ppte;
	/* Hints:
	 *  Invoke 'page_lookup' repeatedly in a loop to find the page table entry '*ppte'
	 * associated with the virtual address 'va' in the current address space 'cur_pgdir'.
	 *
	 *  **While** 'page_lookup' returns 'NULL', indicating that the '*ppte' could not be found,
	 *  allocate a new page using 'passive_alloc' until 'page_lookup' succeeds.
	 */

	/* Exercise 2.9: Your code here. */
	while (page_lookup(cur_pgdir, va, &ppte) == NULL){
		passive_alloc(va, cur_pgdir, asid);
	}

	ppte = (Pte *)((u_long)ppte & ~0x7);
	pentrylo[0] = ppte[0] >> 6;
	pentrylo[1] = ppte[1] >> 6;
}

#if !defined(LAB) || LAB >= 4
/* Overview:
 *   This is the TLB Mod exception handler in kernel.
 *   Our kernel allows user programs to handle TLB Mod exception in user mode, so we copy its
 *   context 'tf' into UXSTACK and modify the EPC to the registered user exception entry.
 *
 * Hints:
 *   'env_user_tlb_mod_entry' is the user space entry registered using
 *   'sys_set_user_tlb_mod_entry'.
 *
 *   The user entry should handle this TLB Mod exception and restore the context.
 */
void do_tlb_mod(struct Trapframe *tf) {
	struct Trapframe tmp_tf = *tf;

	if (tf->regs[29] < USTACKTOP || tf->regs[29] >= UXSTACKTOP) {
		tf->regs[29] = UXSTACKTOP;
	}
	tf->regs[29] -= sizeof(struct Trapframe);
	*(struct Trapframe *)tf->regs[29] = tmp_tf;
	Pte *pte;
	page_lookup(cur_pgdir, tf->cp0_badvaddr, &pte);
	if (curenv->env_user_tlb_mod_entry) {
		tf->regs[4] = tf->regs[29];
		tf->regs[29] -= sizeof(tf->regs[4]);
		// Hint: Set 'cp0_epc' in the context 'tf' to 'curenv->env_user_tlb_mod_entry'.
		/* Exercise 4.11: Your code here. */
		tf->cp0_epc = curenv->env_user_tlb_mod_entry;
	} else {
		panic("TLB Mod but no user handler registered");
	}
}

void do_signal(struct Trapframe *tf) {
	// if ((tf->cp0_epc) > ULIM) {
    //     return;
    // }
	struct siglist *sig_list = &(curenv->env_sig_head);

	if (sig_list->next == NULL) {
		return;
	}
	// printk("[%08x] signal xx: %d\n", curenv->env_id, sig_list->next->sig);
	int sig = 0, r;
	r = getSig(sig_list, curenv->env_sa_mask, &sig);
	if (r == -1) {
		return;
	}

	// printk("do signal %d\n", sig);

	if (sig == SIGKILL) {
		env_destroy(curenv);
	}
	
	struct Trapframe tmp_tf = *tf;
	if (tf->regs[29] < USTACKTOP || tf->regs[29] >= UXSTACKTOP) {
		tf->regs[29] = UXSTACKTOP; // 将栈指针指向用户异常处理栈
	}
	tf->regs[29] -= sizeof(struct Trapframe); // 将当前的 Trapframe 压入异常处理栈
	*(struct Trapframe *)tf->regs[29] = tmp_tf;
	
	if (curenv->env_sig_entry) {
		tf->regs[4] = tf->regs[29];
		tf->regs[5] = (unsigned int)(curenv->env_handlers[sig]);
		tf->regs[6] = sig;
		tf->regs[7] = curenv->env_id;
		tf->regs[29] -= sizeof(tf->regs[4]);
		tf->regs[29] -= sizeof(tf->regs[5]);
		tf->regs[29] -= sizeof(tf->regs[6]);
		tf->regs[29] -= sizeof(tf->regs[7]);
        
		tf->cp0_epc = curenv->env_sig_entry;
	} else {
		panic("sig but no user handler registered");
	}
}
#endif
