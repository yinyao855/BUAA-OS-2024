#include <lib.h>

void sigchld_handler(int sig) {
    debugf("capture SIGCHLD signal.\n");
    exit();
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);
    if (fork() == 0) {
        exit();
    }
    while (1);
   return 0;
}
