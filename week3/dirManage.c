#include <fcntl.h> 
#include <unistd.h>	//  rmdir; cd; remove and change directory
#include <dirent.h>//Used for handling directory files
#include <errno.h>//For EXIT codes and error handling
#include <sys/types.h> // mkdir;
#include <sys/stat.h> // mkdir; ls; getcwd, file information : Struct Stat Header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // ctime in _ls method

#define MAX_CMD_SIZE	(128)	// 127 byte
//#define STR_SIZE	128
// /tmp/test를 실제 주소로 고정시키기 위해 사용
char real_dir[MAX_CMD_SIZE] = "/tmp/test";

// ls 기능 수행. -a , -l, -al 태그 사용 가능
void _ls(const char *dir,int op_a,int op_l)
{
	//Here we will list the directory
	struct dirent *d;
	struct stat buf;
	int title = 0;
	DIR *dh = opendir(dir);
	// 디렉터리 오픈 실패시 반환
	if (!dh)
	{
		if (errno = ENOENT)
		{
			//If the directory is not found
			perror("Directory doesn't exist");
		}
		else
		{
			//If the directory is not readable then throw error and exit
			perror("Unable to read directory");
		}
		exit(EXIT_FAILURE);
	}
	//While the next entry is not readable we will print directory files
	while ((d = readdir(dh)) != NULL)
	{
		lstat( d->d_name, &buf);
		//If hidden files are found we continue
		// a 태그 없으면 숨김
		if (!op_a && d->d_name[0] == '.')
			continue;
		// l 태그인 경우 : 이름 모드 UID GID TYPE 크기 수정시간
		if(op_l && !title) {
			printf("NAME\tMODE\tUID\tGID\tTYPE\t\tSIZE\t\tUPDATE_TIME\n");
			title++;	
		}
		// 이름
		printf("%s\t", d->d_name);
		if(op_l) {
			// 모드
			printf("%ld\t", (unsigned long)buf.st_mode);
			// UID GID
			printf("%ld\t%ld\t", (long)buf.st_uid, (long)buf.st_gid);
			// TYPE
			switch ( buf.st_mode & S_IFMT ){
				case S_IFBLK:	printf("block device\t");	break;
				case S_IFCHR:	printf("character device\t");	break;
				case S_IFDIR:	printf("directory\t");	break;
				case S_IFIFO:	printf("FIFO/pipe\t");	break;
				case S_IFLNK:	printf("symlink\t");	break;
				case S_IFSOCK:	printf("socket\t");	break;
				default:	printf("unknown?\t");	break;
			}
			printf("%lldbytes\t", (long long)buf.st_size);
			// 수정 시간
			printf("%s", ctime(&buf.st_ctime));
		}
	}
	if(!op_l)
	printf("\n");
}

