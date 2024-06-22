#ifndef SIG_H
#define SIG_H

#include <types.h>

#define SIGINT 2 //中断信号
#define SIGILL 4 //非法指令
#define SIGKILL	9 //停止进程信号
#define SIGSEGV	11 //访问地址错误
#define SIGCHLD	17 //子进程终止信号
#define SIGSYS 31 //系统调用号未定义

#define isvalid(signum) ((signum >= 1 && signum <= 32) ? 1 : 0)

enum {
    SIG_BLOCK,
    SIG_UNBLOCK,
    SIG_SETMASK
};

typedef struct sigset_t {
    uint32_t sig;
} sigset_t;

struct sigaction {
    void     (*sa_handler)(int); // 信号处理函数指针
    sigset_t   sa_mask; // 信号屏蔽集合
};

struct sigstack{
    int sig;
    struct sigstack *next;
}; // 信号存储链表

// 信号注册函数
int sigaction(int signum, const struct sigaction *newact, struct sigaction *oldact);

// 信号发送函数
int kill(u_int envid, int sig);

int sigemptyset(sigset_t *__set);
// 清空参数中的__set掩码，初始化信号集以排除所有信号。这意味着__set将不包含任何信号。(清0)

int sigfillset(sigset_t *__set);
// 将参数中的__set掩码填满，使其包含所有已定义的信号。这意味着__set将包括所有信号。(全为1)

int sigaddset(sigset_t *__set, int __signo);
// 向__set信号集中添加一个信号__signo。如果操作成功，__set将包含该信号。(置位为1)

int sigdelset(sigset_t *__set, int __signo);
// 从__set信号集中删除一个信号__signo。如果操作成功，__set将不再包含该信号。(置位为0)

int sigismember(const sigset_t *__set, int __signo);
// 检查信号__signo是否是__set信号集的成员。如果是，返回1；如果不是，返回0。

int sigisemptyset(const sigset_t *__set);
// 检查信号集__set是否为空。如果为空，返回1；如果不为空，返回0。

int sigandset(sigset_t *__set, const sigset_t *__left, const sigset_t *__right);
// 计算两个信号集__left和__right的交集，并将结果存储在__set中。

int sigorset(sigset_t *__set, const sigset_t *__left, const sigset_t *__right);
// 计算两个信号集__left和__right的并集，并将结果存储在__set中。

int sigprocmask(int __how, const sigset_t * __set, sigset_t * __oset);
// 根据__how的值更改当前进程的信号屏蔽字。__set是要应用的新掩码，__oset（如果非NULL）则保存旧的信号屏蔽字。__how可以是SIG_BLOCK（添加__set到当前掩码）、SIG_UNBLOCK（从当前掩码中移除__set）、或SIG_SETMASK（设置当前掩码为__set）。

int sigpending(sigset_t *__set);
// 获取当前被阻塞且未处理的信号集，并将其存储在__set中。

#endif