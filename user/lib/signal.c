#include <lib.h>

int sigaction(int signum, const struct sigaction *newact, struct sigaction *oldact)
{
    if (!isvalid(signum))
    {
        return -1;
    }

    if (syscall_get_sig_act(0, signum, oldact) != 0) {
        return -1;
    }

    return syscall_set_sig_act(0, signum, newact);
}

int kill(u_int envid, int sig)
{
    return syscall_kill(envid, sig);
}

int sigemptyset(sigset_t *__set)
{
    __set->sig = 0;
    return 0;
}

int sigfillset(sigset_t *__set)
{
    __set->sig = 0xFFFF;
    return 0;
}

int sigaddset(sigset_t *__set, int __signo)
{
    if (!isvalid(__signo)){
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
    __set->sig &= ~(1 << (__signo - 1));
    return 0;
}

int sigismember(const sigset_t *__set, int __signo)
{
    if (!isvalid(__signo)){
        return -1;
    }
    return (__set->sig & (1 << (__signo - 1))) ? 1 : 0;
}

int sigisemptyset(const sigset_t *__set)
{
    return (__set->sig == 0) ? 1 : 0;
}

// 计算两个信号集__left和__right的交集
int sigandset(sigset_t *__set, const sigset_t *__left, const sigset_t *__right) {
    __set->sig = __left->sig & __right->sig;
    return 0;
}

// 计算两个信号集__left和__right的并集
int sigorset(sigset_t *__set, const sigset_t *__left, const sigset_t *__right) {
    __set->sig = __left->sig | __right->sig;
    return 0;
}

int sigprocmask(int __how, const sigset_t *__set, sigset_t *__oset) {
    return syscall_set_sig_set(0, __how, __set, __oset);
}

int sigpending(sigset_t *__set)
{
    return 0;
}