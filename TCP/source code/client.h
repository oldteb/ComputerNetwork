//  client.h
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "queue.h"

#define     TRUE               1
#define     FALSE              0
#define     CLTCOMMDSIZE       5
#define	    SRVCOMMDSIZE       5

#define MAXBUF 2048


int processHelp();
int processUpload();
int processDownload();
int processFilelist();
int processQuit();

//int DataLinkSend(int i, char *buffer, int buf_length);
int add_segheader();

char* ClientCommands[CLTCOMMDSIZE] = {"HELP","UPLOAD","DOWNLOAD","FILELIST","QUIT"};	//commands of user
char* ServerCommands[SRVCOMMDSIZE] = {"#"};	// commands from server

char buffer[MAXBUF];
int len;
char* pPos = NULL;

extern int clientfd;


#define progress_h
#define PROGRESS_NUM_STYLE 0
#define PROGRESS_CHR_STYLE 1
#define PROGRESS_BGC_STYLE 2
	 
typedef struct {
	char chr;       /*tip char*/
	char *title;    /*tip string*/
	int style;      /*progress style*/
	int max;        /*maximum value*/
	float offset;
	char *pro;
} progress_t;
	 
extern void progress_init(progress_t *, char *, int, int);
	 
extern void progress_show(progress_t *, float);
	 
extern void progress_destroy(progress_t *);

extern void progress_init(progress_t *bar, char *title, int max, int style)
{
	bar->chr = '#';
	bar->title = title;
	bar->style = style;
	bar->max = max;
	bar->offset = 100 / (float)max;
	bar->pro = (char *) malloc(max+1);
	if ( style == PROGRESS_BGC_STYLE )
		memset(bar->pro, 0x00, max+1);
	else {
		memset(bar->pro, 32, max);
		memset(bar->pro+max, 0x00, 1);
	}
}

extern void progress_show( progress_t *bar, float bit )
{
	int val = (int)(bit * bar->max);
	switch ( bar->style )
	{
		case PROGRESS_NUM_STYLE:
			printf("\033[?25l\033[31m\033[1m%s%d%%\033[?25h\033[0m\r",
			bar->title, (int)(bar->offset * val));
			fflush(stdout);
			break;
		case PROGRESS_CHR_STYLE:
			memset(bar->pro, '#', val);
			printf("\033[?25l\033[31m\033[1m%s[%-s] %d%%\033[?25h\033[0m\r",
			bar->title, bar->pro, (int)(bar->offset * val));
			//printf("\033[?25l%s[%-s] %d%%",bar->title, bar->pro, (int)(bar->offset * val));
			fflush(stdout);
			break;
		case PROGRESS_BGC_STYLE:
			memset(bar->pro, 32, val);
			printf("\033[?25l\033[31m\033[1m%s\033[41m %d%% %s\033[?25h\033[0m\r",
			bar->title, (int)(bar->offset * val), bar->pro);
			fflush(stdout);
			break;
	}
}
	 
//destroy the the progress bar.
extern void progress_destroy(progress_t *bar)
{
	free(bar->pro);
}

int ClientCmdRecognize(char* buffer, int nbytes){ // recongnize the input commands
	int i;
	char commd[100] = {0};
	strncpy(commd,buffer,nbytes);
	commd[nbytes] = '\0';
	for(i=0;i<CLTCOMMDSIZE;i++){
		if(strcmp(commd,ClientCommands[i])==0)  
			break;
	}
	return i;
}

int ServerCmdRecognize(char* buffer, int nbytes){ // recongnize the commands received from server
	int i;
	char commd[100] = {0};
	strncpy(commd,buffer,nbytes);
	commd[nbytes] = '\0';
	for(i=0;i<SRVCOMMDSIZE;i++){
		if(strcmp(commd,ServerCommands[i])==0)  
			break;
        }
	return i;
}

int processHelp()
{
	memset(buffer,0,sizeof(buffer));
	strcpy(buffer, "#HELP");		
	//len = send(sockfd, buffer, strlen(buffer), 0);
	len = DataLinkSend(clientfd, buffer, strlen(buffer));
	memset(buffer,0,sizeof(buffer));
	//len = recv(clientfd, buffer, MAXBUF, 0);
	len = DataLinkRecv(clientfd, buffer, 1024);
	printf("%s\n",buffer);
	return 0;
}

int getFileSize(char * strFileName)   
{
	FILE * fp = fopen(strFileName, "r");  
	fseek(fp, 0L, SEEK_END);  
	int size = ftell(fp);  
	fclose(fp);  
	return size;  
}

