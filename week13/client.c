// https://github.com/anmolkr186/Multi-user-Chat

#include <stdio.h> // 입출력을 위해 사용
#include <stdlib.h>// 시스템 함수 사용을 위해
#include <error.h> // perror 등 에러 처리를 위해 사용
#include <errno.h>// 파이프 생성시 오류 판별을 위해 사용
#include <sys/stat.h>  // stat 구조체 사용을 위해 | File Descriptor Open을 위해
#include <sys/types.h>// File Descriptor Open을 위해
#include <unistd.h> // 시스템 함수 사용을 위해
#include <fcntl.h> // 쓰기 전용 : O_WRONLY 사용을 위해 | File Descriptor Open을 위해
#include <string.h> // strcpy, strcat 등 문자열 함수를 이용하기 위해
#include <memory.h> //memset 이용을 위해
#include <pthread.h>  // POSIX Thread. POSIX에서 스레드 생성, 동기화를 위해 만든 표준 API

#define SERVER_FIFO "/tmp/test"

int fd, fd_server, bytes_read; // fd : 다른 클라이언트, fd_server : 서버, bytes_read : 받은 메시지
char buf1 [512], buf2 [1024];
char userString[128];
char my_fifo_name [128]; // 쓰레드 주소
int programflag = 0;
char* username; // 유저명
void *send(void *pthread)
{
    
    while(1)
    {
    	// 사용자의 아이디를 pthread로 설정한다.
        int *myid = (int *)pthread; 
        // 1. 처음 연결시
        if(programflag == 0)
        {
            // my_fifo_name에 userString을 연결시킨다.
            strcat (userString, my_fifo_name);
            // server File Descripter를 쓰기 전용으로  open한다. 
            if ((fd_server = open (SERVER_FIFO, O_WRONLY)) == -1) {
                perror ("open: server fifo");
                break;
            }
            // server File Descripter에 수신되는 메시지를 userString만큼 보낸다. 
            if (write (fd_server, userString, strlen (userString)) != strlen (userString)) {
                perror ("write");
                break;
            }
            // server File Descripter와의 연결을 종료한다. 
            if (close (fd_server) == -1) {
                perror ("close");
                break;
            }
            programflag = 1;
        }
        // 2. 그 이후
        else if(programflag ==1)
        {
            printf("\n");
            // buf1을 표준 입력(stdin)으로 메시지를 입력받는다.
            if (fgets(buf1, sizeof (buf1), stdin) == NULL)
                break;
            // buf2에 현재 클라이언트의 주소를 넣고
            strcpy (buf2, my_fifo_name);
            strcat (buf2, " ");
            // buf : <현재 클라이언트의 주소> <메시지>의 형태로 나올 수 있도록 buf2를 만들어준다.
            strcat (buf2, buf1);

            // server File Descripter를 쓰기 전용으로  open한다.  
            if ((fd_server = open (SERVER_FIFO, O_WRONLY)) == -1) {
                perror ("open: server fifo");
                break;
            }
            // server File Descripter에 수신되는 메시지를 buf2만큼 보낸다. 
            if (write (fd_server, buf2, strlen (buf2)) != strlen (buf2)) {
                perror ("write");
                break;
            }   
            // server File Descripter와의 연결을 종료한다. 
            if (close (fd_server) == -1) {
                perror ("close");
                break;
            }
        }
    }
    
    // pthread를 탈출한다.
    pthread_exit(NULL);
    // void형 메서드 : return NULL
    return NULL;
}

void *receive(void *pthread)
{ 
    // 무한 루프를 돌며 메시지를 대기한다.
    while(1)
    {
    	// 데이터를 기다린다.
    	// **argv같은 형식으로 받아온다고 생각하면 됨.
    	// 공백 구분으로 단어를 20개까지 받을 수 있다.
        char read_data[20][100];
        for(int i =0;i<20;i++)
        {
            strcpy(read_data[i],"");
        }
        // 메시지를 보내는 파이프를 읽기 전용으로 연다
        if ((fd = open (my_fifo_name, O_RDONLY)) == -1)
           perror ("open");

        // buf2의 시작지점부터 끝까지 NULL('\0')로 초기화해준다.
        memset (buf2, '\0', sizeof (buf2));

        // bytes_read에 fd에서 다른 유저가 보낸 데이터를 읽는다.
        if ((bytes_read = read (fd, buf2, sizeof (buf2))) == -1)
            perror ("read");

        if (bytes_read > 0) 
        {
            char* token;
            char* rest = buf2; 
            int i = 0;
            while ((token = strtok_r(rest, " ", &rest)))
            { 
                strcpy(read_data[i],token);
                i++;
            }
            // 만약에 read를 성공했다면
            if(strcmp("success",read_data[0])==0)
            {
                printf ("Creation Status: %s \nYour name is: %s\nYour port number is: %s\n", read_data[0],my_fifo_name,read_data[1]);
            }
            else
            {
                // printf("%s",buf2);
                printf("\n[ %s ]", read_data[1]);
                // 20줄까지 **argv처럼 read_data에 불러온다.
                for(int i = 2;i<20;i++)
                {
                    printf("%s ",read_data[i]);
                }
            }
        }
        // fd와의 연결을 종료한다.
        if (close (fd) == -1) {
            perror ("close");
            break;
        }
    }
    pthread_exit(NULL);
    return NULL;

}


int main (int argc, char **argv)
{


    // 클라이언트를 유일한 PID로 만든다.
    if(argc == 1){
        sprintf (my_fifo_name, "%ld", (long) getpid ());
    }
    // 만약 유저명이 입력된 경우 유저명을 아이디로 만든다. 
    else if (argc == 2 ){
        sprintf (my_fifo_name, "%s", argv[1]);
    }
    else{
        printf("입력값이 너무 많아요");
        return 0;
    }
    
    // 클라이언트를 named pipe로 만든다.
    // mkfifo : 파이프 생성 => RW-RW-R-- 권한으로 생성
    remove(my_fifo_name);
    if (mkfifo (my_fifo_name, 0664) == -1)
        perror ("mkfifo");

    // 채팅창 접속
    if (argc == 2)
    {
	    // 유저명(username)을 두 번째 인자로 받는다.
            username = argv[1];
	    printf("Welcome to the Chat ! %s", username);
	    // 새 스트링 추가 
            // strcpy(newstring, "new ");
            strcpy(userString, "usr:");
            strcat(userString, username);
            strcat(userString, " ");
    }
    else
    {
	    printf("Welcome to the Chat ! %s\n", my_fifo_name);
	    strcpy(userString, "usr: ");
    }
    	
    // Send와 Receive 쓰레드를 분리한다
    pthread_t sending_thread,receiving_thread;

    // thread에서 send 메서드를 호출한다.
    if(pthread_create(&sending_thread, NULL, send, (void *)&sending_thread)!=0)
    {
        fprintf(stderr, "Error creating sending thread\n");
        return 1;
    }

    // 다른 thread에서 receive 메서드를 호출한다.
    if(pthread_create(&receiving_thread, NULL, receive, (void *)&receiving_thread)!=0)
    {
        fprintf(stderr, "Error creating receiving thread\n");
        return 1;
    }

    // 두 thread를 join하여 쓰레드의 종료까지 대기한다.
    if(pthread_join(sending_thread, NULL))
        fprintf(stderr, "Error joining sending thread\n");

    if(pthread_join(receiving_thread, NULL))
        fprintf(stderr, "Error joining receiving thread\n");

    return 0;
}
