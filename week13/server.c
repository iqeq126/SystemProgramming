// https://github.com/anmolkr186/Multi-user-Chat

#include <stdio.h> // 입출력을 위해 사용
#include <stdlib.h> // 시스템 함수 사용을 위해
#include <error.h> // 에러 처리를 위해 사용
#include <errno.h> // 파이프 생성시 오류 판별을 위해 사용
#include <sys/stat.h> // stat 구조체 사용을 위해 | File Descriptor Open을 위해
#include <unistd.h> // 시스템 함수 사용을 위해
#include <fcntl.h> // 읽기전용 : O_RDONLY 사용을 위해 | File Descriptor Open을 위해
#include <string.h> // strcpy, strlen 등 문자열 함수를 이용하기 위해
#include <ctype.h> // File Descriptor Open을 위해

#define SERVER_FIFO "/tmp/test"
#define CLIENT_NUM 100
int main (int argc, char **argv)
{
    int fd, fd_client, bytes_read, i;// fd : 서버 fd_client : 클라이언트, bytes_read : 받은 메시지
    char fifo_name_set[20][100]; // 클라이언트에서 받아온 메시지. 최대 길이가 100, 20단어를 받아옴.
    int counter = 0; // 유저수
    char return_fifo[20][100]; // 보내는 메시지. 0번지에는 유저명을 저장. 1~19번지에 보낼 단어를 정의할 수 있고. 길이는 100으로 제한된다.
    char buf [4096];
    
    // 서버의 실행을 알린다.
    printf("Server Online\n");

    // array of strings maintaining addresses of client terminals
    // 클라이언트에서 새로운 메시지를 받기 위해 빈 문자열로 초기화 해준다.
    for(int i = 0;i<20;i++)
    {
        strcpy(fifo_name_set[i],"");
    }

    // making fifo : 파이프 생성 => RW-RW-R-- 권한으로 생성. 존재
    if ((mkfifo (SERVER_FIFO, 0664) == -1) && (errno != EEXIST)) {
        perror ("mkfifo");
        exit (1);
    }
    // opening fifo
    // fd로 서버 파이프를 입력받는다.
    if ((fd = open (SERVER_FIFO, O_RDONLY)) == -1)
        perror ("open");

     // 무한 루프를 돌며 메시지를 대기한다.
     while(1)
     {
        // NULL 문자를 문자열 끝에 추가해준다.
        strcpy(return_fifo[0],"\0");
        // 데이터를 기다린다.
    	// **argv같은 형식으로 받아온다고 생각하면 됨.
    	// 공백 구분으로 단어를 20개까지 받을 수 있다.
        for(int i =0;i<20;i++)
        {
            strcpy(return_fifo[i],"");
        }

         // setting memset for filling a block of memory for string type buf    
         // memset으로 buf를 모두 NULL 문자를 초기화해준다.
         memset (buf, '\0', sizeof (buf));

         // bytes_read에 fd에서 다른 유저가 보낸 데이터를 읽는다.
         if ((bytes_read = read (fd, buf, sizeof (buf))) == -1)
             perror ("read");

	// bytes_read에 값이 읽혔을 경우
         if (bytes_read > 0) 
         {
            // string parsing
            char* token;
            char* rest = buf; // buf의 내용을 rest에 담고
            int i = 0;
            // 공백을 기준으로 token에 가져온다.
            while ((token = strtok_r(rest, " ", &rest)) && strlen(token)!=0)
            { 
            // 가져온 내용은 **argv같은 형식으로 return_fifo[i]에 넣어준다.
                strcpy(return_fifo[i],token);
                i++;
            }

            int sender_number = 0; // 전송 번호
            
            // adding new client address to fifo_name_set
            // 새로운 유저를 추가한다.
            // client에 전달된 내용 출력
            //if(strcasecmp(return_fifo[0],"new")==0)
            if(strncmp(return_fifo[0], "usr:", 4) == 0)
            {
                // printf("\nNew user added with address: %s", return_fifo[1]);                
                counter++; // 유저 추가
                strcpy(fifo_name_set[counter],return_fifo[1]);// counter 번째에 유저명 저장
                //strcpy(fifo_name_set[counter],return_fifo[0]); 
                sender_number = counter; // 전송 번호를 갱신해줌.
       
                char* str = return_fifo[1];// str에 유저명 저장
                //char* str = return_fifo[0]; 
                char success_string[100]; // 성공시 success와 유저 번호를 저장하기 위함
                char a[CLIENT_NUM]; // 유저정보 저장. 100개까지
                sprintf(a,"%d",sender_number); // 전달받은 수 출력
		// f"success {a}" 같은 내용 저장 가능
                strcpy(success_string,"success ");
                strcat(success_string, a);

                // sending the creatin status back to client
                // 클라이언트를 쓰기 전용으로 연다.
                if ((fd_client = open (str, O_WRONLY)) == -1) {
                    perror ("open: client fifo");
                    continue;
                }

                // 클라이언트에 success_string을 기록한다.
                if (write (fd_client, success_string, strlen (success_string)) != strlen (success_string))
                    perror ("write");

                // 끝난 경우 메모리를 반환한다.
                if (close (fd_client) == -1)
                    perror ("close");
            }
            

            // sending messages to clients
            // 클라이언트에게 메시지를 보내는 경우
            //else if(strcasecmp(return_fifo[1],"send")==0)
            else
            {
                // parsing the message and the sender
                // 송신자로부터 메시지를 파싱해준다.
                for(int i =1;i<20;i++)
                {
                    if(strcasecmp(return_fifo[0],fifo_name_set[i])==0)
                    {
                        sender_number = i;
                    }   
                }
                // sender_number : 어느 송신자로부터 메시지를 받았는지 출력
                printf("\n>RECEIVED message from %d(%s): ", sender_number, fifo_name_set[sender_number]);
                // printf("\nMessaged recieved:");
                for ( int i=1;i<20;i++)
                    printf("%s ",return_fifo[i]);

                char a[CLIENT_NUM];
                //sending to all the clients

                for(int j =1;j<20;j++)
                {
                    if(strlen(fifo_name_set[j])!=0)
                    {
                        char currentClient[100],receiver_data[500]="";
                        strcpy(currentClient,fifo_name_set[j]);
                        strcpy(receiver_data,currentClient);
                        // searching for sender's index from fifo_name_set and adding to receiver's address
                        // 송신자의 인덱스를 찾아서 fifo_name_set부터 receiver의 주소까지를 더해준다.
                        for(int i =1;i<20;i++)
                        {
                            if(strcasecmp(return_fifo[0],fifo_name_set[i])==0)
                            {
                                sprintf(a,"%d",i);
                                strcat(receiver_data," ");
                                //strcat(receiver_data,a);
                                strcat(receiver_data, return_fifo[0]);
                                break;
                            }
                        }
                        
                        // adding message
                        // strcat을 이용해 구분된 공백과 return_fifo에 저장된 메시지를 receiver_data에 저장해준다.
                        for(int i =1;i<20;i++)
                        {
                            if(strlen(return_fifo[i])!=0)
                            {
                                strcat(receiver_data," ");
                                strcat(receiver_data,return_fifo[i]);
                            }
                        }
                        // return_fifo[0]와 currentCLient의 유저명이 다를 경우에
                        if(strcasecmp(return_fifo[0],currentClient)!=0)
                        {
                            //sending the message
                            // 메시지를 보낸다.
                            // currentClient를 쓰기 전용으로 열어주고 fd_client FileDescripter에 저장해준다.
                            if ((fd_client = open (currentClient, O_WRONLY)) == -1)
                            {
                                perror ("open: client fifo");
                                continue;
                            }
                            
                            
                            if (write (fd_client, receiver_data, strlen (receiver_data)) != strlen (receiver_data))
                                perror ("write");
                            // fd_client와의 연결을 종료한다.
                            if (close (fd_client) == -1)
                                perror ("close");
                        }
			// 보낸 메시지를 출력한다. 
                        printf("\n>SENT message : ");
                        for ( int i=1;i<20;i++)
                            printf("%s ",return_fifo[i]);  
                   

            }
            
        }

     }
     }
}
}
