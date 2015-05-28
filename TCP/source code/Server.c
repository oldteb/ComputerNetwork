#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include "dllgen.h"

#define         SIZE_SNDBUF     100
#define         SIZE_RCVBUF     1024
#define         CLTCOMMDSIZE    5
#define         progress_h
#define         PROGRESS_NUM_STYLE 0
#define         PROGRESS_CHR_STYLE 1
#define         PROGRESS_BGC_STYLE 2
          

char* CltCommands[CLTCOMMDSIZE] = {"#HELP","#UPLOAD","#DOWNLOAD","#FILELIST","#QUIT"};

int RequestHandler(char* ,int );
int ClientCmdRecognize(char* ,int );
int processHelp(char* ,int );
int processUpload(char* ,int );
int processDownload(char* ,int );
int processFilelist(char* ,int );
int processQuit(char* ,int );
int isFileExisted(char *);
int transfer(char * fname);
int getFileSize(char * strFileName);
void progress_init(progress_t *, char *, int, int);     
void progress_show(progress_t *, float);    
void progress_destroy(progress_t *);
extern void DLL_Init();



int main(){
    int sockfd,len;
    int msg_len = 0;
    int on;
    struct sockaddr_in server_addr,client_addr;
    char buffer[SIZE_RCVBUF];

    char temp[1024] = {0};
    int temp_len = 0;

    //usleep(100000);

    DLL_Init();

    //printf("%d %d\n", dll_scb.nextsend, dll_scb.last);
    //strcpy(dll_scb.DLL_buffer, "thread test!\n");

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0){
        perror("create socket error. -> ");
        exit(1);
    }

    //keep using socket after closesocket function is invoked.  
    on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(8888);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(sockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr))<0){
        perror("bind socket error. -> ");
        exit(2);
    }

    listen(sockfd,5);
    len=sizeof(struct sockaddr);
   
    
    for(;TRUE ;msg_len = 0){
        printf("waiting for new connection:\n");
        clientfd=accept(sockfd,(struct sockaddr *)&client_addr,&len);
        /* recv(clientfd,buffer,256,0);*/
        while(TRUE){
            //temp_len = DataLinkRecv(clientfd, temp, 1024);
            //printf("hohoho:%s, %d\n", temp, temp_len);

            //continue;
            memset(buffer,0,sizeof(buffer));
            printf("waiting user command:\n");
            //msg_len = read(clientfd,buffer,SIZE_RCVBUF);
            msg_len = DataLinkRecv(clientfd,buffer,SIZE_RCVBUF);

            if(msg_len == 0){
                break;
            }

            printf("server received : %s\n",buffer);
            if(!strcmp(buffer, "#QUIT")){
                break;
            }
            else if(buffer[0] == '#'){
                RequestHandler(buffer, msg_len);
            }
            else{
                //write(clientfd, "#INVLD",7);
                DataLinkSend(clientfd, "#INVLD", 7);
            }   
        }
        close(clientfd);

    }

    close(sockfd);

    return 0;
}


int RequestHandler(char * buffer, int msg_len){
    switch(ClientCmdRecognize(buffer,msg_len)){
        case 0:
            //HELP
            processHelp(buffer, msg_len);
            break;
        case 1:
            //UPLOAD
            processUpload(buffer, msg_len);
            break;
        case 2:
            //DOWNLOAD
            processDownload(buffer, msg_len);
            break;
        case 3:
            //FILELIST
            processFilelist(buffer, msg_len);
            break;
        case 4:
            //QUIT
            processQuit(buffer, msg_len);
            break;

        default:
            //write(clientfd, "#INVLD",7);
            DataLinkSend(clientfd, "#INVLD",7);
            break; 
    }

    return 0;
}

int ClientCmdRecognize(char* buffer, int message_length){
    int i;
    char commd[SIZE_RCVBUF] = {0};

    strncpy(commd,buffer,message_length);
    commd[message_length] = '\0';
    //printf("%s\n", commd);
    for(i=0;i<CLTCOMMDSIZE;i++){
        if(strcmp(commd,CltCommands[i])==0)  break;
    }

    return i;
}


int processHelp(char * buffer, int msg_len){
    char rsp_buffer[SIZE_SNDBUF] = {0};
    strncpy(rsp_buffer, buffer, msg_len);
    strcat(rsp_buffer, "_ACK");

    //write(clientfd, rsp_buffer,strlen(rsp_buffer));
    DataLinkSend(clientfd, rsp_buffer, strlen(rsp_buffer));

    return 0;
}

