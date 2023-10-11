#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> // dirent 구조체 사용을 위함
#include <errno.h> // 에러 처리를 위함
#include <sys/stat.h> // stat 구조체 사용을 위함 
#include <sys/types.h>
#include <unistd.h>
#include <time.h> // ctime 사용을 위함
#include <fcntl.h> // O_RDONLY 사용을 위함
#define MAX_CMDLINE_SIZE    (128)
#define MAX_CMD_SIZE        (32)
#define MAX_ARG             (4)

typedef int  (*cmd_func_t)(int argc, char **argv);
typedef void (*usage_func_t)(void);

typedef struct cmd_t {
    char            cmd_str[MAX_CMD_SIZE];
    cmd_func_t      cmd_func;
    usage_func_t   usage_func;
    char            comment[128];
} cmd_t;

#define DECLARE_CMDFUNC(str)    int cmd_##str(int argc, char **argv); \
                                void usage_##str(void)

DECLARE_CMDFUNC(help);
DECLARE_CMDFUNC(mkdir);
DECLARE_CMDFUNC(rmdir);
DECLARE_CMDFUNC(cd);
DECLARE_CMDFUNC(mv);
DECLARE_CMDFUNC(ls);
DECLARE_CMDFUNC(ln);
DECLARE_CMDFUNC(chmod);
DECLARE_CMDFUNC(cat);
DECLARE_CMDFUNC(cp);
DECLARE_CMDFUNC(rm);
DECLARE_CMDFUNC(quit);

/* Command List */
static cmd_t cmd_list[] = {
    {"help",    cmd_help,    usage_help,  "show usage, ex) help <command>"},
    {"mkdir",   cmd_mkdir,   usage_mkdir, "create directory"},
    {"rmdir",   cmd_rmdir,   usage_rmdir, "remove directory"},
    {"cd",      cmd_cd,      usage_cd,    "change current directory"},
    {"mv",      cmd_mv,      usage_mv,    "rename directory & file"},
    {"ls",      cmd_ls,      usage_ls,        "show directory contents"},
    {"ln",		cmd_ln,		usage_ln,		"create link"},
    {"chmod",	cmd_chmod,	usage_chmod, "ex) chmod 0644 test.txt"},
    {"cat",		cmd_cat,	usage_cat, 	"cat file"},
    {"cp",		cmd_cp,		usage_cp, 	"copy file"},
    {"rm",		cmd_rm,		 usage_rm,		"remove file"},
    {"quit",    cmd_quit,    NULL,        "terminate shell"},
};

const int command_num = sizeof(cmd_list) / sizeof(cmd_t);
static char *chroot_path = "/tmp/test";
static char *current_dir;

static int search_command(char *cmd)
{
    int i;

    for (i = 0; i < command_num; i++) {
        if (strcmp(cmd, cmd_list[i].cmd_str) == 0) {
            /* found */
            return (i);
        }
    }

    /* not found */
    return (-1);
}

static void get_realpath(char *usr_path, char *result)
{
    char *stack[32];
    int   index = 0;
    char  fullpath[128];
    char *tok;
    int   i;
#define PATH_TOKEN   "/"

    if (usr_path[0] == '/') {
        strncpy(fullpath, usr_path, sizeof(fullpath)-1);
    } else {
        snprintf(fullpath, sizeof(fullpath)-1, "%s/%s", current_dir + strlen(chroot_path), usr_path);
    }

    /* parsing */
    tok = strtok(fullpath, PATH_TOKEN);
    if (tok == NULL) {
        goto out;
    }

    do {
        if (strcmp(tok, ".") == 0 || strcmp(tok, "") == 0) {
            ; // skip
        } else if (strcmp(tok, "..") == 0) {
            if (index > 0) index--;
        } else {
            stack[index++] = tok;
        }
    } while ((tok = strtok(NULL, PATH_TOKEN)) && (index < 32));

out:
    strcpy(result, chroot_path);

    // TODO: boundary check
    for (i = 0; i < index; i++) {
        strcat(result, "/");
        strcat(result, stack[i]);
    }
}

