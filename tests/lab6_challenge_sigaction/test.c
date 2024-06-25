#include <lib.h>
int a=0;
void sigint_handler(int sig) {
	debugf("capture SIGINT!!!\n");
    kill(0,SIGINT);
	kill(0,SIGINT);
	kill(0,SIGINT);
	kill(0,SIGINT);
    debugf("%d\n",a++);
    if(a>5){
        kill(0, SIGKILL);
		kill(0, SIGKILL);
		kill(0, SIGKILL);
		kill(0, SIGKILL);
    }
}

void sigkill_handler(int sig) {
	debugf("capture SIGKILL!!!\n");
	debugf("error: SIGKILL cannot be override !!!!\n");
}

int main() {
	debugf("Enter sigtst.\n");
    struct sigaction sa,sa2;
    sa.sa_handler = sigint_handler;
	sa2.sa_handler = sigkill_handler;
    sigemptyset(&sa.sa_mask);
	sigemptyset(&sa2.sa_mask);
    sigaction(SIGINT, &sa, NULL);
	sigaction(SIGKILL, &sa2, NULL);
	kill(0,SIGINT);
	while (1) {
		;
	}
	return 0;
}