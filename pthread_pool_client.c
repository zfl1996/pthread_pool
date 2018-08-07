#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define	SERVER_IP	"192.168.12.136"
#define	SERVER_PORT	8000	
int main(void)
{
		struct sockaddr_in serveraddr,clientaddr;
		int sockfd;
		bzero(&serveraddr,sizeof(serveraddr));
		serveraddr.sin_family = AF_INET; //指定协议族 ipv4协议
		serveraddr.sin_port = htons(SERVER_PORT); //小端转大端端口号
		inet_pton(AF_INET,SERVER_IP,&serveraddr.sin_addr.s_addr);
		sockfd = socket(AF_INET,SOCK_STREAM,0);
		char buf[1500];
		/*客户端请求,接收服务器响应,将结果打印到标准输出*/
		connect(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));
		while((fgets(buf,sizeof(buf),stdin))!=NULL)
		{
				write(sockfd,buf,strlen(buf));
				read(sockfd,buf,sizeof(buf));
				write(STDOUT_FILENO,buf,strlen(buf));
				bzero(buf,sizeof(buf));
		}
		close(sockfd);
		return 0;
}