int processUpload()
{
	memset(buffer,0,sizeof(buffer));
	strcpy(buffer, "#UPLOAD");		
	//len = send(clientfd, buffer, strlen(buffer), 0);
	len = DataLinkSend(clientfd, buffer, strlen(buffer));
	FILE * fp;
	int act_read = 0;
	int file_size = 0;
	char file_name[MAXBUF];
	
	printf("Upload files to server, please input filename:\n");
	gets(buffer);		
	//len = send(sockfd, buffer, strlen(buffer), 0);
	
	//memset(file_name,0,sizeof(file_name));
	strcpy(file_name,buffer);
 	
	if((fp = fopen(file_name, "r")) == NULL){
		fclose(fp);
		printf("File not found!");
		return 0;
	}
	else{
		//  File existed...
		file_size = getFileSize(file_name);	//get file size
		sprintf(buffer, "%d", file_size);	//convert (file_size)int to char
		printf("File_size:%sbytes\n", buffer);	
		strcat(file_name, "#");
		strcat(file_name, buffer);
		//send(clientfd, file_name, strlen(file_name), 0);
		DataLinkSend(clientfd, file_name, strlen(file_name));
		memset(buffer, 0, sizeof(buffer));
		sleep(1);
		do{
			act_read = fread(buffer, sizeof(char), sizeof(buffer), fp);
			if(act_read == 0){
				break;
			}
			else if(act_read < sizeof(buffer)){
				//write(clientfd, buffer, act_read);
				DataLinkSend(clientfd, buffer, act_read);
				break;
			}		
			else{
				//write(clientfd, buffer, act_read);
				DataLinkSend(clientfd, buffer, act_read);
				memset(buffer, 0, sizeof(buffer));
			}
			continue;
		}while(TRUE);
		printf("File transfer end.\n");
		fclose(fp);
	}
	return 0;
}

int processDownload()
{
	memset(buffer,0,sizeof(buffer));
	strcpy(buffer, "#DOWNLOAD");		
	//len = send(clientfd, buffer, strlen(buffer), 0);
	DataLinkSend(clientfd, buffer, strlen(buffer));
	/*memset(buffer,0,sizeof(buffer));
        len = recv(sockfd, buffer, MAXBUF, 0);	//receive filelist into the buffer
	printf("file list: '%s'\n", buffer);*/
       	
	printf("input file name:");	//input filename and send the filename to server
	memset(buffer,0,sizeof(buffer));
	gets(buffer);
	//len = send(clientfd, buffer, strlen(buffer), 0);
	len = DataLinkSend(clientfd, buffer, strlen(buffer));

	char file_name[MAXBUF];
	memset(file_name,0,sizeof(file_name));
	strcpy(file_name,buffer);
	       	
	memset(buffer,0,sizeof(buffer));
	//len = recv(clientfd, buffer, MAXBUF, 0);		//receive filesize into the buffer	
	len = DataLinkRecv(clientfd, buffer, MAXBUF);
	int file_size = atoi(buffer);		//file_size = filesize, convert char filesize to int
	printf("file name:%s,file size:%dbytes\n",file_name,file_size);

	FILE *fp = fopen(file_name, "w"); 
	if (fp == NULL)
	{
		printf("File: %s Can Not Open To Write!", file_name); 
		exit(1); 
	}
        
        // buffer stores the content of file 
	memset(buffer,0,sizeof(buffer));
	int length = 0;
	len = 0 ;

	progress_t bar;
	progress_init(&bar, "", 50, PROGRESS_BGC_STYLE);

	do{
		//len = recv(clientfd, buffer, MAXBUF, 0);
		len = DataLinkRecv(clientfd, buffer, MAXBUF);
		printf("-%c\n", buffer[len-3]);
		length+= len;
		int write_length = fwrite(buffer, sizeof(char), len, fp);
		int file_percent = length*100/file_size;
		//progress_show(&bar, (float)length/(float)file_size);
		printf("%d\n", length);
		if(length >= file_size)
			break;
	}while(TRUE);
	progress_destroy(&bar);
	printf("\nRecieve File: %s From Server Finished!\n", file_name); 
	// close the file pointer  
	fclose(fp);
	return 0;
}

int processFilelist()
{
	memset(buffer,0,sizeof(buffer));
	strcpy(buffer, "#FILELIST");		
	//len = send(clientfd, buffer, strlen(buffer), 0);
	len = DataLinkSend(clientfd, buffer, strlen(buffer));
	memset(buffer,0,sizeof(buffer));
	//len = recv(clientfd, buffer, MAXBUF, 0);
	len = DataLinkRecv(clientfd, buffer, MAXBUF);
	printf("Filelist on server:\n%s\n",buffer);
	return 0;
}

int processQuit()
{
	memset(buffer,0,sizeof(buffer));
	strcpy(buffer, "#QUIT");		
	//len = send(clientfd, buffer, strlen(buffer), 0);
	len = DataLinkSend(clientfd, buffer, strlen(buffer));
	printf("Quit from server...\n");
	exit(0);
	
}

// int DataLinkSend(int i, char *buffer, int buf_length)
// {
// 	//char *sendbuffer = *buffer;
// 	int len = send(i, buffer, strlen(buffer), 0);
// 	return len;
// }

int add_segheader()
{
	//char head = "#head#";
	char h[] = "#head#";
	char* header = h; 
	
	//strcat(header, buffer);
	char* segment;
	while(*header!='\0'){
		*segment++ = *header++;	
	}
	//while(*buffer!='\0'){
	//	*segment++ = *buffer++;	
	//}
	*segment = '\0';	
}