// 경로 문제 해결을 위해 정의.
int mkdirs(const char *path, mode_t mode)
{
    char tmp_path[MAX_CMD_SIZE+10];
    const char *tmp = path;
    int  len  = 0;
    int  ret;

	// path 크기에 벗어나는 경우
    if(path == NULL || strlen(path) >= MAX_CMD_SIZE+10) {
        return -1;
    }

	// 경로(/)가 존재하는 경우
    while((tmp = strchr(tmp, '/')) != NULL) {
        len = tmp - path;
        tmp++;

        if(len == 0) {
            continue;
        }
        strncpy(tmp_path, path, len);
        tmp_path[len] = 0x00;
	// 에러 처리
        if((ret = mkdir(tmp_path, mode)) == -1) {
            if(errno != EEXIST) {
                return -1;
            }
        }
    }

    return mkdir(path, mode);
}
int main(int argc, char **argv){
	char *command, *tok_str, *sub_str,*sub_str2, *sub_str3;
	//char *current_dir = "/";
	char current_dir[MAX_CMD_SIZE] = "/";
	char real_dir[MAX_CMD_SIZE] = "/tmp/test";
	// /tmp/test로 이동
	chdir(real_dir);
	// /tmp/test 생성
	if(mkdir(real_dir,0755) == -1){
	//	2번 이상 실행하면 /tmp/test 폴더가 이미 존재하는 것때문에 실제로 이상이 없는 경우에도 오류가 지속적으로 떠서 가려둠
	//	perror(real_dir);
	}
	char *cwd;
	
	// command를 MAX_CMD_SIZE만큼 할당
	command = (char *)malloc(MAX_CMD_SIZE);
	// 에러 처리
	if(command == NULL){
		perror("malloc");
		exit(1);
	}
	
	do{
		// 현재 경로 출력
		printf("%s $ ", current_dir);
		// MAX_CMD_SIZE를 넘어 서는 경우 중단
		if (fgets(command, MAX_CMD_SIZE-1, stdin) == NULL) break;
		char tmp_str[MAX_CMD_SIZE];
		tok_str = strtok(command, "\n");
		// argv에서 인자를 받듯이, strncpy에서 공백을 기준으로 명령어를 각각 입력해줌
		strncpy(tmp_str, tok_str, MAX_CMD_SIZE);
		sub_str = strtok(command, " ");
		sub_str2 = strtok(NULL, " ");
		sub_str3 = strtok(NULL, " ");
	
	
		// 실제 경로의 /의 수 : dir_num, .의 수 : dot_num, 현재 경로의 /의 수 : sub_dir_num
		int i = 0, dir_num = 0, dot_num = 0, sub_dir_num = 0;
		while( i < MAX_CMD_SIZE ){
			if (real_dir[i] == '/')
				dir_num++;
			i++;
		}
		i = 0;
		if( sub_str2 ){
			while ( sub_str2[i] != '\0' ){
				if( sub_str2[i] == '.'){
					dot_num++;
				}
				if( sub_str2[i] == '/'){
					sub_dir_num++;
				}
				i++;
			}
		}
		if ( dot_num > 2){
			printf("도트는 3개 이상 사용할 수 없습니다.\n");
			continue;
		}
		
		if (dir_num > dot_num){
			if ( tok_str == NULL ) continue;
			
			if ( !strcmp(tok_str, "exit") || !strcmp(tok_str, "quit")){
				break;
			}
			// 내가 위치한 경로를 좀 더 편하게 보여주기 위해 실제 경로와 현재 경로를 추가로 구현
			else if( !strcmp(sub_str, "pwd") ){
				cwd = getcwd(NULL, BUFSIZ);
				printf("real_directory : %s\n", cwd);
				printf("current_directory : %s\n", current_dir);
			
			} else if(!strcmp(sub_str, "cd") ){
				int cd_dot = 1, t = 0;
				// cd 하려는 경로가 루트일 때
				if ( !strcmp( sub_str2, "/")){
					chdir("/tmp/test");
					strncpy(real_dir, "/tmp/test", MAX_CMD_SIZE);
					strncpy(current_dir, "/", MAX_CMD_SIZE);
				}
				// .. 를 이용해 이전 경로로 이동할 때
				else if (!strncmp(sub_str2, "..", 2)){
					while( dot_num > cd_dot){
						int idx;
						idx = MAX_CMD_SIZE-1;
						do{
							real_dir[idx] = '\0';
							idx--;
						}while( real_dir[idx] != '/');
						if (idx){
							real_dir[idx] = '\0';
						}
						idx = MAX_CMD_SIZE-1;
						do{
							current_dir[idx] = '\0';
							idx--;
						}while( current_dir[idx] != '/');
						if (idx){
							current_dir[idx] = '\0';
						}
						cd_dot++;
					}
					// 실제 경로로 이동
					chdir(real_dir);
					// 현재 경로의 tmp를 지정해준다.
					char tmp_str_current[MAX_CMD_SIZE];
					// .을 처리하기 위해 지정
					int idx = cd_dot;
					int t_current= 0;
					// tmp_str_current에 ../를 제거한 경로 지정
					if ( sub_str2[idx] == '/'){
						while( sub_str2[idx] != '\0'){
							if (t!=0){
								tmp_str_current[t_current] = sub_str2[idx];
								t_current++;
							}
							t++;
							idx++;
						}
						strncat(real_dir, "/", 2);
						strncat(real_dir, tmp_str_current, MAX_CMD_SIZE);
						// 실제 경로를 이용해 cd를 진행 and 에러 처리
						if( chdir(real_dir) == -1){
							printf("%s 디렉터리로 이동할 수 없습니다.\n", real_dir);
							perror("chdir");
							continue;
						}
						// 현재 경로 <- 현재경로의 tmp
						strncat(current_dir, tmp_str_current, MAX_CMD_SIZE);						
					}
					
				}
				// 루트와 ..가 아닌 경우
				else{
					strncat(real_dir, "/", 2);
					strncat(real_dir, sub_str2, MAX_CMD_SIZE);
					
					if( chdir(real_dir) == -1)
					{
						printf("%s 디렉터리로 이동할 수 없습니다.\n", real_dir);
						perror("chdir");
						continue;
					}
					if (strcmp(current_dir, "/")){
						if ( sub_str2[0] != '/'){
							strncat(current_dir, "/", 2);
						}
					}
					strncat(current_dir, sub_str2, MAX_CMD_SIZE);
				}
				
			} else if(!strcmp(sub_str, "ls") ){
				// 태그를 나누어 ls를 진행해준다.
				if ( sub_str2 == NULL){
					_ls(real_dir, 0, 0);
				}
				else{
					if(sub_str2[0] == '-'){
						int op_a = 0, op_l = 0;
						char *p = (char*)(sub_str2 + 1);
						while(*p){
							if(*p == 'a') op_a = 1;
							else if(*p == 'l') op_l = 1;
							else{
								perror("ls Option not available");
								continue;
							}
							p++;
						}
						_ls(real_dir, op_a, op_l);
					}
				
				}
							
			} else if(!strcmp(sub_str, "mkdir") ){
				char tmp_dir[MAX_CMD_SIZE] = "";
				strncat(tmp_dir, real_dir, MAX_CMD_SIZE);
				// 뒤로 가려고 할 때
				if (!strncmp(sub_str2, "..", 2)){
					// 루트에 위치하는 경우
					if ( !strcmp(real_dir, "/tmp/test") && !strcmp(current_dir, "/"))
						continue;
					// 경로를 벗어난 경우
					else if(dir_num < dot_num+1){
						printf("디렉터리를 만들 수 없습니다.\n");
						perror("mkdir");
						continue;
					}
					else{
						// 실제 경로의 .수가 mkdir에 들어가는 .수보다 많을 때
						int mkdir_dot = 2;
						while( dot_num >= mkdir_dot){
							int idx;
							idx = MAX_CMD_SIZE-1;
							do{
								tmp_dir[idx] = '\0';
								idx--;
							}while( tmp_dir[idx] != '/');
							if (idx){
								tmp_dir[idx] = '\0';
							}
							idx = MAX_CMD_SIZE-1;
							do{
								tmp_dir[idx] = '\0';
								idx--;
							}while( tmp_dir[idx] != '/');
							if (idx){
								tmp_dir[idx] = '\0';
							}
							mkdir_dot++;
						}
					}
				}
				// tmp_dir을 이용해 디렉터리를 설정해준다.
				strncat(tmp_dir, "/", 2);
				strncat(tmp_dir, sub_str2, MAX_CMD_SIZE);
				printf("mkdir : %s\n", tmp_dir); 
				// 사용자 정의 mkdirs 이용해 입력받은 tmp_dir에  디렉터리를 생성해준다.
				if ( mkdirs(tmp_dir, 0775) == -1 ){
				
					printf("디렉터리를 생성할 수 없습니다.");
					perror("mkdir");
				}
				
			
			} else if(!strcmp(sub_str, "rmdir") ){
				char tmp_dir[MAX_CMD_SIZE] = "";
				// tmp_dir에 경로처리를 통해 디렉터리를 삭제해준다.
				strncat(tmp_dir, real_dir, MAX_CMD_SIZE);
				strncat(tmp_dir, "/", 2);
				strncat(tmp_dir, sub_str2, MAX_CMD_SIZE);
				printf("rmdir : %s\n", tmp_dir); 
				if(rmdir(tmp_dir) == -1){
					printf("디렉터리를 삭제할 수 없습니다.\n");
					perror("rmdir");
					continue;
				}
			} else if(!strcmp(sub_str, "rename") ){
				// rename [목적지 : tmp_dir2] [출발지 : tmp_dir3]
				char tmp_dir2[MAX_CMD_SIZE] = "";
				strncat(tmp_dir2, real_dir, MAX_CMD_SIZE);
				strncat(tmp_dir2, "/", 2);
				strncat(tmp_dir2, sub_str2, MAX_CMD_SIZE);
				char tmp_dir3[MAX_CMD_SIZE] = "";
				strncat(tmp_dir3, real_dir, MAX_CMD_SIZE);
				strncat(tmp_dir3, "/", 2);
				strncat(tmp_dir3, sub_str3, MAX_CMD_SIZE);
				printf("rename %s %s\n", tmp_dir2, tmp_dir3);
				// 에러 처리
				if ( rename(tmp_dir2, tmp_dir3) == -1){
					printf("이름을 바꿀 수 없습니다.\n");
					perror("rename");
				}
			} else if(!strcmp(sub_str, "help") ){
				printf("help : 구현된 명령어를 설명해주는 역할을 수행한다.\n");
				printf("ls : -a, -l, -al 연산을 지원한다.\n현재 디렉터리에 존재하는 파일 및 폴더 등의 정보를 나타낸다.\n");
				printf("cd <경로>: 현재 디렉터리의 경로를 바꿔준다.\n상단의 기본규칙에 언급한 사항들의 내용을 우선적으로 따른다.\n");
				printf("기본 규칙1. \"cd ../경로\", \"cd 경로\"의 형태들을 기본적으로 지원한다.\n");
				printf("기본 규칙2. \"cd ./경로\", \"cd /경로\", \"cd 경로/\", \"cd /경로/\" 등의 형태는 지원하지 않는다.\n");
				printf("mkdir <경로> : <경로>상에  디렉터리를 만들어준다.\n이미 존재하는 경우엔 만들어주지 않는다.");
				printf("rmdir <경로> : <경로>상에 존재하는 디렉터리를 삭제해준다.\n경로 상에 다른 파일 및 폴더가 있는 경우엔 삭제하지 않는다.\n");
				printf("pwd : 실제 디렉터리와 현재 디렉터리의 경로를 보여준다.\ncd 명령의 결과를 보기 위해 사용되었다.\n");
				printf("rename <A경로> <B경로> : A경로를 B경로로 이름을 바꿔준다.\n");
				
			} else { 
				printf("your command: %s\n", tok_str);
				printf("aHand argument is ");
				
				tok_str = strtok(NULL, "\n");
				if ( tok_str == NULL ) { 
					printf("NULL\n");
				} else {
					printf("%s\n", tok_str);
				}
			}
		}
		else{
			printf("루트 디렉터리 이상으로 접근할 수 없습니다\n");
			perror("chdir");
		}
	} while(1);
	free(command);
	return 0;
}
