#include <lib.h>

void printset(sigset_t *ped)
{
	int i;
	for(i = 1; i <= 32; i++){
		if((sigismember(ped, i) == 1)){
			debugf("1");
		} else {
			debugf("0");
		}
	}
	debugf("\n");
}

void sigint_handler(int sig) {
    debugf("capture SIGINT.\n");
}


int main(void)
{
	sigset_t set, oldset, pedset;
	
	sigemptyset(&set);
    sigemptyset(&pedset);
	// sigaddset(&set, SIGINT);//-2
	// sigaddset(&set, SIGQUIT);//-3
	sigaddset(&set, SIGKILL);//-9,注意该信号不会阻塞在屏蔽字，不能捕捉，忽略，所以该步骤是无效的。和编号19一样。
	sigaddset(&set, SIGSEGV);//11,即段错误。

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
	
	int ret = sigprocmask(SIG_BLOCK, &set, &oldset);
	if(ret != 0){
		debugf("sigprocmask failed.\n");
		return -1;
	}

    int a = 0;

    debugf("curenv 0x%x\n", env->env_sa_mask.sig);
	
	//为了方便观察现象加个循环,不断查看未决信号集
	while(1){
		//获取未决信号集
        kill(0, SIGINT);
        kill(0, SIGSEGV);
		ret = sigpending(&pedset);
        // debugf("set: 0x%x\n", pedset.sig);  
		if(ret != 0){
			debugf("sigpending failed.\n");
			return -1;
		}
		printset(&pedset);//打印
		// sleep(1);
        debugf("%d\n",a);
        if (a > 10) {
            kill(0, SIGKILL);
        }
        a++;
	}

	return 0;
}

