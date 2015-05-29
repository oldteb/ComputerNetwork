/************************************************************************
    
    client.c

    Author:                 Yumou Wang

    Complete Time:          3/10/2014 

    This code constructs the client side of a simple FTP
    application. It allows user to download, upload, and
    lookup files on FTP server. This file contains basic
    structure of client end program.

************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include "client.h"


int main(int argc, char **argv)
{
    struct sockaddr_in dest;
    char buffer[MAXBUF + 1];
    char sendtext[MAXBUF + 1];
    int childPid;

    if (argc != 3) {
        printf("Invalid arguments！The correct format is：\n\t\t%s IP address Port\n\tSuch as:\t%s 127.0.0.1 8888\n",argv[0], argv[0]);
        exit(0);
    }


    DLL_Init();


    /* create a socket for TCP connection */
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket");
        exit(errno);
    }
    printf("socket created\n");

    /* Initialize the server's IP address and port number */
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], (struct in_addr *) &dest.sin_addr.s_addr) == 0) {
        perror(argv[1]);
        exit(errno);
    }
    printf("address created\n");

    /* Connect the server */
    if (connect(clientfd, (struct sockaddr *) &dest, sizeof(dest)) != 0) {
        perror("Connect ");
        exit(errno);
    }
    printf("server connected\n");
    printf("");
    
    while(TRUE)
    {
        /*send massage to the server*/
        printf("Please input command:\n");
        memset(buffer,0,sizeof(buffer));
        gets(buffer);
        switch(ClientCmdRecognize(buffer,strlen(buffer))){
            case 0: //  HELP;
                processHelp();
                break;
            case 1: //  UPLOAD;
                processUpload();
                break;
            case 2: //  DOWNLOAD;
                processDownload();
                break;
            case 3: //  FILELIST;
                processFilelist();
                break;
            case 4: //  QUIT;
                processQuit();
                break;  
            default:
                printf("Invalid command!\n");
        }
    }

    /* close connection */
    close(clientfd);
    return 0;
    
}

