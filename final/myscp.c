/*
	Submitted by :	Jealani Shaik 
	course: 	CS590 to Dr.Lin.
	Date: 		7/23/2015.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define MYPORT 4096
#define MAXDATASIZE 100
#define OPTK "Operation"
#define PUTTK "put"
#define GETTK "get"
#define FLNAMTK "FileName"
#define SZTK "Size"

void printHelp()
{
  printf("Usage: ./myscp <remotehost>:<remotefile> <localPath>\n");
  printf("Usage: ./myscp <localFile> <remotehost>:<remotefile>\n");
}
void GetRemoteServerAndFileName(char* input, char* srv, char* fileName)
{
	char* tok;
	if(strlen(input) <= 0)
		return;

	tok = strtok(input, ":");
	strcpy(srv, tok);
	tok = strtok(NULL, ":");
	strcpy(fileName, tok);

	return;
}
int IsRemote(char* str)
{
	while(*str != '\0')
  	{
		if(*str == 58)
			return 1;
		str++;
	}
	return 0;
}

int main(int argc, char** argv)
{
	int c;
	while((c = getopt(argc, argv, "?h")) != -1)
  	{
		switch(c)
  	  	{
			case 'h':
			case '?':
     			printHelp();
		  		exit(0);
	  			break;
    	}
  	}
  	if(argc != 3)
	{
	   printf("Invalid number of arguments\n");
		printHelp();
	   exit(0);
  	}
	int option = 4; //start with invalid option
	int arg1IsRemote = IsRemote(argv[1]);
	int arg2IsRemote = IsRemote(argv[2]);
	if(arg1IsRemote && arg2IsRemote)
		option = 4; //Mot supported
	else if(arg1IsRemote && !arg2IsRemote)
		option = 3; //Remote to Loca
	else if(!arg1IsRemote && arg2IsRemote)
		option = 2; //Local to Remote
	else if(!arg1IsRemote && !arg2IsRemote)
		option = 1; //local copy
	

	char serverName[60], remoteFileName[128];
	if(option == 1) //local copy
	{
		struct stat buf1, buf2;
		if(stat(argv[1],&buf1) == -1)
		{
			char err[128];
			sprintf(err, "cannot find the file/directory %s\n", argv[1]);
			perror(err);
			exit(1);
		}
		FILE *fileToRead, *fileToWrite;
		if((fileToWrite = fopen(argv[2], "w")) == 0)
		{
			perror("Cannot create destination file");
			exit(1);
		}
		if(S_ISDIR(buf1.st_mode))//if first arg is a directory then second arg better be a directory as well.
		{
			if(!S_ISDIR(buf2.st_mode))
			{
				perror("Destination is not a directory");
				exit(1);
			}
			//if we are here means that we are copying multiple files to a directory
			//Have to write code here - SJP	
		}

		if((fileToRead = fopen(argv[1], "r")) == 0)
		{
			printf("Failed to open file: %s\n", argv[1]);
			exit(0);
		}
		int sizeToRead = 256; int bytesRead = 0;
		char* bufToRead = (char*)malloc(sizeof(char)*sizeToRead);
		while(!feof(fileToRead))
		{
			bytesRead = fread(bufToRead, 1, sizeToRead-1, fileToRead);
			fwrite(bufToRead, 1, bytesRead, fileToWrite);
			if(bytesRead != (sizeToRead-1))
				break;
		}
		exit(0);
	}
	else if(option == 2) //Local to Remote
	{
		GetRemoteServerAndFileName(argv[2], serverName, remoteFileName);
		
  		char buf[256];
	  	int numbytes, sfd;
	  	struct sockaddr_in s_addr;
  		struct hostent *he;
	  	if((he = gethostbyname(serverName)) == NULL)
  		{
    		perror("Failed to resolve the server");
   	 	exit(1);
  		}
	
  		if((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	  	{
  		  perror("Socket creation failed");
	  	  exit(1);
  		}
	
	  	s_addr.sin_family = AF_INET;
  		s_addr.sin_port = htons(MYPORT);
	  	//s_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);//inet_addr("127.0.0.1");
  		s_addr.sin_addr = *((struct in_addr *)he->h_addr);
	  	memset(s_addr.sin_zero, '\0', 8);
  		if(connect(sfd, (struct sockaddr*)&s_addr, sizeof(s_addr)) == -1)
  		{
  		  perror("connect failed");
	  	  exit(1);
  		}
		struct stat buf1;
		if(stat(argv[1], &buf1) == -1)
		{
			char err[128];
			sprintf(err, "Local file: %s does not exist\n", argv[1]);
			perror(err);
			exit(1);
		}
		int msgSize = sprintf(buf, "%s=%s:%s=%s:%s=%d", OPTK, GETTK, FLNAMTK, remoteFileName,SZTK,(int)buf1.st_size);
		buf[msgSize] = '\0';

		printf("sending message '%s' of size: %d\n", buf, msgSize+1);
		if((numbytes = send(sfd, buf, msgSize+1, 0)) == -1)
		{
			perror("Failed to send Initialization message");
			exit(1);
		}
		FILE* fp;
		if((fp = fopen(argv[1], "r")) == 0)
		{
			printf("failed to open file: %s\n", argv[1]);
			exit(1);
		}
		int transSoFar = 0;
		time_t start, curr;
		time(&start);
		int fileSize = (int)buf1.st_size;
		while(!feof(fp))
		{
			time(&curr);
			msgSize = fread(buf, 1, 256, fp);
			transSoFar = transSoFar + msgSize;
			send(sfd, buf, msgSize, 0);
			if(((curr-start)%3) == 0)
				printf("Finshed transferring: %d%c...\n",((transSoFar*100)/fileSize),'%');
		}
		if(recv(sfd, buf, 256, 0) > 0)
			printf("%s\n", buf);
		else
			printf("File transfer failed\n");
		
		fclose(fp);
		close(sfd);			
	}
	else if(option == 3)	//Remote to Local
	{
		GetRemoteServerAndFileName(argv[1], serverName, remoteFileName);
		
  		char buf[256];
	  	int numbytes, sfd;
	  	struct sockaddr_in s_addr;
  		struct hostent *he;
	  	if((he = gethostbyname(serverName)) == NULL)
  		{
    		perror("Failed to resolve the server");
   	 	exit(1);
  		}
	
  		if((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	  	{
  		  perror("Socket creation failed");
	  	  exit(1);
  		}
	
	  	s_addr.sin_family = AF_INET;
  		s_addr.sin_port = htons(MYPORT);
	  	//s_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);//inet_addr("127.0.0.1");
  		s_addr.sin_addr = *((struct in_addr *)he->h_addr);
	  	memset(s_addr.sin_zero, '\0', 8);
  		if(connect(sfd, (struct sockaddr*)&s_addr, sizeof(s_addr)) == -1)
  		{
  		  perror("connect failed");
	  	  exit(1);
  		}
		
		int msgSize = sprintf(buf, "%s=%s:%s=%s:%s=%d", OPTK, PUTTK, FLNAMTK, remoteFileName,SZTK,0);
		buf[msgSize] = '\0';
		printf("sending message '%s' of size: %d\n", buf, msgSize+1);
		if((numbytes = send(sfd, buf, msgSize+1, 0)) == -1)
		{
			perror("Failed to send Initialization message");
			exit(1);
		}
		recv(sfd, buf, 128, 0);//will recieve 1-Error or 0-good
		if(atoi(buf) == 1)
		{
			recv(sfd, buf, 128, 0);
			printf("%s\n", buf);
			exit(1);
		}
		int fileSize = 0; int flags = 0;
		if((numbytes=recv(sfd, buf, 128, flags)) > 0) 
		{
			char* tok;
			tok = strtok(buf, ":");
			if(tok == NULL)
			{
				perror("Server is not responding...");
				exit(1);
			}else
			{
				tok = strtok(NULL, ":");
				fileSize = atoi(tok);
			}
			FILE* fp;
			if((fp = fopen(argv[2], "w")) == 0)
			{
				printf("failed to create/open file: %s for writing\n", argv[2]);
				exit(1);
			}
			int transSoFar = 0;
			time_t start, curr;
			time(&start);
			while(transSoFar < fileSize)
			{
				time(&curr);
				if((msgSize =recv(sfd, buf, 256, 0)) != 0)
				{
					fwrite(buf, 1, msgSize, fp);				
					transSoFar = transSoFar + msgSize;
				}
				if(((curr-start)%3) == 0)
					printf("Finshed transferring: %d%c...\n",(int)((transSoFar*100)/fileSize), '%');
			}
			sprintf(buf, "File transfer completed successfully\n");
			printf("%s", buf);
			send(sfd, buf, strlen(buf), 0);
			fclose(fp);
			close(sfd);
		}
	}else 
		printf("Opreation is not supported\n");

  	return 0;
}