int main(int argc, char **argv)
{
    char *command, *tok_str;
    char *cmd_argv[MAX_ARG];
    int  cmd_argc, i, ret;

    command = (char*) malloc(MAX_CMDLINE_SIZE);
    if (command == NULL) {
        perror("command malloc");
        exit(1);
    }

    current_dir = (char*) malloc(MAX_CMDLINE_SIZE);
    if (current_dir == NULL) {
        perror("current_dir malloc");
        free(command);
        exit(1);
    }

    if (chdir(chroot_path) < 0) {
        mkdir(chroot_path, 0755);
        chdir(chroot_path);
    }

    // chroot(chroot_path);

    do {
        if (getcwd(current_dir, MAX_CMDLINE_SIZE) == NULL) {
            perror("getcwd");
            break;
        }

        if (strlen(current_dir) == strlen(chroot_path)) {
            printf("/"); // for root path
        }
        printf("%s $ ", current_dir + strlen(chroot_path));

        if (fgets(command, MAX_CMDLINE_SIZE-1, stdin) == NULL) break;

        /* get arguments */
        tok_str = strtok(command, " \n");
        if (tok_str == NULL) continue;

        cmd_argv[0] = tok_str;

        for (cmd_argc = 1; cmd_argc < MAX_ARG; cmd_argc++) {
            if (tok_str = strtok(NULL, " \n")) {
                cmd_argv[cmd_argc] = tok_str;
            } else {
                break;
            }
        }

        /* search command in list and call command function */
        i = search_command(cmd_argv[0]);
        if (i < 0) {
            printf("%s: command not found\n", cmd_argv[0]);
        } else {
            if (cmd_list[i].cmd_func) {
                ret = cmd_list[i].cmd_func(cmd_argc, cmd_argv);
                if (ret == 0) {
                    printf("return success\n");
                } else if (ret == -2 && cmd_list[i].usage_func) {
                    printf("usage : ");
                    cmd_list[i].usage_func();
                } else {
                    printf("return fail(%d)\n", ret);
                }
            } else {
                printf("no command function\n");
            }
        }
    } while (1);

    free(command);
    free(current_dir);

    return 0;
}


int cmd_help(int argc, char **argv)
{
    int i;

    if (argc == 1) {
        for (i = 0; i < command_num; i++) {
            printf("%32s: %s\n", cmd_list[i].cmd_str, cmd_list[i].comment);
        }
    } else if (argv[1] != NULL) {
        i = search_command(argv[1]);
        if (i < 0) {
            printf("%s command not found\n", argv[1]);
        } else {
            if (cmd_list[i].usage_func) {
                printf("usage : ");
                cmd_list[i].usage_func();
                return (0);
            } else {
                printf("no usage\n");
                return (-2);
            }
        }
    }

    return (0);
}

int cmd_mkdir(int argc, char **argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);
        
        if ((ret = mkdir(rpath, 0755)) < 0) {
            perror(argv[0]);
        }
    } else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_rmdir(int argc, char **argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);
        
        if ((ret = rmdir(rpath)) < 0) {
            perror(argv[0]);
        }
    } else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_cd(int argc, char **argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);

        if ((ret = chdir(rpath)) < 0) {
            perror(argv[0]);
        }
    } else {
        ret = -2;
    }

    return (ret);
}

int cmd_mv(int argc, char **argv)
{
    int  ret = 0;
    char rpath1[128];
    char rpath2[128];

    if (argc == 3) {

        get_realpath(argv[1], rpath1);
        get_realpath(argv[2], rpath2);

        if ((ret = rename(rpath1, rpath2)) < 0) {
            perror(argv[0]);
        }
    } else {
        ret = -2;
    }

    return (ret);
}

