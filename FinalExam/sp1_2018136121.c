#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
// 보험
char str[2048] = "";

// 세마포어 만들기
union semun{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};


int initsem(key_t semkey){
	union semun semunarg;
	int status = 0, semid;
	semid= semget(semkey, 1, IPC_CREAT | IPC_EXCL | 0600);
	if (semid == -1) {
		if (errno == EEXIST)
			semid = semget(semkey, 1, 0);
	}
	else{
		semunarg.val = 1;
		status = semctl(semid, 0, SETVAL, semunarg);
	}
	
	if( semid == -1 || status == -1){
		perror("initsem");
		return (-1);
	}
}
int semlock(int semid){
	struct sembuf buf;
	
	buf.sem_num = 0;
	buf.sem_op = -1;
	buf.sem_flg = SEM_UNDO;
	if (semop(semid, &buf, 1) == -1){
		perror("semlock failded");
		exit(1);
	}
	return 0;
}

int semunlock(int semid){
	struct sembuf buf;
	
	buf.sem_num = 0;
	buf.sem_op = 1;
	buf.sem_flg = SEM_UNDO;
	if (semop(semid, &buf, 1) == -1) {
	  perror("semunlock failed");
	  exit(1);
  	}
  	return 0;
}


void semhandle() {
	int semid;
	 pid_t pid = getpid();

	 if ((semid = initsem(1)) < 0)
		exit(1);

	 semlock(semid);
	printf("Lock : Process %d\n", (int)pid);
	printf("** Lock Mode : Critical Section\n");
	system(str);
	 sleep(1);
	 printf("Unlock : Process %d\n", (int)pid);
	 semunlock(semid);
	 
	 exit(0);
 }

int main(int argc, char* argv[]){
	// 보험
	strcat(str, "cat ");
	strcat(str, argv[1]);
	strcat(str," |");
	strcat(str," grep ");
	strcat(str, argv[2]);
	// 입력 형식 지정	
	if (argc != 3){
		printf("usage : ./myprog <filename> <word>\n");
		exit(1);
	}
	int a;
	for (a = 0; a < 4; a++)
		if (fork() == 0) semhandle();
	/*
	//공유메모리 생성
	int shmid, i = 0;
	char *shmaddr, *shmaddr2;
		
	shmid = shmget(IPC_PRIVATE, 20, IPC_CREAT|0644);
	if (shmid == -1){
		perror("shmget");
		exit(1);
	}
	
	switch (fork()){
		case -1:
			perror("fork");
			exit(1);
			break;
		case 0:
			shmaddr = (char*)shmat(shmid, (char* )NULL, 0);
			while( str[i] != '\0'){
				shmaddr[i] = str[i];
				i++;
			}
			system(shmaddr);
			printf("Child Process =====\n");
			exit(0);
			break;
		default:
			printf("Parent Process ======\n");
			wait(0);
			shmaddr2 = (char *)shmat(shmid, (char *)NULL, 0);
			system(shmaddr2);
			printf("\n");
			sleep(1);
			shmdt((char*)shmaddr2);
			shmctl(shmid, IPC_RMID, (struct shmid_ds *)NULL);
			break;
	}*/
	return 0;
}