int processUpload(char * buffer, int msg_len){
    int length = 0;
    int len = 0;
    int fsize = 0;
    char * p = NULL;
    char fpath[SIZE_RCVBUF] = {0};
    char fname[SIZE_RCVBUF] = {0}; 
    char rcv_buffer[SIZE_RCVBUF] = {0};
    FILE *fp = NULL;
    int file_percentage = 0;

    //length = read(clientfd, fname, SIZE_RCVBUF);
    length = DataLinkRecv(clientfd, fname, SIZE_RCVBUF);

    p = strstr(fname, "#");
    fsize = atoi((char *)(p+1));

    strncpy(rcv_buffer,fname, (int)(p-fname));
    memset(fname, 0, sizeof(fname));
    strcpy(fname, rcv_buffer);
    memset(rcv_buffer, 0, sizeof(rcv_buffer));
    strcat(fpath, "/projects/Network/Proj2/rdt1.0/file/");
    strcat(fpath, fname);
    printf("File name: %s\nFile size: %d\n", fname, fsize);

    if((fp = fopen(fpath, "w")) == NULL){
        printf("File: %s Can Not Open To Write!", fpath); 
        return 0;
    }
    
    // rcv_buffer stores the content of file 
    length = 0;
    progress_t bar;
    progress_init(&bar, "", 50, PROGRESS_BGC_STYLE);

    do{
        //len = recv(clientfd, rcv_buffer, SIZE_RCVBUF, 0);
        DataLinkRecv(clientfd, rcv_buffer, SIZE_RCVBUF);
        length += len;
        fwrite(rcv_buffer, sizeof(char), len, fp);
        file_percentage = length*100/fsize;
        progress_show(&bar, (float)length/(float)fsize);
 
        if(length >= fsize)
            break;

    }while(TRUE);
    progress_destroy(&bar);
    printf("\nFile %s uploading Finished!\n", fname); 
    // close the file pointer  
    fclose(fp);
    return 0;

}

int processDownload(char * buffer, int msg_len){
    char fname[SIZE_RCVBUF] = {0};
    char rsp_buffer[SIZE_RCVBUF] = {0};

    //read(clientfd,fname,SIZE_RCVBUF);
    DataLinkRecv(clientfd,fname,SIZE_RCVBUF);
    transfer(fname);

    return 0;
}



int processFilelist(char * buffer, int msg_len){
    // char rsp_buffer[BUF_SIZE] = {0};
    // strncpy(rsp_buffer, buffer, msg_len);
    // strcat(rsp_buffer, "_ACK");
    // write(clientfd, rsp_buffer,strlen(rsp_buffer));
    getFileList();
    return 0;
}


int processQuit(char * buffer, int msg_len){
    char rsp_buffer[SIZE_SNDBUF] = {0};
    strncpy(rsp_buffer, buffer, msg_len);
    strcat(rsp_buffer, "_ACK");
    //write(clientfd, rsp_buffer,strlen(rsp_buffer));
    DataLinkSend(clientfd, rsp_buffer,strlen(rsp_buffer));
    return 0;
}



int transfer(char * fname){
    FILE * fp;
    int act_read = 0;
    int fsize = 0;
    char buffer[SIZE_SNDBUF] = {0};
    char fpath[SIZE_RCVBUF] = {0};
    strcpy(fpath, "/projects/Network/Proj2/rdt1.0/file/");
    strcat(fpath, fname);
    if((fp = fopen(fpath, "r")) == NULL){
        fclose(fp);
        //write(clientfd, "#FILE_NOT_FOUND",16);
        DataLinkSend(clientfd, "#FILE_NOT_FOUND",16);
        return 0;
    }  
    else{
        //  File existed...
        printf("File %s is downloading...\n", fname);
        fsize = getFileSize(fpath);
        //itoa(fsize, buffer, 10);
        sprintf(buffer, "%d", fsize);
        //write(clientfd, buffer,strlen(buffer));
        DataLinkSend(clientfd, buffer,strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
        sleep(2);

        do{
            usleep(20000);
            act_read = fread(buffer, sizeof(char), SIZE_SNDBUF, fp);
            if(act_read == 0){
                break;
            }
            else if(act_read < SIZE_SNDBUF){
                //write(clientfd, buffer, act_read);
                DataLinkSend(clientfd, buffer, act_read);
                break;
            }
            else{
                //write(clientfd, buffer, act_read);
                DataLinkSend(clientfd, buffer, act_read);
                memset(buffer, 0, SIZE_SNDBUF);
            }
            continue;
        }while(TRUE);
        printf("File transfer ended.\n");
        fclose(fp);
    }


    return 0;
}


// int getFileSizeSystemCall(char * strFileName){  
//     struct stat temp;
//     stat(strFileName, &temp);  
//     return temp.st_size;  
// }


int getFileSize(char * strFileName)   
{  
    FILE * fp = fopen(strFileName, "r");  
    fseek(fp, 0L, SEEK_END);  
    int size = ftell(fp);  
    fclose(fp);  
    return size;  
}

int getFileList(){
    char buffer[SIZE_SNDBUF] = {0};
    DIR *directory_pointer;
    struct dirent *entry;
    struct FileList
    {
        char filename[64];
        struct FileList *next;
    }start,*node;

    if ((directory_pointer=opendir("/projects/Network/Proj2/rdt1.0/file"))==NULL)
        printf("Error opening %d\n",2);
    else
    { 
        start.next=NULL;
        node=&start;
        while ((entry=readdir(directory_pointer))!=NULL)
        {
            if(!strcmp(entry->d_name,"..") || !strcmp(entry->d_name,".")){
                continue;
            }
            node->next=(struct FileList *)malloc(sizeof(struct FileList));
            node=node->next;
            strcpy(node->filename,entry->d_name);
            node->next=NULL;
        }
        closedir(directory_pointer);
        node=start.next;
        while(node)
        {
            strcat(buffer, node->filename);
            strcat(buffer, "\n");
            node=node->next;
        }
        //write(clientfd, buffer, strlen(buffer));
        DataLinkSend(clientfd, buffer, strlen(buffer));
    }

  return 0;
}

void progress_init(progress_t *bar, char *title, int max, int style){
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

void progress_show( progress_t *bar, float bit ){
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
void progress_destroy(progress_t *bar){
    free(bar->pro);
}