static const char *get_type_str(char type)
{
    switch (type) {
        case DT_BLK:
            return "BLK";
        case DT_CHR:
            return "CHR";
        case DT_DIR:
            return "DIR";
        case DT_FIFO:
            return "FIFO";
        case DT_LNK:
            return "LNK";
        case DT_REG:
            return "REG";
        case DT_SOCK:
            return "SOCK";
        default: // include DT_UNKNOWN
            return "UNKN";
    }
}
// https://www.it-note.kr/173
int cmd_ls(int argc, char **argv)
{
    int ret = 0;
    int title = 0;
    DIR *dp;
    
    struct dirent *dep;
    struct stat buf;
    if (argc > 2) {
        ret = -2;
        goto out;
    }

    if ((dp = opendir(".")) == NULL) {
        ret = -1;
        goto out;
    }

    while (dep = readdir(dp)) {
	    	lstat( dep->d_name, &buf);
	    	 //모드 이름 UID GID 크기 접근시간 수정시간 생성시간 하드링크수 심볼릭 링크 정보
			if(!title) {
				printf("MODE\t\tNAME\t\tUID\tGID\tSIZE\t\tACCESS_TIME\t\t\tUPDATE_TIME\t\t\tCHANGE_TIME\t\t\tLINK_NUM\tSYM_INFO\n");
				title++;	
			}
        	// printf("%10ld %4s %s\n", dep->d_ino, get_type_str(dep->d_type), dep->d_name);
			// 모드(type)
			switch ( buf.st_mode & S_IFMT ){
				case S_IFBLK:	printf("b");	break;
				case S_IFCHR:	printf("c");	break;
				case S_IFDIR:	printf("d");	break;
				case S_IFIFO:	printf("p");	break;
				case S_IFLNK:	printf("l");	break;
				case S_IFSOCK:	printf("s");	break;
				default:	printf("-");	break;
			}
			// 모드
			
			// printf("%ld\t", (unsigned long)buf.st_mode);
				
			if ( buf.st_mode & S_IRUSR ) printf("r"); else printf("-");
			if ( buf.st_mode & S_IWUSR ) printf("w"); else printf("-");
			if ( buf.st_mode & S_IXUSR ) printf("x"); else printf("-");
			if ( buf.st_mode & S_IRGRP ) printf("r"); else printf("-");
			if ( buf.st_mode & S_IWGRP ) printf("w"); else printf("-");
			if ( buf.st_mode & S_IXGRP ) printf("x"); else printf("-");
			if ( buf.st_mode & S_IROTH ) printf("r"); else printf("-");
			if ( buf.st_mode & S_IWOTH ) printf("w"); else printf("-");
			if ( buf.st_mode & S_IXOTH ) printf("x"); else printf("-");
			// 이름
			printf("\t%8s\t", dep->d_name);
			// UID GID
			printf("%ld\t%ld\t", (long)buf.st_uid, (long)buf.st_gid);
			// 크기
			printf("%8lldbytes\t", (long long)buf.st_size);
			// 접근 시간
			printf("%s\t", strtok(ctime(&buf.st_atime), "\n"));
			// 수정 시간
			printf("%s\t", strtok(ctime(&buf.st_mtime), "\n"));
			// 생성 시간
			printf("%s\t", strtok(ctime(&buf.st_ctime), "\n"));
			// 하드링크수
			printf("%ld\t\t", (unsigned long)(buf.st_nlink)); 
			// 심볼릭 링크 정보
			// https://badayak.com/entry/C%EC%96%B8%EC%96%B4-%EC%8B%AC%EB%B3%BC%EB%A6%AD%EB%A7%81%ED%81%AC-%EC%A0%95%EB%B3%B4-%EA%B5%AC%ED%95%98%EA%B8%B0-readlink
			char buff[1024];
			int sz_link;
			char rpath[128];
			get_realpath(dep->d_name, rpath);
			memset(buff, 0, sizeof(buff));
			sz_link = readlink(rpath, buff, sizeof(buff));
			int i = 10, t = 0;
			while(buff[i] != '\0'){
				buff[i-10] = buff[i];
				i++;
			}
			for(t = i; t > i-11; t--){
				buff[t] = '\0';
			}
			if ( 0 < sz_link) printf("%s->%s\n", dep->d_name, buff);
			else printf("%s\n", dep->d_name);// 링크가 없는 경우printf("error! %d, %s\n", errno, strerror(errno)); 
    }

    closedir(dp);

out:
    return (ret);
}

