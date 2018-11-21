#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT 80 //設定port為80
#define BUFSIZE 8096

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpeg"},
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{"exe","text/plain" },
	{0,0} };

void handle_socket(int fd)
{
	int j, file_fd, buflen, len;
	long i, ret;
	char * fstr;
	static char buffer[BUFSIZE+1];

	ret = read(fd,buffer,BUFSIZE); //讀取瀏覽器要求

	if(ret==0||ret==-1) { //連線出現問題，結束行程
		exit(3);
	}

	if(ret>0&&ret<BUFSIZE) //處理字串格式(字串結尾)
		buffer[ret] = 0;
	else
		buffer[0] = 0;

	for(i=0;i<ret;i++) 
		if(buffer[i]=='\r'||buffer[i]=='\n')
			buffer[i] = 0;

	if(strncmp(buffer,"GET ",4)&&strncmp(buffer,"get ",4)) //判斷是否GET
		exit(3);
	
	for(i=4;i<BUFSIZE;i++) { //隔開"HTTP/1.0"
		if(buffer[i] == ' ') {
			buffer[i] = 0;
			break;
		}
	}

	if(!strncmp(&buffer[0],"GET /\0",6)||!strncmp(&buffer[0],"get /\0",6) ) //當客戶端要求根目錄時讀取 index.html
		strcpy(buffer,"GET /index.html\0");

	buflen = strlen(buffer); //檢查客戶端所要求的檔案格式
	fstr = (char *)0;

	for(i=0;extensions[i].ext!=0;i++) {
		len = strlen(extensions[i].ext);
		if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
			fstr = extensions[i].filetype;
			break;
		}
	}

	if(fstr == 0) { //檔案格式不支援
		fstr = extensions[i-1].filetype;
	}

	if((file_fd=open(&buffer[5],O_RDONLY))==-1) //開檔
		write(fd, "Failed to open file", 19);

	sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr); //回傳200與格式給瀏覽器
	write(fd,buffer,strlen(buffer));

	while((ret=read(file_fd, buffer, BUFSIZE))>0) { //讀檔並印於客戶端的瀏覽器
		write(fd,buffer,ret);
	}
	
	exit(1);
}

void sigchld(int signo)
{
	pid_t pid;
	
	while((pid=waitpid(-1, NULL, WNOHANG))>0);
}

int main(int argc, char **argv)
{
	int listenfd, socketfd;
	pid_t pid;
	socklen_t length;
	struct sockaddr_in cli_addr, serv_addr;
	

	if(fork() != 0) //背景執行
		return 0;


	listenfd = socket(AF_INET, SOCK_STREAM, 0); //開啟網路socket

	bzero(&serv_addr, sizeof(serv_addr));//接口訊息 網路連線設定
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT); //設定為define的port
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //使用任何在本機的對外IP

	bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)); //開啟網路監聽器

	listen(listenfd, 64); //開始監聽

	signal(SIGCHLD, sigchld);

	while(1) {
		length = sizeof(cli_addr);

		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length))<0) //accept:等待客戶端連線
			exit(3);

		if((pid = fork()) < 0) { //執行fork() 分出子行程
			exit(3);
		}else {
			if(pid == 0) { //此為子行程
				close(listenfd);
				handle_socket(socketfd);
				exit(0);
			}else { //此為父行程
				close(socketfd);
			}
		}
	}
	return 0;
}
