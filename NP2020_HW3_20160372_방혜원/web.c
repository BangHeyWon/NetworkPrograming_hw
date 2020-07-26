#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define BUF_SIZE 2048
#define SMALL_BUF 100

void* request_handler(void* arg);
void send_data(FILE* fp, char* ct, char*file_name);
char* content_type(char* file);
void send_error(FILE *fp);
void error_handling(char* message);
void read_childproc(int sig);

int main(int argc, char *argv[])
{

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_size;
    pid_t pid;
    struct sigaction act;
    socklen_t adr_sz;
    int str_len, state;

    if(argc!=2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    act.sa_handler=read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    state=sigaction(SIGCHLD, &act, 0);

    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock, 20)==-1)
        error_handling("listen() error");

    while(1)
    {
        adr_sz=sizeof(clnt_adr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
        if(clnt_sock==-1)
            continue;
        else 
            puts("new client connect...");
        pid=fork();
        
        if(pid==-1)
        {
            close(clnt_sock);
            continue;
        }
        if(pid==0)
        {
            
            close(serv_sock);
            request_handler(&clnt_sock);
	    
            close(clnt_sock);
            puts("client disconnected...");
            return 0;
        }
        else
            close(clnt_sock);
    }
    close(serv_sock);
    return 0;
}


void* request_handler(void *arg)
{
    int clnt_sock=*((int*)arg);
    char req_line[SMALL_BUF];
    FILE* clnt_read;
    FILE* clnt_write;

    char method[10];
    char ct[15];
    char file_name[30];

    clnt_read=fdopen(clnt_sock, "r");
    clnt_write=fdopen(dup(clnt_sock), "wb");
    fgets(req_line, SMALL_BUF, clnt_read);
    if(strstr(req_line, "HTTP/")==NULL)
    {
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return 0;
    }
    
    strcpy(method, strtok(req_line, " /"));
    char *ptr = strtok(NULL," /");
    if(!strcmp(ptr,"HTTP")) strcpy(file_name, "index.html");
    else strcpy(file_name, ptr);
    
    printf("file_name : %s\n", file_name);
    strcpy(ct, content_type(file_name));
    if(strcmp(method, "GET")!=0)
    {
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return 0;
    }
    fclose(clnt_read);
    send_data(clnt_write, ct, file_name);
}

void send_data(FILE* fp, char* ct, char* file_name)
{
    char protocol[]="HTTP/1.0 200 OK\r\n";
    char server[]="Server:Linux Web Server \r\n";
    char cnt_type[SMALL_BUF];
    char cnt_len[BUF_SIZE];

    char a;
    int success=0;
    int size;
    FILE* send_file;
    printf("%s\n", file_name);
    send_file=fopen(file_name, "rb");
    if(send_file==NULL)
    {
        send_error(fp);
        return;
    }
    
    fseek(send_file,0,SEEK_END);
    size = ftell(send_file);
    fseek(send_file, 0, SEEK_SET);

    sprintf(cnt_type, "Content-type:%s\r\n\r\n", ct);
    sprintf(cnt_len, "Content-length:%d\r\n",size);
    printf("Content-type : %s\n", ct);
    printf("Content-length : %d\n", size);

    /*header info*/
    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fflush(fp);

    /*request data */
    while(1){
	a = fgetc(send_file);
	if(feof(send_file))
	    break;
	fputc(a, fp);
	fflush(fp);
    }
    
    fflush(fp);
    fclose(fp);
    fclose(send_file);
}

char* content_type(char* file)
{
    char extension[SMALL_BUF];
    char file_name[SMALL_BUF];
    strcpy(file_name, file);
    strtok(file_name, ".");
    strcpy(extension, strtok(NULL, "."));
    if(!strcmp(extension, "html")||!strcmp(extension, "htm"))
        return "text/html";
    else if(!strcmp(extension, "png")||!strcmp(extension, "pn"))
	return "image/png";
    else if(!strcmp(extension, "jpg")||!strcmp(extension, "jp"))
	return "image/jpg";
    else if(!strcmp(extension, "jpeg")||!strcmp(extension, "jpe"))
	return "image/jpeg";
    else
        return "text/plain";
}

void send_error(FILE* fp)
{
    char protocol[]="HTTP/1.1 404 Not Found\r\n";
    char server[]="Server:Linux Web Server \r\n";
    char cnt_type[]="Content-type:text/html\r\n\r\n";
    int size;
    char file_name[30];
    char a;

    FILE* send_file;
    strcpy(file_name, "error.html");
    printf("%s\n", file_name);
    send_file=fopen(file_name, "rb");

    if(send_file==NULL)
    {
        char cnt_len[]="Content-length:137\r\n";
        char content[]="<html><head><title>NETWORK></title></head>"
	"<body><font size=+5><br>error occur!""</font></body></html>";
	fputs(protocol, fp);
    	fputs(server, fp);
    	fputs(cnt_len, fp);
    	fputs(cnt_type, fp);
    	fputs(content,fp);
    	fflush(fp);
    }

    else{
    	char cnt_len[BUF_SIZE];
    	fseek(send_file,0,SEEK_END);
    	size = ftell(send_file);
    	fseek(send_file, 0, SEEK_SET);

    	sprintf(cnt_len, "Content-length:%d\r\n",size);
    	printf("Content-length : %d\n", size);
   
    	/*header info*/
    	fputs(protocol, fp);
    	fputs(server, fp);
    	fputs(cnt_len, fp);
    	fputs(cnt_type, fp);
    	fflush(fp);

    	while(1){
		a = fgetc(send_file);
		if(feof(send_file))
	    	break;
		fputc(a, fp);
		fflush(fp);
    	}
    
    	fflush(fp);
    	fclose(fp);
    	fclose(send_file);    
    }
}


void read_childproc(int sig)
{
    pid_t pid;
    int status;
    
    while(1)
    {
	pid=waitpid(-1, &status, WNOHANG);
	
	if(pid<=0)
	    break;
        else
	    printf("removed proc id : %d \n", pid);
    }
}

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

