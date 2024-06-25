// User-level IPC library routines

#include <env.h>
#include <lib.h>
#include <mmu.h>

// Send val to whom.  This function keeps trying until
// it succeeds.  It should panic() on any error other than
// -E_IPC_NOT_RECV.
//
// Hint: use syscall_yield() to be CPU-friendly.
void ipc_send(u_int whom, u_int val, const void *srcva, u_int perm) {
	int r;
	while ((r = syscall_ipc_try_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV) {
		syscall_yield();
	}
	user_assert(r == 0);
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.
//
// Hint: use env to discover the value and who sent it.
u_int ipc_recv(u_int *whom, void *dstva, u_int *perm) {
	int r = syscall_ipc_recv(dstva);
	if (r != 0) {
		user_panic("syscall_ipc_recv err: %d", r);
	}

	if (whom) {
		*whom = env->env_ipc_from;
	}

	if (perm) {
		*perm = env->env_ipc_perm;
	}

	return env->env_ipc_value;
}


// 下面是原本signal.c的函数
int sigaction(int signum, const struct sigaction *newact, struct sigaction *oldact)
{
    if (!isvalid(signum))
    {
        return -1;
    }

    if (syscall_get_sig_act(0, signum, oldact) != 0) {
        return -1;
    }

    // if (env_set_sig_entry() != 0) { // 为进程设置信号入口函数
    //     return -1;
    // }

    return syscall_set_sig_act(0, signum, newact);
}

int kill(u_int envid, int sig)
{
    if (!isvalid(sig)){
        return -1;
    }
    return syscall_kill(envid, sig);
}

int sigemptyset(sigset_t *__set)
{
    if (__set == NULL) {
        return -1;
    }
    __set->sig = 0;
    return 0;
}

int sigfillset(sigset_t *__set)
{
    if (__set == NULL) {
        return -1;
    }
    __set->sig = 0xFFFFFFFF;
    return 0;
}

int sigaddset(sigset_t *__set, int __signo)
{
    if (!isvalid(__signo)){
        return -1;
    }
    if (__set == NULL) {
        return -1;
    }
    __set->sig |= (1 << (__signo - 1));
    return 0;
}

int sigdelset(sigset_t *__set, int __signo)
{
    if (!isvalid(__signo)){
        return -1;
    }
    if (__set == NULL) {
        return -1;
    }
    __set->sig &= ~(1 << (__signo - 1));
    return 0;
}

int sigismember(const sigset_t *__set, int __signo)
{
    if (!isvalid(__signo)){
        return -1;
    }
    if (__set == NULL) {
        return -1;
    }
    return (__set->sig & (1 << (__signo - 1))) ? 1 : 0;
}

int sigisemptyset(const sigset_t *__set)
{
    if (__set == NULL) {
        return -1;
    }
    return (__set->sig == 0) ? 1 : 0;
}

// 计算两个信号集__left和__right的交集
int sigandset(sigset_t *__set, const sigset_t *__left, const sigset_t *__right) {
    if (__set == NULL || __left == NULL || __right == NULL) {
        return -1;
    }
    __set->sig = __left->sig & __right->sig;
    return 0;
}

// 计算两个信号集__left和__right的并集
int sigorset(sigset_t *__set, const sigset_t *__left, const sigset_t *__right) {
    if (__set == NULL || __left == NULL || __right == NULL) {
        return -1;
    }
    __set->sig = __left->sig | __right->sig;
    return 0;
}

int sigprocmask(int __how, const sigset_t *__set, sigset_t *__oset) {
    // if (__how != SIG_BLOCK && __how != SIG_UNBLOCK && __how != SIG_SETMASK) {
    //     return -1;
    // }
    return syscall_set_sig_set(0, __how, __set, __oset);
}

int sigpending(sigset_t *__set)
{
    if (__set == NULL) {
        return -1;
    }
    return syscall_get_sig_pend(0, __set);
}