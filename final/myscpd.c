/*
	Submitted by :	Jealani Shaik 
	course: 	CS590 to Dr.Lin.
	Date: 		7/23/2015.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define SOCKET_1 "ClientToServer"
#define SOCKET_2 "ServerToClient"
#define MYPORT 4096
#define BACKLOG 10

#define OPTK "Operation"
#define PUTTK "put"
#define GETTK "get"
#define FLNAMTK "FileName"
#define SZTK "Size"
#define PUTOPTYPE 1
#define GETOPTYPE 2
void sigchild_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}
int IsRemote(char* str)
{
	while(*str != '\0')
	{
	  if((*str == 58))
		return 1;
	  str++;
	}
	return 1;
}
void ParseMsg(char* msg, int* opType, char* fileName, long *fileSize)
{
	char* tok, *temp;
	tok = strtok(msg, ":");
	int counter = 0;
	while(tok != NULL)
	{
		switch(counter)
		{
			case 0:
				if((tok = strchr(tok, '=')) != NULL)
				{
					temp = tok+1;
					if(strcmp(temp, PUTTK) == 0)
						*opType = PUTOPTYPE;
					else
						*opType = GETOPTYPE;
				}
				break;
			case 1:
				if((tok = strchr(tok, '=')) != NULL)
				{
					temp = tok +1;
					strcpy(fileName, temp);
				}
				break;
			case 2:
				if((tok = strchr(tok, '=')) != NULL)
				{
					temp = tok +1;
					*fileSize = atoi(temp);
				}
				break;
		}	
		tok = strtok(NULL, ":");
		counter++;
	}
	return;
}

int main(int argc, char** argv)
{
	int sockfd, new_fd;
	struct sockaddr_in myaddr, their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes  =1;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Create socket failed");
		exit(1);
	}
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		perror("Set socket options failed");
		exit(1);
	}
	printf("Socket creadted on port: %d\n", MYPORT);
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(MYPORT);
	myaddr.sin_addr.s_addr = INADDR_ANY;
	memset(&myaddr.sin_zero, '\0', 8);
	if(bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr)) == -1)
	{
		perror("binding on socket failed");
		exit(1);
	}
	if(listen(sockfd, BACKLOG) == -1)
	{
		perror("Failed to listen for connections");
		exit(1);
	}
	printf("Listening to incoming connections\n");
	sa.sa_handler = sigchild_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}
	while(1)
	{
		sin_size = sizeof(their_addr);
		if((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
		{	
			perror("Failed to accept connections");
			exit(1);
		}
		printf("server: File transfer request from %s \n", inet_ntoa(their_addr.sin_addr));
		pid_t child_pid;
		if((child_pid = fork()) < 0)
		{
			perror("failed to fork a process for transfer");
			exit(1);
		}else if(child_pid == 0)
		{
			char msg[256];
			int msgLen = 0;
			//we do not expect any error at this stage of program.
			if((msgLen = recv(new_fd, msg, 256, 0)) <= 0)
			{
				printf("Error recieving initialization message");
				exit(1);
			}
			msg[msgLen] = '\0';
			printf("recieved msg '%s' of size=%d\n", msg, msgLen+1);
			char fileName[128];
			int opType, bytesRead; long fileSize;
			ParseMsg(msg, &opType, fileName, &fileSize);
			FILE *fp;
			char bufToRead[256];
			if(opType == PUTOPTYPE)//Remote To Local
			{
				struct stat buf1;
				if(stat(fileName, &buf1) < 0)
				{
					perror("Cannot find local file");
					char errMsg[256];
					sprintf(errMsg, "File '%s' not found on server: \n", fileName);
					send(new_fd, "1", 1, 0); //indicating an error message
					send(new_fd, errMsg, strlen(errMsg), 1); 
					exit(1);
				}else
				{
					send(new_fd, "0", 1, 0); //indicating an goahead message
					sprintf(msg, "%s:%d", SZTK, (int)buf1.st_size);
					send(new_fd, msg, strlen(msg), 0);
		
				
					if((fp = fopen(fileName, "r")) == 0)
					{
						printf("Failed to open file: %s\n", fileName);
						exit(1);
					}
					while(!feof(fp))
					{
						bytesRead = fread(bufToRead, 1, 256, fp);
						send(new_fd, bufToRead, bytesRead, 0);
						if(bytesRead != 256)
							break;
					}
					int flags = 0;
					if(recv(new_fd, msg, 256, flags) > 0)
						printf("%s\n", msg);
					fclose(fp);
				}
			}else//Local To Remote
			{
				if((fp = fopen(fileName, "w")) == 0)
				{
					printf("Failed to create file: %s\n", fileName);
					exit(1);
				}
				while(fileSize > 0)
				{
					bytesRead = recv(new_fd, bufToRead, 256, 0);
					fwrite(bufToRead, 1, bytesRead, fp);
					if(bytesRead != 256)
						break;
					fileSize -= bytesRead;
				}
				sprintf(msg, "File transfer completed successfully\n");
				send(new_fd, msg, strlen(msg), 0);
				fclose(fp);
			}
			close(new_fd);
			exit(0);
		}else
		{
			if(waitpid(child_pid, NULL, 0) != child_pid)
				perror("Failed to wait for child to finish");
			close(new_fd);
		}
	return (0);
}
	
