#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <pthread.h>
int process(int);
int forward(int,int);
int main()
{
	int sock_server = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	int optval=1;
	setsockopt(sock_server,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
	struct sockaddr_in serv_addr,dest_addr;
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port=htons(8888);
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	bind(sock_server,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
	listen(sock_server,10);
	memset(&dest_addr,0,sizeof(dest_addr));
	while(1)
	{
		int len=sizeof(struct sockaddr_in);
		int new=accept(sock_server,(struct sockaddr*)&dest_addr,&len);
		len=1;
		setsockopt(sock_server,SOL_TCP,TCP_NODELAY,&len,sizeof(int));
		int f;
		if((f=fork())==0)
			process(new);
	}
}

int process(int new)
{
	char buf[256];
	read(new,buf,2);
	int version = (int)buf[0];
	if(version ==5)
		printf("version 5 ");
	int len=(int)buf[1];
	read(new,buf,len);
	buf[0]=0x05;
	buf[1]=0x00;
	write(new,buf,2);
	read(new,buf,4);
	int atyp=(int)buf[3];
	int new_socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	struct sockaddr_in dest_addr;
	switch(atyp)
	{
		case 0x01:
			char *ip_str=malloc(4);
			read(new,ip_str,4);
			char *ip=malloc(16);
			sprintf(ip,"%hhu.%hhu.%hhu.%hhu",ip_str[0],ip_str[1],ip_str[2],ip_str[3]);
			printf(" %s  ",ip);
			unsigned short port;
			read(new,&port,2);
			printf("%hd\n",ntohs(port));
			memset(&dest_addr,0,sizeof(struct sockaddr_in));
			dest_addr.sin_family=AF_INET;
			dest_addr.sin_port=port;
			dest_addr.sin_addr.s_addr=inet_addr(ip);
			connect(new_socket,(struct sockaddr*)&dest_addr,sizeof(dest_addr));
			break;
		case 0x03:
			char *domain_len=malloc(1);
			read(new_socket,domain_len,1);
			char *domain=malloc((int)(*domain_len));
			read(new_socket,domain,(int)(*domain_len));
			struct hostent *destination=gethostbyname(domain);
			struct in_addr *p=*(destination->h_addr_list);
			unsigned short p_port;
			read(new,&p_port,2);
			struct sockaddr_in destination_addr;
			memset(&destination_addr,0,sizeof(struct sockaddr_in));
			destination_addr.sin_family = AF_INET;
			destination_addr.sin_port=p_port;
			destination_addr.sin_addr=*p;
			connect(new_socket,(struct sockaddr*)&destination_addr,sizeof(destination_addr));
			break;
	}
	buf[0]=0x05;
	buf[1]=buf[2]=0x00;
	buf[3]=0x01;
	buf[4]=buf[5]=buf[6]=buf[7]=buf[8]=buf[9]=0;
	write(new,buf,10);
	forward(new,new_socket);
}

int forward(int a,int b)
{
	char *buf=malloc(500);
	char *buffer=malloc(500);
	int p,n;
	while(1)
	if((p=fork())==0)
	{
		n=read(a,buf,500);
		write(b,buf,n);
	}
	else
	{
		n=read(b,buf,500);
		write(a,buf,n);
	}
}
