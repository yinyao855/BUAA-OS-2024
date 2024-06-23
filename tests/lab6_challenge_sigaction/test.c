#include <lib.h>

int main()
{
    struct sigset_t tmp;
    tmp.sig = 10000;
    sigemptyset(&tmp);
    debugf("%u\n", tmp.sig);
    sigfillset(&tmp);
    debugf("%x\n", tmp.sig);
    sigemptyset(&tmp);
    debugf("%u\n", tmp.sig);
    sigaddset(&tmp, 2);
    debugf("%u\n", tmp.sig);
    sigdelset(&tmp, 2);
    debugf("%u\n", tmp.sig);
    debugf("%d %d\n", sigismember(&tmp, 2), tmp.sig);
    sigaddset(&tmp, 2);
    debugf("%u\n", tmp.sig);
    debugf("%d\n", sigismember(&tmp, 2));
    return 0;
}