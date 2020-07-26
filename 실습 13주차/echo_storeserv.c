#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 100
void error_handling(char *message);
void read_childproc(int sig);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int fds[2];

    pid_t pid;
    struct sigaction act;
    socklen_t adr_sz;
    int str_len, state;
    char buf[BUF_SIZE];
    if(argc!=2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    act.sa_handler=read_childproc;//시그널 핸들러를 등록
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    state=sigaction(SIGCHLD, &act, 0);//시그널 핸들러를 등록

    serv_sock=socket(PF_INET, SOCK_STREAM, 0);//소켓생성
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock, 5)==-1)//서버 소켓을 리슨상태로 기다리게 함.
        error_handling("listen() error");

    pipe(fds);//파이프 생성 
    pid = fork();//자식 프로세스 생성 
    if(pid==0)//서버 구동하고서 처음 생기는 자식 프로세스
    {
	FILE* fp=fopen("echomsg.txt", "wt");
	char msgbuf[BUF_SIZE];
	int i, len;

	for(i = 0; i < 10; i++)
	{
	    len = read(fds[0], msgbuf, BUF_SIZE);//fds[0]에서 값을 읽어온다. 
	    fwrite((void*)msgbuf, 1, len, fp);//fp는 echomsg.txt를 가리킨다.
	}
	fclose(fp);
	return 0;
    }

    while(1)
    {
        adr_sz=sizeof(clnt_adr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);//클라이언트 연결이 오기를 기다렸다가 연결을 함.
        if(clnt_sock==-1)
            continue;
        else 
            puts("new client connecte..");
        pid=fork();//새로운 자식 생성
        if(pid==0)//자식은 부모의 파이프를 공유할 수 있다.(자식 생성 전에 파이프가 생성되어있었으므로)
        {
            close(serv_sock);
            while((str_len=read(clnt_sock, buf, BUF_SIZE))!=0)
	    {
                write(clnt_sock, buf, str_len);
		write(fds[1], buf, str_len);
	    }

            close(clnt_sock);
            puts("client disconnected..");
            return 0;
        }
        else
            close(clnt_sock);
    }
    close(serv_sock);
    return 0;
}

void read_childproc(int sig)//시그널 번호를 받아서 어떤 자식이 죽었기 때문에 바로 리턴해주고 자식의 pid를 출력해준다. exit_status값을 보면 왜 죽었는지도 알 수 있다. 
{
    pid_t pid;
    int status;
    pid=waitpid(-1, &status, WNOHANG);
    printf("removed proc id : %d \n", pid);
}
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