int cmd_ln(int argc, char **argv)
{
	int ret = 0;
	char rpath1[128]; // source : original file name
	char rpath2[128]; // destination : link name
	// hard link
	if (argc == 3){
		get_realpath(argv[1], rpath1);
		get_realpath(argv[2], rpath2);
		if(link(rpath1, rpath2) != 0)
		{
			perror("hardlink() error");
			unlink(rpath1);
			ret = -1;
		}
	}
	// symbolic link
	else if (argc == 4){
		get_realpath(argv[2], rpath1);
		get_realpath(argv[3], rpath2);	
		if(symlink(rpath1, rpath2) != 0)
		{
			perror("symlink() error");
			unlink(rpath1);
			ret = -1;
		}
	}
	else{
		ret = -2;
	}
	
	return (ret);	
}
// https://prgwonders.blogspot.com/2015/12/c-program-to-implement-cat-command.html
int cmd_cat(int argc, char **argv)
{
    int  ret = 0;
    char rpath[128];
	char buf[2048];	// 파일 내용 담는 버퍼
    int fdold, count;
    if (argc == 2) {
        get_realpath(argv[1], rpath);
        fdold = open(rpath, O_RDONLY); // 파일 읽어오기
        if (fdold == -1)
        {
        	printf("cannot open file");
        	ret = -1;
        }
        else
        {
        	while((count = read(fdold, buf, sizeof(buf))) > 0)
        	{
        		printf("%s", buf);	
        	}
        }
    } else {
        ret = -2; // syntax error
    }
	return (ret);
}
int cmd_rm(int argc, char **argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);
        
        if ((ret = remove(rpath)) < 0) {
            perror(argv[0]);
        }
    } else {
        ret = -2; // syntax error
    }

    return (ret);
}
int cmd_cp(int argc, char **argv)
{
    int  ret = 0;
    char rpath1[128];
    char rpath2[128];
    if (argc == 3) {
    	/*if (argv[1] == "*" ){
    		
    	}
    	else{
		
        }*/
        
	struct stat buf;
        get_realpath(argv[1], rpath1);
	get_realpath(argv[2], rpath2);
        stat(rpath1, &buf);
        int oldfd = open(rpath1, O_CREAT | O_WRONLY | O_TRUNC, buf.st_mode & 0777);
        int newfd = open(rpath2, O_CREAT | O_WRONLY | O_TRUNC, buf.st_mode & 0777);
        if (oldfd == -1 || newfd == -1){
        	perror(argv[0]);
        }
        dup2(oldfd, newfd);
        close(oldfd); close(newfd);
    } 
    else {
        ret = -2;
    }

    return (ret);
}

int cmd_chmod(int argc, char **argv)
{
    int  ret = 0;
    char rpath[128];
    char* octa;
    int rwx = 0100000;
    struct stat buf;
    if (argc == 3) {
        get_realpath(argv[2], rpath);
        stat(rpath, &buf);
        rwx = ( ( buf.st_mode ) / 01000 ) * 01000;
	//printf("%#lo\n", (unsigned long) buf.st_mode);
		if(argv[1][0] == '0'){
			rwx += strtol(argv[1], &octa, 8);
			buf.st_mode = rwx;
			//printf("%#lo\n", (unsigned long) buf.st_mode);
			chmod(rpath, buf.st_mode);
		}
		else{
			if(argv[1][0] == '-'){
			    if (strchr(argv[1], 'r')) rwx += 0444;
				if (strchr(argv[1], 'w')) rwx += 0222;
				if (strchr(argv[1], 'x')) rwx += 0111;
				buf.st_mode &= ~(rwx);
			}
			else if(argv[1][0] == '+'){
				if (strchr(argv[1], 'r')) rwx += 0444;
				if (strchr(argv[1], 'w')) rwx += 0222;
				if (strchr(argv[1], 'x')) rwx += 0111;
				buf.st_mode |= rwx;
			}
			else if(argv[1][0] == 'u' || argv[1][0] == 'g' || argv[1][0] == 'o' ){
				if(argv[1][0] == 'u'){
					if (strchr(argv[1], 'r')) rwx += 0400;
					if (strchr(argv[1], 'w')) rwx += 0200;
					if (strchr(argv[1], 'x')) rwx += 0100;
				}
				if(argv[1][0] == 'g'){
					if (strchr(argv[1], 'r')) rwx += 0040;
					if (strchr(argv[1], 'w')) rwx += 0020;
					if (strchr(argv[1], 'x')) rwx += 0010;
				}
				if(argv[1][0] == 'o'){
					if (strchr(argv[1], 'r')) rwx += 0004;
					if (strchr(argv[1], 'w')) rwx += 0002;
					if (strchr(argv[1], 'x')) rwx += 0001;
				}
				if(argv[1][1] == '-'){
					buf.st_mode &= ~(rwx);
				}
				if(argv[1][1] == '+'){
					buf.st_mode |= rwx;
				}
			}
	//		printf("%#lo\n", (unsigned long)buf.st_mode);
			chmod(rpath, (unsigned long)buf.st_mode);
		}
    } else {
	    perror(argv[0]);	
        ret = -2;
    }

    return (ret);
}

