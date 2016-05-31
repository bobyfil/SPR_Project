#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>

struct files
{
	char **files_arr;
	int size;
};

struct user
{
	char *username;
	struct files files;
};

struct users
{
	struct user *users_arr;
	int users_arr_size;
};
struct users users;

void users_free()
{
	if(users.users_arr != NULL)
	{
		int i;
		for(i=0;i<users.users_arr_size;i++)
		{
			if(users.users_arr[i].username != NULL)
				free(users.users_arr[i].username);

			if(users.users_arr[i].files.files_arr != NULL)
			{
				int j;
				for(j=0;j<users.users_arr[i].files.size;j++)
				{
					free(users.users_arr[i].files.files_arr[j]);
				}
				free(users.users_arr[i].files.files_arr);
			}
		}
		free(users.users_arr);
	}
}

int user_check(char *username, int *index_before)
{
	int first = 0;
	int last = users.users_arr_size;
	int mid = (first+last)/2;
	int ret = 0;
	int cmp;
	while(first <= last)
	{
		cmp = strcmp(users.users_arr[mid].username, username);
		if(cmp == 0)
		{
			return 1;
		}
		else if(cmp<0)
		{
			first = mid +1;
		}
		else
		{
			last = mid -1;
		}
	}
	if(strcmp(users.users_arr[mid].username, username) > 0)
	{
		*index_before = mid - 1;
	}
	else
	{
		*index_before = mid;
	}
	return 0;
}

void user_insert(char *username, int index_before)
{
	struct user user;
	user.username = malloc(strlen(username) + 1);
	strcpy(user.username, username);
	user.files.size = 0;
	user.files.files_arr = NULL;
	users.users_arr = (struct user *)realloc(users.users_arr, (++users.users_arr_size) * sizeof(struct user));
	if(users.users_arr_size == 1)
	{
		users.users_arr[0] = user;
	}
	else
	{
		int i;
		for(i = users.users_arr_size - 2;i >= index_before + 1;i--)
		{
			users.users_arr[i+1] = users.users_arr[i];
		}
		users.users_arr[index_before+1] = user;
	}
}

void *handle_connection(void *socket)
{
	char buffer[512];
	int ret, bytes, received = 0, sockfd = (int)socket, username_set = 0;
	char *addr = NULL, *e, *username = NULL;
	do
	{
		bzero(buffer, sizeof(buffer));
		if((bytes = read(sockfd, buffer, sizeof(buffer)-1))<0)
		{
			perror("Error reading from socket");
			pthread_exit(NULL);
		}
		char *temp;
		if(username_set == 0 && bytes >0)
		{
			e=strchr(buffer, '\n');
			char *str;
			int len;
			if(e == NULL)
				len = strlen(buffer);
			else
				len =(int)( e - buffer);
			if(username != NULL)
				temp = realloc(username, strlen(username) + len + 1);
			else
				temp = realloc(username, len + 1);
			if(temp == NULL)
			{	
				if(username != NULL)
				{
					free(username);
					username = NULL;
				}
				puts("Realloc failed");
				pthread_exit(NULL);
			}
			username = temp;
			if(username != NULL)
				strncat(username, buffer, len + strlen(username) + 1);
			else
				strncpy(username, buffer, len);
			if(e != NULL)
			{
				username_set = 1;
				if(len < strlen(buffer))
				{
					if(addr != NULL)
						addr = realloc(addr, strlen(addr) + strlen(buffer) - len + 1);
					else
						addr = realloc(addr, strlen(buffer) - len + 1);
					if(addr != NULL)
						strncat(addr, buffer + len + 1, strlen(buffer) - len + strlen(addr));
					else
						strncpy(addr, buffer + len + 1, strlen(buffer) - len);

					
				}
			}
		}
		else if(bytes>0)
		{
			if(addr != NULL)
				temp = (char *)realloc((void *)addr, strlen(addr) + strlen(buffer) + 1);
			else
				temp = (char *)realloc((void *)addr, strlen(buffer) + 1);
			if(temp == NULL)
			{
				if(addr != NULL)
				{
					free(addr);
					addr = NULL;
				}
				perror("Realloc failed)");
				pthread_exit(NULL);
			}
			addr = temp;
			if(addr != NULL)
				strncat(addr, buffer, strlen(buffer) + strlen(addr));
			else
				strncpy(addr, buffer, strlen(buffer));
		}
	}while(bytes>0);
	//strncpy(username, username, strlen(username) -2);
	username[strlen(username)-1] = 0;
	
	int index_before;
	puts(username);
	puts(addr);
}
 
int start_server(int portno, int conn_cnt)
{
	int server_sockfd, client_sockfd;
	socklen_t clientlen;
	struct sockaddr_in serv_addr, cli_addr;
	pthread_t *threads = NULL;
	int threads_cnt = 0;

	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sockfd<0)
	{
		perror("Error opening socket");
		return 0;
	}
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if(bind(server_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
	{
		perror("Error on binding");
		return 0;
	}
	listen(server_sockfd, 5);
	puts("Listening..."); 
	int i =0;
	while(conn_cnt == 0 || (i++) < conn_cnt)
	{
		clientlen = sizeof(struct sockaddr_in);
		client_sockfd = accept(server_sockfd, (struct sockaddr *) &cli_addr, &clientlen);
		if(client_sockfd < 0)
		{
			perror("Error on accept");
			exit(1);
		}
		pthread_t *temp = (pthread_t *)realloc((void *)threads, (++threads_cnt) * sizeof(pthread_t));
		if(temp == NULL)
		{
			if(threads != NULL)
			{
				free(threads);
				threads = NULL;
			}
			perror("realloc failed");
			exit(1);
		}
		threads = temp;
		if(pthread_create(&threads[threads_cnt-1], NULL, handle_connection, (void *)client_sockfd))
		{
			puts("Error creating thread");
			exit(1);
		}
	}
	for(i=0;i<threads_cnt;i++)
	{
		if(pthread_join(threads[i], NULL))
		{
			puts("Error joining thread");
			exit(1);
		}
	}
	return 1;
}

int main(int argc, char *argv[])
{
	int portno, conn_cnt = 0;
	if(argc < 2)
	{
		puts("No port specified");
		exit(1);
	}
	portno = atoi(argv[1]);
	if(portno<=0)
	{
		puts("Invalid port no");
		exit(1);
	}
	if(argc > 2)
	{
		conn_cnt = atoi(argv[2]);
		if(conn_cnt<0) conn_cnt = 0;
	}
	users.users_arr_size = 0;
	if(start_server(portno, conn_cnt) == 0)
	{
		puts("Error starting server.");
		exit(1);
	}
	users_free();
	return 0;
}

