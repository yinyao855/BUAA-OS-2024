#include <stdio.h>

int  num[20];

int check(int x){
	int tmp=x;
	int i=0;
	while(tmp){
		num[i]=tmp%10;
		tmp/=10;
		i++;
	}
	int j=0;
	i--;
	for(;j<i&&num[j]==num[i];j++,i--);
	if (j<i){
		return 0;
	}
	else{
		return 1;
	}
}


int main() {
	int n;
	scanf("%d", &n);

	if (check(n)) {
		printf("Y\n");
	} else {
		printf("N\n");
	}
	return 0;
}
