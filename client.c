#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<fcntl.h>
#define DEF_PORTNO 80
#define SEP "/"
#define PORT_SEP ":"
#define HTTP_MSG "GET %s HTTP/1.0\r\n\r\n"

int handle_link(char *link, char **h, char **p, char **f, int *port)
{
	char *host, *path, *filename;
	int portno;
	char *linktmp = (char*) malloc(strlen(link) + 1);
	strcpy(linktmp, link);
	char *token = strtok(linktmp, SEP);
	if(strlen(token) == strlen(link))
	{
		path = (char*)malloc(strlen(SEP) + 1);
		strcpy(path, SEP);
	}
	else
	{
		path = (char*)malloc(strlen(link) - strlen(token)+1);
		strcpy(path, link + strlen(token));
	}
	int flag = 0;
	char *temp;
	while((token = strtok(NULL, SEP))!=NULL)
	{
		temp = token;
		flag = 1;
	}
	filename = (char*)malloc(strlen(temp)+1);
	strcpy(filename, temp);
	if(flag ==0)
	{
		filename = NULL;
		if(linktmp != NULL)
			free(linktmp);
		if(token != NULL)
			free(token);
		if(path!=NULL)
			free(path);
		return 0;
	}
 	token = strtok(linktmp, PORT_SEP);
	host = (char*)malloc(strlen(token)+1);
	strcpy(host,token);
	token = strtok(NULL, PORT_SEP);
	if(token == NULL)
	{
		portno = DEF_PORTNO;
	}
	else
	{
		portno = atoi(token);
		free(token);
	}
	free(linktmp);
	*h = host;
	*p = path;
	*f = filename;
	*port = portno;
	return 1;
}
int download_file(char *link)
{
	int sockfd, bytes, sent, total, filefd, portno = 0;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	char *filename = NULL;
    char buffer[1024];
    char *message, *path = NULL, *host = NULL;
	//char *buffer = malloc(1024);
	if(handle_link(link, &host, &path, &filename, &portno) == 0)
	{
		puts("Invalid link!");
		if(host != NULL)
			free(host);
		if(path != NULL)
			free(path);
		if(filename != NULL)
			free(filename);
		exit(1);
	}
	message = (char*)malloc(strlen(HTTP_MSG) + strlen(path) -1);
	sprintf(message, HTTP_MSG, path); 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0)
    {
        perror("Error opening socket.");
        exit(1);
    }
	
    server = gethostbyname(host);
    if(server == NULL)
    {  
        perror("Error, no such host.");
        exit(1);
    } 
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {   
        perror("Error connecting.");
        exit(1);
    }
    total = strlen(message);
    sent=0;
    do
	{
		bytes = write(sockfd, message+sent, total-sent);
		if(bytes<0)
		{	
        	perror("Error writing message to socket.");
        	exit(1);
		}
		else if(bytes == 0)
			break;
		else
			sent+=bytes;
    }while(sent<total);
	if(access(filename, F_OK) ==0)
	{
		remove(filename);
	}
	puts(filename);
	filefd = open(filename, O_CREAT|O_APPEND|O_WRONLY);
	if(filefd<0)
	{
		perror("File cannot be opened.\n");
		exit(1);
	}
	int clear = 0;
	char *str = (char*)malloc(4);
	int w=0;
    do
	{
		bzero(buffer, sizeof(buffer));
		bytes = read(sockfd,buffer, sizeof(buffer) -1);
		if(bytes<0)
		{	
        	perror("Error reading respoce from socket.");
        	exit(1);
		}
		else if(bytes>0)
		{
			if(clear == 0)
			{
				//puts(buffer);
				//puts("\n===================================================\n");
				int i;				
				for(i=0; i<strlen(buffer)-4;i++)
				{
					strncpy(str, buffer+i, 4);
					if(strcmp(str, "\r\n\r\n") == 0)
					{						
						bytes-=(i+4);
						memcpy(buffer,buffer+i+4, bytes);
						//puts(buffer);
						clear = 1;
						break;
					}
				}
				
				//strncpy(str, buffer, 4);
				//printf("%s\n", str);
			}
			if((w = write(filefd, buffer, bytes))<0)
			{
				perror("Error writing to file.");
				exit(1);
			}
		}
    }while(bytes>0);
	close(sockfd);
	close(filefd);
	free(str);
	return 1;
}

int check_with_server(char *hostname, int port, char *username, char *addr)
{
	int sockfd, bytes;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char buffer[512];
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
	{
		perror("Error opening socket");
		exit(1);
	}

	server = gethostbyname(hostname);
	if(server == NULL)
	{
		puts("Invalid host");
		exit(1);
	}
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port=htons(port);
	if(connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
	{
		perror("Error connecting to server");
		exit(1);
	}
	char *message = malloc(strlen(addr) + strlen(username) + 2);
	strcpy(message, username);
	strncat(message, "\n", 2 + strlen(message));
	strncat(message, addr, strlen(addr) + strlen(message)+ 1);
	int total = strlen(message), sent=0;
	do
	{
		bytes = write(sockfd, message+sent, total-sent);
		if(bytes<0)
		{
			perror("Error sending message to server");
			exit(1);
		}
		else if(bytes==0)
			break;
		else
			sent += bytes;
	}while(sent<total);
	return 0;
}

int main(int argc, char *argv[])
{
    int serverPortNo;
    //char *host = "www.tu-sofia.bg";
    char *link;
    //char *message = "GET /img/TU_Logo_Mery.png HTTP/1.0\r\n\r\n";
    //char *message = "GET / HTTP/1.0\r\n\r\n";
	if(argc<5)
	{
		perror("You should provide 4 parameters(server hostname, port, username, web address)");
	}
	serverPortNo = atoi(argv[2]);
	if(serverPortNo <=0)
	{
		puts("Invalid port");
		exit(1);
	}
	//printf("%s\n%s\n%s\n%d", argv[1], argv[2], argv[3], argc);
	link =(char*)malloc(strlen(argv[4]) + 1);
	strcpy(link, argv[4]);
	if(check_with_server(argv[1], serverPortNo, argv[3], link))
	{
		if(download_file(link))
			printf("File saved!\n");
	}
	else
		puts("File already downloaded");

	return 0;
}
