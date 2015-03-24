#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>

#define name "07562012052214"
#define passwd "12206318"

int getip(char* ip)
{
	int sfd, intr;
	struct ifreq buf[16];
	struct ifconf ifc;
	sfd = socket (AF_INET, SOCK_DGRAM, 0);
	if (sfd < 0)
		return -1;
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = (caddr_t)buf;
	if (ioctl(sfd, SIOCGIFCONF, (char *)&ifc))
		return -1;
	intr = ifc.ifc_len / sizeof(struct ifreq);
	while (intr-- > 0 && ioctl(sfd, SIOCGIFADDR, (char *)&buf[intr]));
	close(sfd);
	char* sip = inet_ntoa(((struct sockaddr_in*)(&buf[intr].ifr_addr))-> sin_addr);
	memcpy(ip, sip, strlen(sip));
	return 0;
}

int addrtoip(char* addr,char* ip)
{
	struct addrinfo *infop = NULL, hint;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	int ret = getaddrinfo(addr, "80", &hint, &infop);
	if(ret){
		printf("%s\n", gai_strerror(ret));
		return -1;
	}
	//	 不循环infop = infop->ai_next;只取第一个结果
	if(infop != NULL){
		struct sockaddr_in *sa = (struct sockaddr_in *)infop->ai_addr;
		char* src = inet_ntoa(sa->sin_addr);
		memcpy(ip, src, strlen(src));
		ip[strlen(src)] = '\0';
	}
	else
		return -1;
	return 0;
}

int urlrequest(char* url,int port,char* buf,int flag,char* post,char* cookie)
{
	char* p = strstr(url, "/");
	char* get = NULL;
	char addr[100];
	char ip[20];
	memset(buf,0,sizeof(char)*4);
	if (NULL != p){
		get = p;
		int l = strlen(url) - strlen(p);
		strncpy(addr, url, l);
		addr[l] = '\0';
	}
	else{
		get = "/";
		strcpy(addr, url);
	}
	printf("host:%s\nrequest:%s\n", addr, get);
	addrtoip(addr, ip);
	int socket_desc;
	struct sockaddr_in server;
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1){
		printf("create socket failed!\n");
		return -1;
	}
	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
		printf("connect failed!");
		close(socket_desc);
		return -1;
	}
	char request[2000] = "\0";
	if (flag == 0){
		char i[10];
		sprintf(i,"%d",port);
		strcat(request, "GET ");
		strcat(request, get);
		strcat(request," HTTP/1.1\r\nHost: ");
		strcat(request, addr);
		strcat(request, ":");
		strcat(request, i);
		if (NULL != cookie){
			strcat(request,"\r\nCookie: ");
			strcat(request, cookie);
		}
		strcat(request,"\r\nAccept-Encoding: deflate\r\n\r\n");
	}
	else if(flag == 1){
		char i[10];
		strcat(request, "POST ");
		strcat(request, get);
		strcat(request," HTTP/1.1\r\nHost: ");
		strcat(request, addr);
		strcat(request, ":");
		sprintf(i,"%d",port);
		strcat(request, i);
		if (NULL != cookie){
			strcat(request,"\r\nCookie: ");
			strcat(request, cookie);
		}
		strcat(request,"\r\nContent-Length: ");
		int size = strlen(post) - 1;
		sprintf(i,"%d",size);
		strcat(request,i);
		strcat(request,"\r\nContent-Type: application/x-www-form-urlencoded");
		strcat(request,"\r\nAccept-Encoding: deflate\r\n\r\n");
		strcat(request, post);
	}

	if(send(socket_desc, request, strlen(request), 0) < 0){
		printf("send get request failed!\n");
		close(socket_desc);
		return -1;
	}
	char tmp[1024];
	while(recv(socket_desc, tmp, sizeof(tmp), 0) > 0){
		strcat(buf, tmp);
	}
	close(socket_desc);
	return 0;
}

int checknetwork(char* wlanacip,char* cookie)
{
	char buf[3000];
	char *s,*start;
	int c;
	if (urlrequest("ip.taobao.com/service/getIpInfo.php", 80, buf, 0, NULL, NULL) == -1) return -1;
	s = "wlanacip=";
	start = strstr(buf, s);
	if (NULL == start)
		return 1;
	c = strcspn(start + strlen(s), "\n");
	strncpy(wlanacip, start + strlen(s), c);
	wlanacip[c] = '\0';
	printf("wlanacip:%s\n",wlanacip);


	if (urlrequest("enet.10000.gd.cn/sdxfailed.jsp", 10001, buf, 0, NULL, NULL) == -1) return -1;
	s = "Set-Cookie:";
	start = strstr(buf, s);
	if (NULL == start)
		return 1;
	c = strcspn(start + strlen(s), ";");
	strncpy(cookie, start + strlen(s), c);
	cookie[c] = '\0';
	printf("cookie:%s\nok!\n",cookie);
	return 0;
}

int login(char* wlanacip,char* cookie)
{
	char buf[3000];
	char ip[20];
	if (getip(ip) == -1) return -1;
	char post[200] = "userName1=";
	strcat(post, name);
	strcat(post, "&password1=");
	strcat(post, passwd);
	strcat(post, "&rand=0000&");
	strcat(post, "eduuser=");
	strcat(post, ip);
	strcat(post, "&edubas=");
	strcat(post, wlanacip);
	if (urlrequest("enet.10000.gd.cn/login.do", 10001, buf, 1, post, cookie) == -1) return -1;
	char* s = "success";
	char* start = strstr(buf, s);
	if (NULL != start)
		return 0;
	else
		return -1;
}

int main(int argc,char* argv[])
{
	printf("start:\n");
	int status;
	status = system("/etc/init.d/network restart");
	if(status < 0)
		printf("can't restart network!");
	else
		printf("network restart ok!");
	sleep(20);
	char wlanacip[20];
	char cookie[200];
	int state = checknetwork(wlanacip, cookie);
	switch(state){
	case 1:
		printf("online!\n");
		break;
	case 0:
		printf("login now!\n");
		if (login(wlanacip,cookie)==-1) printf("login failed!");
		else printf("login ok!");
		break;
	case -1:
		printf("failed!\n");
		break;
	}
	return 0;
}
