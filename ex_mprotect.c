#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
void sig_handler(int signo){
	psignal(signo, "Received Signal:");
	sleep(5);
	printf("In Signal Handler, After sleep\n");
}

int sum(int a, int b){
	return a + b;
}

int main(){
	struct sigaction act;
	char text[0x1000];	// 4kb
	int (*sum2)(int, int); 	// 함수 포인터
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGQUIT);
	act.sa_flags = 0;
	act.sa_handler = sig_handler;
	if(sigaction (SIGINT, &act, (struct sigaction *)NULL) < 0){
		perror("sigaction");
		exit(1);
	}
	
	printf("sum=%p, text:%p\n", &sum, &text);

	// 페이지 사이즈로 정렬해야한다. 4k, 8k 등등. Ox7ffffffff000은 정렬을 위함. 0x4000은 스택. PROC들은 mprotext를 위함.
	mprotect((long long)text & 0x7ffffffff000, 0x4000, PROT_READ | PROT_WRITE | PROT_EXEC );
	
	memcpy(text, sum, 100);	// sum 함수의 주소. 그 코드가 저장된 영역을 text 변수에 저장한다. 넉넉잡아 100바이트
	sum2 = text;		// sum2 = sum코드가 복사된 스택 영역
	printf("1+2=%d\n", sum2(1,2));
	
	fprintf(stderr, "Input SIGNT: ");
	pause();
	fprintf(stderr, "Atfer Signal Handler\n");
}
