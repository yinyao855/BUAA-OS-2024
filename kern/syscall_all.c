#include <env.h>
#include <io.h>
#include <mmu.h>
#include <pmap.h>
#include <printk.h>
#include <sched.h>
#include <sig.h>
#include <syscall.h>

extern struct Env *curenv;
static struct siglist SIGlist[4096];

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int c) {
	printcharc((char)c);
	return;
}

/* Overview:
 * 	This function is used to print a string of bytes on screen.
 *
 * Pre-Condition:
 * 	`s` is base address of the string, and `num` is length of the string.
 */
int sys_print_cons(const void *s, u_int num) {
	if (((u_int)s + num) > UTOP || ((u_int)s) >= UTOP || (s > s + num)) {
		return -E_INVAL;
	}
	u_int i;
	for (i = 0; i < num; i++) {
		printcharc(((char *)s)[i]);
	}
	return 0;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void) {
	return curenv->env_id;
}

/* Overview:
 *   Give up remaining CPU time slice for 'curenv'.
 *
 * Post-Condition:
 *   Another env is scheduled.
 *
 * Hint:
 *   This function will never return.
 */
// void sys_yield(void);
void __attribute__((noreturn)) sys_yield(void) {
	// Hint: Just use 'schedule' with 'yield' set.
	/* Exercise 4.7: Your code here. */
	schedule(1);
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *
 * Post-Condition:
 *  Returns 0 on success.
 *  Returns the original error if underlying calls fail.
 */
int sys_env_destroy(u_int envid) {
	struct Env *e;
	try(envid2env(envid, &e, 1));

	if (e->env_parent_id != 0) {
		sys_ukill(e->env_parent_id, SIGCHLD);
	}
	
	printk("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 *   Register the entry of user space TLB Mod handler of 'envid'.
 *
 * Post-Condition:
 *   The 'envid''s TLB Mod exception handler entry will be set to 'func'.
 *   Returns 0 on success.
 *   Returns the original error if underlying calls fail.
 */
int sys_set_tlb_mod_entry(u_int envid, u_int func) {
	struct Env *env;

	/* Step 1: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.12: Your code here. (1/2) */
	try(envid2env(envid, &env, 1));

	/* Step 2: Set its 'env_user_tlb_mod_entry' to 'func'. */
	/* Exercise 4.12: Your code here. (2/2) */
	env->env_user_tlb_mod_entry = func;

	return 0;
}

/* Overview:
 *   Check 'va' is illegal or not, according to include/mmu.h
 */
static inline int is_illegal_va(u_long va) {
	return va < UTEMP || va >= UTOP;
}

static inline int is_illegal_va_range(u_long va, u_int len) {
	if (len == 0) {
		return 0;
	}
	return va + len < va || va < UTEMP || va + len > UTOP;
}

/* Overview:
 *   Allocate a physical page and map 'va' to it with 'perm' in the address space of 'envid'.
 *   If 'va' is already mapped, that original page is sliently unmapped.
 *   'envid2env' should be used with 'checkperm' set, like in most syscalls, to ensure the target is
 * either the caller or its child.
 *
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_BAD_ENV: 'checkperm' of 'envid2env' fails for 'envid'.
 *   Return -E_INVAL:   'va' is illegal (should be checked using 'is_illegal_va').
 *   Return the original error: underlying calls fail (you can use 'try' macro).
 *
 * Hint:
 *   You may want to use the following functions:
 *   'envid2env', 'page_alloc', 'page_insert', 'try' (macro)
 */
int sys_mem_alloc(u_int envid, u_int va, u_int perm) {
	struct Env *env;
	struct Page *pp;

	/* Step 1: Check if 'va' is a legal user virtual address using 'is_illegal_va'. */
	/* Exercise 4.4: Your code here. (1/3) */
	if (is_illegal_va(va)) {
		return -E_INVAL;
	}

	/* Step 2: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* Hint: **Always** validate the permission in syscalls! */
	/* Exercise 4.4: Your code here. (2/3) */
	try(envid2env(envid, &env, 1));

	/* Step 3: Allocate a physical page using 'page_alloc'. */
	/* Exercise 4.4: Your code here. (3/3) */
	try(page_alloc(&pp));

	/* Step 4: Map the allocated page at 'va' with permission 'perm' using 'page_insert'. */
	return page_insert(env->env_pgdir, env->env_asid, pp, va, perm);
}

/* Overview:
 *   Find the physical page mapped at 'srcva' in the address space of env 'srcid', and map 'dstid''s
 *   'dstva' to it with 'perm'.
 *
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_BAD_ENV: 'checkperm' of 'envid2env' fails for 'srcid' or 'dstid'.
 *   Return -E_INVAL: 'srcva' or 'dstva' is illegal, or 'srcva' is unmapped in 'srcid'.
 *   Return the original error: underlying calls fail.
 *
 * Hint:
 *   You may want to use the following functions:
 *   'envid2env', 'page_lookup', 'page_insert'
 */
int sys_mem_map(u_int srcid, u_int srcva, u_int dstid, u_int dstva, u_int perm) {
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *pp;

	/* Step 1: Check if 'srcva' and 'dstva' are legal user virtual addresses using
	 * 'is_illegal_va'. */
	/* Exercise 4.5: Your code here. (1/4) */
	if (is_illegal_va(srcva) || is_illegal_va(dstva)) {
		return -E_INVAL;
	}

	/* Step 2: Convert the 'srcid' to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.5: Your code here. (2/4) */
	try(envid2env(srcid, &srcenv, 1));

	/* Step 3: Convert the 'dstid' to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.5: Your code here. (3/4) */
	try(envid2env(dstid, &dstenv, 1));

	/* Step 4: Find the physical page mapped at 'srcva' in the address space of 'srcid'. */
	/* Return -E_INVAL if 'srcva' is not mapped. */
	/* Exercise 4.5: Your code here. (4/4) */
	pp = page_lookup(srcenv->env_pgdir, srcva, NULL);
	if (pp == NULL) {
		return -E_INVAL;
	}

	/* Step 5: Map the physical page at 'dstva' in the address space of 'dstid'. */
	return page_insert(dstenv->env_pgdir, dstenv->env_asid, pp, dstva, perm);
}

/* Overview:
 *   Unmap the physical page mapped at 'va' in the address space of 'envid'.
 *   If no physical page is mapped there, this function silently succeeds.
 *
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_BAD_ENV: 'checkperm' of 'envid2env' fails for 'envid'.
 *   Return -E_INVAL:   'va' is illegal.
 *   Return the original error when underlying calls fail.
 */
int sys_mem_unmap(u_int envid, u_int va) {
	struct Env *e;

	/* Step 1: Check if 'va' is a legal user virtual address using 'is_illegal_va'. */
	/* Exercise 4.6: Your code here. (1/2) */
	if (is_illegal_va(va)) {
		return -E_INVAL;
	}

	/* Step 2: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.6: Your code here. (2/2) */
	try(envid2env(envid, &e, 1));

	/* Step 3: Unmap the physical page at 'va' in the address space of 'envid'. */
	page_remove(e->env_pgdir, e->env_asid, va);
	return 0;
}

/* Overview:
 *   Allocate a new env as a child of 'curenv'.
 *
 * Post-Condition:
 *   Returns the child's envid on success, and
 *   - The new env's 'env_tf' is copied from the kernel stack, except for $v0 set to 0 to indicate
 *     the return value in child.
 *   - The new env's 'env_status' is set to 'ENV_NOT_RUNNABLE'.
 *   - The new env's 'env_pri' is copied from 'curenv'.
 *   Returns the original error if underlying calls fail.
 *
 * Hint:
 *   This syscall works as an essential step in user-space 'fork' and 'spawn'.
 */
int sys_exofork(void) {
	struct Env *e;

	/* Step 1: Allocate a new env using 'env_alloc'. */
	/* Exercise 4.9: Your code here. (1/4) */
	try(env_alloc(&e, curenv->env_id));

	/* Step 2: Copy the current Trapframe below 'KSTACKTOP' to the new env's 'env_tf'. */
	/* Exercise 4.9: Your code here. (2/4) */
	e->env_tf = *((struct Trapframe *)KSTACKTOP - 1);

	/* Step 3: Set the new env's 'env_tf.regs[2]' to 0 to indicate the return value in child. */
	/* Exercise 4.9: Your code here. (3/4) */
	e->env_tf.regs[2] = 0;

	/* Step 4: Set up the new env's 'env_status' and 'env_pri'.  */
	/* Exercise 4.9: Your code here. (4/4) */
	e->env_status = ENV_NOT_RUNNABLE;
	e->env_pri = curenv->env_pri;

	for (u_int i = 1; i <= 32; i++) // 子进程继承父进程信号屏蔽集和处理函数
	{
		e->env_handlers[i] = curenv->env_handlers[i];
		e->env_sa_mask_list[i] = curenv->env_sa_mask_list[i];
	}
	e->env_sa_mask = curenv->env_sa_mask;
	e->env_sig_head = curenv->env_sig_head;

	return e->env_id;
}

/* Overview:
 *   Set 'envid''s 'env_status' to 'status' and update 'env_sched_list'.
 *
 * Post-Condition:
 *   Returns 0 on success.
 *   Returns -E_INVAL if 'status' is neither 'ENV_RUNNABLE' nor 'ENV_NOT_RUNNABLE'.
 *   Returns the original error if underlying calls fail.
 *
 * Hint:
 *   The invariant that 'env_sched_list' contains and only contains all runnable envs should be
 *   maintained.
 */
int sys_set_env_status(u_int envid, u_int status) {
	struct Env *env;

	/* Step 1: Check if 'status' is valid. */
	/* Exercise 4.14: Your code here. (1/3) */
	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE) {
		return -E_INVAL;
	}

	/* Step 2: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.14: Your code here. (2/3) */
	try(envid2env(envid, &env, 1));

	/* Step 3: Update 'env_sched_list' if the 'env_status' of 'env' is being changed. */
	/* Exercise 4.14: Your code here. (3/3) */
	if (env->env_status != ENV_NOT_RUNNABLE && status == ENV_NOT_RUNNABLE) {
		TAILQ_REMOVE(&env_sched_list, env, env_sched_link);
	}
	else if (env->env_status != ENV_RUNNABLE && status == ENV_RUNNABLE) {
		TAILQ_INSERT_TAIL(&env_sched_list, env, env_sched_link);
	}

	/* Step 4: Set the 'env_status' of 'env'. */
	env->env_status = status;
	return 0;
}

/* Overview:
 *  Set envid's trap frame to 'tf'.
 *
 * Post-Condition:
 *  The target env's context is set to 'tf'.
 *  Returns 0 on success (except when the 'envid' is the current env, so no value could be
 * returned).
 *  Returns -E_INVAL if the environment cannot be manipulated or 'tf' is invalid.
 *  Returns the original error if other underlying calls fail.
 */
int sys_set_trapframe(u_int envid, struct Trapframe *tf) {
	if (is_illegal_va_range((u_long)tf, sizeof *tf)) {
		return -E_INVAL;
	}
	struct Env *env;
	try(envid2env(envid, &env, 1));
	if (env == curenv) {
		*((struct Trapframe *)KSTACKTOP - 1) = *tf;
		// return `tf->regs[2]` instead of 0, because return value overrides regs[2] on
		// current trapframe.
		return tf->regs[2];
	} else {
		env->env_tf = *tf;
		return 0;
	}
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Post-Condition:
 * 	This function will halt the system.
 */
void sys_panic(char *msg) {
	panic("%s", TRUP(msg));
}

/* Overview:
 *   Wait for a message (a value, together with a page if 'dstva' is not 0) from other envs.
 *   'curenv' is blocked until a message is sent.
 *
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_INVAL: 'dstva' is neither 0 nor a legal address.
 */
int sys_ipc_recv(u_int dstva) {
	/* Step 1: Check if 'dstva' is either zero or a legal address. */
	if (dstva != 0 && is_illegal_va(dstva)) {
		return -E_INVAL;
	}

	/* Step 2: Set 'curenv->env_ipc_recving' to 1. */
	/* Exercise 4.8: Your code here. (1/8) */
	curenv->env_ipc_recving = 1;

	/* Step 3: Set the value of 'curenv->env_ipc_dstva'. */
	/* Exercise 4.8: Your code here. (2/8) */
	curenv->env_ipc_dstva = dstva;

	/* Step 4: Set the status of 'curenv' to 'ENV_NOT_RUNNABLE' and remove it from
	 * 'env_sched_list'. */
	/* Exercise 4.8: Your code here. (3/8) */
	curenv->env_status = ENV_NOT_RUNNABLE;
	TAILQ_REMOVE(&env_sched_list, curenv, env_sched_link);

	/* Step 5: Give up the CPU and block until a message is received. */
	((struct Trapframe *)KSTACKTOP - 1)->regs[2] = 0;
	schedule(1);
}

/* Overview:
 *   Try to send a 'value' (together with a page if 'srcva' is not 0) to the target env 'envid'.
 *
 * Post-Condition:
 *   Return 0 on success, and the target env is updated as follows:
 *   - 'env_ipc_recving' is set to 0 to block future sends.
 *   - 'env_ipc_from' is set to the sender's envid.
 *   - 'env_ipc_value' is set to the 'value'.
 *   - 'env_status' is set to 'ENV_RUNNABLE' again to recover from 'ipc_recv'.
 *   - if 'srcva' is not NULL, map 'env_ipc_dstva' to the same page mapped at 'srcva' in 'curenv'
 *     with 'perm'.
 *
 *   Return -E_IPC_NOT_RECV if the target has not been waiting for an IPC message with
 *   'sys_ipc_recv'.
 *   Return the original error when underlying calls fail.
 */
int sys_ipc_try_send(u_int envid, u_int value, u_int srcva, u_int perm) {
	struct Env *e;
	struct Page *p;

	/* Step 1: Check if 'srcva' is either zero or a legal address. */
	/* Exercise 4.8: Your code here. (4/8) */
	if (srcva != 0 && is_illegal_va(srcva)) {
		return -E_INVAL;
	}

	/* Step 2: Convert 'envid' to 'struct Env *e'. */
	/* This is the only syscall where the 'envid2env' should be used with 'checkperm' UNSET,
	 * because the target env is not restricted to 'curenv''s children. */
	/* Exercise 4.8: Your code here. (5/8) */
	try(envid2env(envid, &e, 0));

	/* Step 3: Check if the target is waiting for a message. */
	/* Exercise 4.8: Your code here. (6/8) */
	if (e->env_ipc_recving == 0) {
		return -E_IPC_NOT_RECV;
	}

	/* Step 4: Set the target's ipc fields. */
	e->env_ipc_value = value;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_perm = PTE_V | perm;
	e->env_ipc_recving = 0;

	/* Step 5: Set the target's status to 'ENV_RUNNABLE' again and insert it to the tail of
	 * 'env_sched_list'. */
	/* Exercise 4.8: Your code here. (7/8) */
	e->env_status = ENV_RUNNABLE;
	TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);

	/* Step 6: If 'srcva' is not zero, map the page at 'srcva' in 'curenv' to 'e->env_ipc_dstva'
	 * in 'e'. */
	/* Return -E_INVAL if 'srcva' is not zero and not mapped in 'curenv'. */
	if (srcva != 0) {
		/* Exercise 4.8: Your code here. (8/8) */
		p = page_lookup(curenv->env_pgdir, srcva, NULL);
		if (p == NULL) {
			return -E_INVAL;
		}
		try(page_insert(e->env_pgdir, e->env_asid, p, e->env_ipc_dstva, perm));
	}
	return 0;
}

// XXX: kernel does busy waiting here, blocking all envs
int sys_cgetc(void) {
	int ch;
	while ((ch = scancharc()) == 0) {
	}
	return ch;
}

/* Overview:
 *  This function is used to write data at 'va' with length 'len' to a device physical address
 *  'pa'. Remember to check the validity of 'va' and 'pa' (see Hint below);
 *
 *  'va' is the starting address of source data, 'len' is the
 *  length of data (in bytes), 'pa' is the physical address of
 *  the device (maybe with a offset).
 *
 * Pre-Condition:
 *  'len' must be 1, 2 or 4, otherwise return -E_INVAL.
 *
 * Post-Condition:
 *  Data within [va, va+len) is copied to the physical address 'pa'.
 *  Return 0 on success.
 *  Return -E_INVAL on bad address.
 *
 * Hint:
 *  You can use 'is_illegal_va_range' to validate 'va'.
 *  You may use the unmapped and uncached segment in kernel address space (KSEG1)
 *  to perform MMIO by assigning a corresponding-lengthed data to the address,
 *  or you can just simply use the io function defined in 'include/io.h',
 *  such as 'iowrite32', 'iowrite16' and 'iowrite8'.
 *
 *  All valid device and their physical address ranges:
 *	* ---------------------------------*
 *	|   device   | start addr | length |
 *	* -----------+------------+--------*
 *	|  console   | 0x180003f8 | 0x20   |
 *	|  IDE disk  | 0x180001f0 | 0x8    |
 *	* ---------------------------------*
 */
int sys_write_dev(u_int va, u_int pa, u_int len) {
	/* Exercise 5.1: Your code here. (1/2) */
	if (is_illegal_va_range(va, len)){
		return -E_INVAL;
	}

	if (len != 1 && len !=2 && len !=4){
		return -E_INVAL;
	}
	
	if ((0x180003f8 <= pa && pa + len <= 0x18000418) ||
        (0x180001f0 <= pa && pa + len <= 0x180001f8)) {
        memcpy((void *)(pa | KSEG1), (void *)va, len);
        return 0;
    }

	return -E_INVAL;
}

/* Overview:
 *  This function is used to read data from a device physical address.
 *
 * Pre-Condition:
 *  'len' must be 1, 2 or 4, otherwise return -E_INVAL.
 *
 * Post-Condition:
 *  Data at 'pa' is copied from device to [va, va+len).
 *  Return 0 on success.
 *  Return -E_INVAL on bad address.
 *
 * Hint:
 *  You can use 'is_illegal_va_range' to validate 'va'.
 *  You can use function 'ioread32', 'ioread16' and 'ioread8' to read data from device.
 */
int sys_read_dev(u_int va, u_int pa, u_int len) {
	/* Exercise 5.1: Your code here. (2/2) */
	if (is_illegal_va_range(va, len)){
		return -E_INVAL;
	}

	if (len != 1 && len !=2 && len !=4){
		return -E_INVAL;
	}
	
	if ((0x180003f8 <= pa && pa + len <= 0x18000418) ||
        (0x180001f0 <= pa && pa + len <= 0x180001f8)) {
        memcpy((void *)va, (void *)(pa | KSEG1), len);
        return 0;
    }

	return -E_INVAL;
}

int sys_set_sig_trapframe(u_int envid, struct Trapframe *tf) {
	if (is_illegal_va_range((u_long)tf, sizeof *tf)) {
		return -E_INVAL;
	}
	struct Env *env;
	try(envid2env(envid, &env, 1));
	// env->env_sig_flag = 0;
	if (env == curenv) {
		*((struct Trapframe *)KSTACKTOP - 1) = *tf;
		return tf->regs[2];
	} else {
		env->env_tf = *tf;
		return 0;
	}
}

int sys_set_sig_entry(u_int envid, u_int func) {
	struct Env *env;
	try(envid2env(envid, &env, 1));

	env->env_sig_entry = func;
	return 0;
}

int sys_get_sig_act(u_int envid, int signum, struct sigaction *oldact) {
	struct Env *e;
	if (!isvalid(signum)){
        return -1;
    }

	try(envid2env(envid, &e, 1));

	if (oldact != NULL) {
		oldact->sa_handler = (void *)e->env_handlers[signum];
		oldact->sa_mask = e->env_sa_mask_list[signum];
	}
	return 0;
}

// 设置信号处理函数
int sys_set_sig_act(u_int envid, int signum, struct sigaction *act) {
	struct Env *e;
	if (!isvalid(signum)){
        return -1;
    }

	try(envid2env(envid, &e, 1));

	if (signum != SIGKILL) {
		e->env_handlers[signum] = (u_int)act->sa_handler;
	}
	e->env_sa_mask_list[signum].sig = act->sa_mask.sig & (~SIG2MASK(SIGKILL));

	// printk("env's signal: %d, handler: %d, sa mask: %d\n", signum, e->env_handlers[signum], e->env_sa_mask);
	return 0;
}

int sys_set_sig_set(u_int envid, int how, sigset_t *set, sigset_t *oldset) {
	struct Env *env;
    try(envid2env(envid, &env, 1));
	if (oldset != NULL) { 
    //当oldset不为NULL时，还需将原有的信号掩码放在oldset指定的地址空间中
        oldset->sig = env->env_sa_mask.sig;
    }
	if (set == NULL) {
		return 0;
	}
	switch (how) {
    	case SIG_BLOCK: //将set参数中指定的信号添加到当前进程的信号掩码中
			env->env_sa_mask.sig |= set->sig;
        	break;
   		case SIG_UNBLOCK: //将set参数中指定的信号从当前进程的信号掩码中删除
        	env->env_sa_mask.sig &= (~set->sig);
        	break;
    	case SIG_SETMASK: //将当前进程的信号掩码设置为set参数中指定的信号集
         	env->env_sa_mask.sig = set->sig;
        	break;    
    	default:
        	break;
    }
	env->env_sa_mask.sig &= ~(SIG2MASK(SIGKILL));
	return 0;
}

int sys_ukill(u_int envid, int sig) {
	struct Env *env;
	if (!isvalid(sig)) {
        return -1;
    }

	// 加入信号
	try(envid2env(envid, &env, 0));
	// printk("[%08x] receive signal: %d\n", env->env_id, sig);
	int i;
	struct siglist *p = &(env->env_sig_head);
	struct siglist *q = p->next;
	
	for (i = 0; i < 4096; i++)
	{
		if (SIGlist[i].sig == 0) { // 代表还没有分配
			SIGlist[i].sig = sig;
			SIGlist[i].next = NULL;
			break;
		}
	}
	// printk("%d allocate %d\n", SIGlist[i].sig, i);
	// 处理空链表的情况
    if (p->next == NULL) {
        p->next = &(SIGlist[i]);
        return 0;
    }
	// printk("not empty! %x %d %x\n", p->next,p->next->sig, p->next->next);
	while (q != NULL && sig > q->sig) {
		p = q;
        q = q->next;
    }
	if (q != NULL && sig == q->sig) {
		// 说明信号已经存在
		SIGlist[i].sig = 0;
		return 0;
	}
    SIGlist[i].next = q;
    p->next = &SIGlist[i];

	// if (sig == SIGKILL) {
	// 	env->env_sig_head.next = &SIGlist[i];
	// 	SIGlist[i].next = NULL;
	// }
	
	return 0;
}

int getSig(struct siglist *head, sigset_t sa_mask, int *sig) {
	// 从信号链表中找到第一个满足条件的信号
	if (head->next == NULL) {
		return -1;
	}

	struct siglist *p = head, *q = head->next;
	int flag = 0;
	while (q != NULL) 
	{
		if (q->sig == SIGKILL) {
			flag = 1;
			break;
		}
		p = q;
		q = q->next;
	}
	if (flag) {
		*sig = q->sig;
		p->next = q->next;
		q->sig = 0;
		q->next = NULL;
		head->next = NULL; // 清空剩余信号
		return 0;
	}
	flag = 0;
	p = head, q = head->next;
	while (q != NULL)
	{
		if (((1 << (q->sig -1)) & sa_mask.sig) == 0) {
			flag = 1;
			break;
		}
		p = q;
		q = q->next;
	}
	if (flag) {
		*sig = q->sig;
		// printk("find signal %d\n", *sig);
		// 将信号从信号链表中移除
		p->next = q->next;
		q->sig = 0;
		q->next = NULL;
		return 0;
	}
	return -1;
}

// 获取被阻塞且未处理的信号集
int sys_get_sig_pend(u_int envid, sigset_t *set) {
	struct Env *env;
	try(envid2env(envid, &env, 0));

	struct siglist *p = &(env->env_sig_head);
	struct siglist *q = p->next;
	sigset_t sa_mask = env->env_sa_mask;
	u_int tmp = 0;

	while (q != NULL)
	{
		if ((SIG2MASK(q->sig) & sa_mask.sig) != 0) {
			tmp |= SIG2MASK(q->sig);
		}
		p = q;
		q = q->next;
	}
	set->sig = tmp;
	return 0;
}

void *syscall_table[MAX_SYSNO] = {
    [SYS_putchar] = sys_putchar,
    [SYS_print_cons] = sys_print_cons,
    [SYS_getenvid] = sys_getenvid,
    [SYS_yield] = sys_yield,
    [SYS_env_destroy] = sys_env_destroy,
    [SYS_set_tlb_mod_entry] = sys_set_tlb_mod_entry,
    [SYS_mem_alloc] = sys_mem_alloc,
    [SYS_mem_map] = sys_mem_map,
    [SYS_mem_unmap] = sys_mem_unmap,
    [SYS_exofork] = sys_exofork,
    [SYS_set_env_status] = sys_set_env_status,
    [SYS_set_trapframe] = sys_set_trapframe,
    [SYS_panic] = sys_panic,
    [SYS_ipc_try_send] = sys_ipc_try_send,
    [SYS_ipc_recv] = sys_ipc_recv,
    [SYS_cgetc] = sys_cgetc,
    [SYS_write_dev] = sys_write_dev,
    [SYS_read_dev] = sys_read_dev,
	[SYS_set_sig_trapframe] = sys_set_sig_trapframe,
	[SYS_set_sig_entry] = sys_set_sig_entry,
	[SYS_get_sig_act] = sys_get_sig_act,
	[SYS_set_sig_act] = sys_set_sig_act,
	[SYS_set_sig_set] = sys_set_sig_set,
	[SYS_ukill] = sys_ukill,
	[SYS_get_sig_pend] = sys_get_sig_pend,
};

/* Overview:
 *   Call the function in 'syscall_table' indexed at 'sysno' with arguments from user context and
 * stack.
 *
 * Hint:
 *   Use sysno from $a0 to dispatch the syscall.
 *   The possible arguments are stored at $a1, $a2, $a3, [$sp + 16 bytes], [$sp + 20 bytes] in
 *   order.
 *   Number of arguments cannot exceed 5.
 */
void do_syscall(struct Trapframe *tf) {
	int (*func)(u_int, u_int, u_int, u_int, u_int);
	int sysno = tf->regs[4];
	if (sysno < 0 || sysno >= MAX_SYSNO) {
		tf->regs[2] = -E_NO_SYS;
		tf->cp0_epc+=4;
		sys_ukill(0, SIGSYS);
		return;
	}

	/* Step 1: Add the EPC in 'tf' by a word (size of an instruction). */
	/* Exercise 4.2: Your code here. (1/4) */
	tf->cp0_epc += 4;

	/* Step 2: Use 'sysno' to get 'func' from 'syscall_table'. */
	/* Exercise 4.2: Your code here. (2/4) */
	func = syscall_table[sysno];

	/* Step 3: First 3 args are stored in $a1, $a2, $a3. */
	u_int arg1 = tf->regs[5];
	u_int arg2 = tf->regs[6];
	u_int arg3 = tf->regs[7];

	/* Step 4: Last 2 args are stored in stack at [$sp + 16 bytes], [$sp + 20 bytes]. */
	u_int arg4, arg5;
	/* Exercise 4.2: Your code here. (3/4) */
	arg4 = *(u_int *)(tf->regs[29] + 16);
	arg5 = *(u_int *)(tf->regs[29] + 20);

	/* Step 5: Invoke 'func' with retrieved arguments and store its return value to $v0 in 'tf'.
	 */
	/* Exercise 4.2: Your code here. (4/4) */
	tf->regs[2] = func(arg1, arg2, arg3, arg4, arg5);
}