int cmd_quit(int argc, char **argv)
{
    exit(1);
    return 0;
}

void usage_help(void)
{
    printf("help <command>\n");
}

void usage_mkdir(void)
{
    printf("mkdir <directory>\n");
}

void usage_rmdir(void)
{
    printf("rmdir <directory>\n");
}

void usage_cd(void)
{
    printf("cd <directory>\n");
}

void usage_mv(void)
{
    printf("mv <old_name> <new_name>\n");
}
void usage_ls(void)
{
    printf("ls -al\n");
}
void usage_ln(void)
{
	printf("hard link : ln <original_file> <new_file>\n");
	printf("symbolic link : ln -s <original_file> <new_file>");
}
void usage_cat(void)
{
	printf("cat <file>");
}
void usage_cp(void)
{
	printf("cp <original_file> <new_file>");
}
void usage_chmod(void)
{
	printf("chmod <permission> <filename>\n");
	printf("permission : ex) 0644 : rw-r--r--\n");
	printf("permission : ex) u+rwx : user에만 rwx 권한 추가\n");
	printf("permission : ex) u-x : user에만 x 권한 제거, g+x : 그룹에만 x권한 추가, o-rw : 기타권한에 rw 제거\n");
	printf("+rw : user, group, other 모두 rw 권한 추가\n"); 
}
void usage_rm(void)
{
	printf("rm <filename>");
}





//24)##을 하면 선언을 해줌 반복 줄임. 매크로 함수. 전처리

// help : 전체 명령어 리스트와 간단한 코멘트
// help <command> : ~~에 있는 상세 도움말 출력
// 46) 상수 하드 코딩 지양
// 매직 넘버가 나오면 멈출 수 있도록 상수를 정의할 수 있다.
// 속도에 대한 이슈가 없도록 매크로 상수.
// 최적화 구조, 알고리즘, 시스템적인 부분 공부할 것
// currnet_dir : 다른 곳에서도 쓰기 위해

// 138 : 현재 디렉터리와 chroot 패스와 같은지 비교 : 문자열 통으로 비교해도 되지만 길이로만 체크
// 포인터를 옮겨주어 길이만큼 더한다? 
// 루트면 /를 출력, 아니면 그 이후만 출력

// 151) : argc=1, argv : 인자값 들어감
// 160) 명령어 문자열을 통해 찾는다.
// cmd_mkdir :  argc가 들어온 경우
// real_path : 절대
// *stach 선언
// goto 나쁘지 않다! 써도 됨ㅋㅋ
// goto => ex) malloc에서 에러 체크할 때 바로 확인하기 위해 사용
// 라벨을 만들어서 점프한 다음 확인
// goto 안쓰면 더러워지는 경우가 있어서 가독성이 떨어질 수 있다.

// 74~84 : 절대경로 : fullpath에 유저 경로 넣게 함. 상대경로 : 현재 디렉토리 문자열을 넣어서 fullpath를 만들어줌
// static을 사용하는 이유 : 파일 전체에서 사용하기 위해서 
// 밖에서 호출하는 것만 아니면 파일내에서 사용하기 위해
// 
