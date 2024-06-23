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
#define SIG2MASK(signum) (1 << (signum - 1))

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

struct siglist{
    int sig;
    struct siglist *next;
}; // 信号存储链表

int getSig(struct siglist *head, sigset_t sa_mask, int *sig);

#endif