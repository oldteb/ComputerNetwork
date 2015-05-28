//  server.h
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
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include "srvstr.h"

#ifndef _INCLUDE_SRVGEN_H
#define _INCLUDE_SRVGEN_H
    #include "srvgen.h"
#endif


struct msgbuf{
    long mtype;
    char mtext[1];
    int sockfd;
}msgbuf;


int servStart();
int servEnd();
int servBlock();
int servUnblock();
int servThrowout();
int servStats();
int servCommdErr();
//int processConnect(int );
int processChat(UsrInfo_ptr );
int processQuit(UsrInfo_ptr );
int processTransfer();
int processFlag();
int processHelp();
int processDisconnect(UsrInfo_ptr );
int commdInit();
int srvcommandValid(char* nextAct);
int ClientCmdRecognize(char* buffer, int message_length);
int getMassege(UsrInfo_ptr );
void connect_handler(int *);
int message_handler(UsrInfo_ptr , int );
int auto_match();
int RemoveUser(UsrInfo_ptr );
int ipcmsg_handler(struct msgbuf * );
int printStats();
int LogutAll();
int guarantee_write(UsrInfo_ptr , int );



struct msgbuf msgT = {0};
int msgID[1];
int msgLength = sizeof(struct msgbuf)-sizeof(long);


int commdInit(){
	srvcommdFunc[0] = servStart;
	srvcommdFunc[1] = servEnd;
	srvcommdFunc[2] = servBlock;
	srvcommdFunc[3] = servUnblock;
	srvcommdFunc[4] = servThrowout;
	srvcommdFunc[5] = servStats;
	srvcommdFunc[6] = servCommdErr;
	return 0;
}


int servStart(){
	int clientfd;
	int message_length;
    int on;
    int pid;
    int flags;
    //int sin_size = sizeof(struct sockaddr);
	struct sockaddr_in my_addr;
    struct sockaddr_in remote_addr;
    UsrInfo_ptr scnr = NULL;
    UsrInfo_ptr temp = NULL;

    socklen_t sin_size = sizeof(struct sockaddr);

    pthread_t cons_tid;
    int* temp_cltsockfd;

    msgT.mtype = 1;
    msgT.mtext[0]=0;
    msgT.sockfd = 0;

    if(serverstate == SRV_STAT_START){
        printf("server is already running...\n");
        return 0;
    }



	printf("Creating socket ...\n");
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1){
	    perror("socket creat error!\n");
	    exit(0);            
	}

	//keep using socket after closesocket function is invoked.	
	on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

    //* set sockfd to non-blocking...
    flags = fcntl(sockfd,F_GETFL,0);
    fcntl(sockfd,F_SETFL,flags|O_NONBLOCK);


    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(8787);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero),8);

    if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr))==-1)
    {
        perror("bind error!\n");
        exit(1);                                  
    }

    if(listen(sockfd,8)==-1)
    {
        perror("listen error!\n");
        exit(0);                                                         
    }

    msgID[0] = msgget(IPC_PRIVATE,IPC_CREAT|0660);

    printf("Ready to accept...\n");
    pid = fork();
    if(pid == 0){
        //pthread_attr_init(&attr);
        //pthread_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if(pthread_mutex_init(&work_mutex,NULL) != 0){
            printf("Mutex initialize failed...\n");
        }
        while(TRUE){
            msgrcv(msgID[0],(void *)&msgT,msgLength,1,IPC_NOWAIT);

            if(msgT.mtext[0] != 0){
                ipcmsg_handler(&msgT);
            }
            else if(errno == EAGAIN || errno == ENOMSG)
            {   
                if((clientfd = accept(sockfd,(struct sockaddr*)&remote_addr,&sin_size)) > 0){
                    /*    TCP Construct Request received...    */
                    temp_cltsockfd = (int* )malloc(sizeof(int));
                    *temp_cltsockfd = clientfd;
                    if(pthread_create(&cons_tid, NULL, (void *)connect_handler, (void *)temp_cltsockfd)){
                        printf("Error in creating thread...\n");
                    }

                }

                /*    Check the message queue...    */
                for(scnr = uihead; scnr != NULL; ){
                    message_length = getMassege(scnr);
                    if(message_length == 0){
                        //Remote socket closed rudely...   
                        temp = scnr;
                        scnr = scnr->next;
                        RemoveUser(temp);
                    }
                    else if(message_length > 0){
                        //    There is a message needed to be handled...    
                        message_handler(scnr, message_length);
                        memset(chatbuffer,0,BUF_SIZE);
                        scnr = scnr->next;
                    }
                    else{
                        //    No message found current scnr... 
                        scnr = scnr->next;
                    }
                }
            }

            sleep(0.3);
        }
    }
    else{
        //  parent process...
        serverstate = SRV_STAT_START;
        printf("This is parent process...");
        printf("Subprocess pid = %d\n",pid);

    }

    return 0;
}


int servEnd(){
    /*   Inform all the users to log out...    */
    if(serverstate == SRV_STAT_END){
        printf("server is already close...\n");
        return 0;
    }
    msgT.mtext[0] = 'E';    // E stands for 'END'...
    if(msgsnd(msgID[0],&msgT,msgLength,0)!=0)
        printf("Error on servEnd\n"); 

    //while(wait(NULL)!=-1 || errno!=ECHILD);
    printf("Server Terminated.\n");
    close(sockfd);
    serverstate = SRV_STAT_END;
    return 0;
}

int servBlock(){
    /*    Block the giving user...    */
    int src = 0;

    if(serverstate != SRV_STAT_START){
        printf("Error: Server is not started.\n");
        return 0;
    }
    printf("The ID number of the user you want to block is:");
    scanf("%d",&src);
    getchar();

    msgT.mtext[0] = 'B';   // B stands for 'BLOCK'...
    msgT.sockfd = src;
    if(msgsnd(msgID[0],&msgT,msgLength,0)!=0)
        printf("Error on servEnd\n");
    msgT.mtext[0] = 0;
    msgT.sockfd = 0;

    return 0;
}

int servUnblock(){
    /*    Unblock the giving user...    */
    int src = 0;

    if(serverstate != SRV_STAT_START){
        printf("Error: Server is not started.\n");
        return 0;
    }
    printf("The ID number of the user you want to unblock is:");
    scanf("%d",&src);
    getchar();

    msgT.mtext[0] = 'U';   // B stands for 'UNBLOCK'...
    msgT.sockfd = src;
    if(msgsnd(msgID[0],&msgT,msgLength,0)!=0)
        printf("Error on servEnd\n");
    msgT.mtext[0] = 0;
    msgT.sockfd = 0;

    return 0;
}

int servThrowout(){
	/*    Throwout the giving user...    */
    int src = 0;

    if(serverstate != SRV_STAT_START){
        printf("Error: Server is not started.\n");
        return 0;
    }
    printf("The ID number of the user you want to throwout is:");
    scanf("%d",&src);
    getchar();

    msgT.mtext[0] = 'T';   // T stands for 'THROWOUT'...
    msgT.sockfd = src;
    if(msgsnd(msgID[0],&msgT,msgLength,0)!=0)
        printf("Error on servEnd\n");
    msgT.mtext[0] = 0;
    msgT.sockfd = 0;

	return 0;
}

int servStats(){
	/*    Print a list of status of all the users...    */
    if(serverstate != SRV_STAT_START){
        printf("Error: Server is not started.\n");
        return 0;
    }
    msgT.mtext[0] = 'S';   // S stands for 'STATS'...
    if(msgsnd(msgID[0],&msgT,msgLength,0)!=0)
        printf("Error on servEnd\n");
    msgT.mtext[0] = 0;

	return 0;
}

int servCommdErr(){
    printf("error\n");
	return 0;
}


int srvcommandValid(char* nextAct){
	int i = 0;
	for(;i<SRVCOMMDSIZE;i++){
		if(strcmp(nextAct,SrvCommands[i])==0)  break;
	}
	
	return i;
}


int processChat(UsrInfo_ptr src){
    int match_success = 0;

    if(src->stat == CLT_STAT_CONNECT){
        src->stat = CLT_STAT_WAIT;
        match_success = auto_match();
    }
    else{
        /*   CHAT request deny...    */
        write(src->sockfd, "#CHAT_DENY",11);
        return 0;
    }

    if(match_success == 0){
        /*   Wait for chat peer...    */
        write(src->sockfd, "#CHAT_PEER_WAIT",16);
    }
    return 0;
}


int processQuit(UsrInfo_ptr src){
    if(src->stat == CLT_STAT_CHAT || src->stat == CLT_STAT_TRANSFER){    /*    Stop chatting...    */
        PEER(src)->stat = CLT_STAT_CONNECT;
        PEER(src)->channel = NULL;
        write(PEER(src)->sockfd, "#CHAT_CLOSE",12);
        //write(src->sockfd, "#CHAT_CLOSE1",13);
        RmvChatChannel(src->channel);
        src->stat = CLT_STAT_CONNECT;
        src->channel = NULL;
        auto_match();
    }
    else if(src->stat == CLT_STAT_WAIT){    /*    Stop waiting...    */
        src->stat = CLT_STAT_CONNECT;
        write(src->sockfd, "#CHAT_CLOSE",12);
    }
    else{   /*    Invalid command...    */
        write(src->sockfd, "#IVLDCMD",9);
    }

    return 0;
}

int processTransfer(UsrInfo_ptr src){

        if(src->stat == CLT_STAT_CHAT){     /*    Require transfering file....   */
            src->stat = CLT_STAT_TRANSFER;
            PEER(src)->stat = CLT_STAT_TRANSFER;
            write(PEER(src)->sockfd, "#TRANSFER",10);
        }
        else if(src->stat != CLT_STAT_TRANSFER){     /*    Unable to transfer file...   */
            write(src->sockfd, "#TRANSFER_DENY",15);
        }
        else if(strcmp(chatbuffer,"#TRANSFER_END") == 0){  /*   Transfer file complete...    */
            
            printf("File transfer completed.\n");
            write(src->sockfd, "#TRANSFER_COMPLETE1",20);
            write(PEER(src)->sockfd, "#TRANSFER_COMPLETE2",20);
            src->stat = CLT_STAT_CHAT;
            PEER(src)->stat = CLT_STAT_CHAT;
        }

        return 0;

}

int processFlag(UsrInfo_ptr src){
    /*    Send warning to the server...    */
    if(src->stat == CLT_STAT_CHAT){
        if(PEER(src)->warning == WARNED){
            /*    Remind admin user should be blocked...    */
            printf("User %d should be blocked...\n", PEER(src)->sockfd);
        }
        else{
            PEER(src)->warning = WARNED;
            write(PEER(src)->sockfd, "#WARNED",8);
        }
    }
    else{
        write(src->sockfd, "#FLAG_DENY",11);
    }

    return 0;
}

int processHelp(UsrInfo_ptr src){
    /*   Send the invoker the help list...    */
    char str1[BUF_SIZE];
    strcpy(str1, "Avaliable commands are:\n");

    strcat(str1, HelpCommands[src->stat]);
    write(src->sockfd, str1,strlen(str1));

    return 0;
}

int processDisconnect(UsrInfo_ptr src){
    int temp_sockfd = src->sockfd;
    CC_ptr channel = src->channel;

    write(temp_sockfd,"#ACK4",6);
    if(channel != NULL){   /*    close the channle first...    */
        write(PEER(src)->sockfd, "#CHAT_CLOSE",12);
        PEER(src)->stat = CLT_STAT_CONNECT;
        PEER(src)->channel = NULL;
        RmvChatChannel(src->channel);
    }

    RmvUsrInfo(temp_sockfd);
    close(temp_sockfd);
    return 0;
}


int ClientCmdRecognize(char* buffer, int message_length){
    int i;
    char commd[BUF_SIZE] = {0};

    strncpy(commd,buffer,message_length);
    commd[message_length-2] = '\0';
    for(i=0;i<CLTCOMMDSIZE;i++){
        if(strcmp(commd,CltCommands[i])==0)  break;
    }

    return i;
}

int getMassege(UsrInfo_ptr src){
    int message_length = -1;

    if((message_length = read(src->sockfd,chatbuffer,BUF_SIZE)) == -1){
        //    No message found from src...  
    }
    else{
        if(message_length == 0){
            //Reomote socket closed rudely... 
            printf("User %d is off line.\n", src->sockfd);   
            //RemoveUser(src);
        }
        else{
            printf("Get message from %s, ", src->nickname);
            printf("message length: %d\n",message_length);
        }
    }

    return message_length;
}


void connect_handler(int * arg){
    int clientfd = *((int *)arg);
    int message_length = 0;
    char buffer[BUF_SIZE];
    char nickname[SBUF_SIZE] = {0};
    UsrInfo_ptr usr;
    int flags = 0;
    int set = 212992;  

    //pthread_detach(pthread_self());

    //printf("Connection with %d initialized.\n", clientfd);
    memset(buffer,0,sizeof(buffer));
    if((message_length = read(clientfd,buffer,BUF_SIZE))==-1)
    {
        printf("read error:%s\n",strerror(errno));
        close(clientfd);
        return;         
    }
    else{
        buffer[message_length-2] = '\0';
        //printf("Command received from %d: %s\n", clientfd, buffer);
        /*    Check Conection commands...    */
        if(strncmp(buffer,CltCommands[0],sizeof(CltCommands[0])) != 0){
            //  Conection construct failed...  
            printf("Invalid commands received, new conection construction failed...\n");
            write(clientfd,"#ACK0",6);
            close(clientfd);
        }
        else{
            if(GetUsrNum() >= MAX_USR_NUM){
                write(clientfd,"#ACK5",6);
                printf("New user try to log in. Maxmuim user amount reached\n");
                close(clientfd);
                return;
            }

            strcpy(nickname, buffer+8 );
            if(strlen(nickname) > MAX_KICKNM_LEN){
                write(clientfd,"#ACK6",6);
                close(clientfd);
                return;
            }

            //  set sockfd to non-blocking...
            flags = fcntl(clientfd,F_GETFL,0);
            fcntl(clientfd,F_SETFL,flags|O_NONBLOCK);

            //  set the size of the send buffer of sockfd... 
            setsockopt(clientfd, SOL_SOCKET, SO_SNDBUF, &set, sizeof(set) );

            AddUsrInfo(clientfd);
            if((usr = GetUsrInfo(clientfd)) == NULL){
                printf("Error in initializing userinfo %d.\n", clientfd);     
            }

            strcpy(usr->nickname, nickname );
            printf("New user %s login\n", usr->nickname);
            write(clientfd,"#ACK1",6);

        }
    }
    free(arg);

}


int message_handler(UsrInfo_ptr src, int message_length){
    char *pPos = NULL;
    int sendtotal = 0;


    //    Handle the file transfer packet...
    if(strcmp(chatbuffer,"#TRANSFER_END") != 0 && src->stat == CLT_STAT_TRANSFER){
        if((pPos = strstr(chatbuffer,"#TRANSFER_END")) != NULL){
            /*    The last part of the file...   */
            message_length = pPos-chatbuffer;
            chatbuffer[message_length-2] = '\0';
            
            sendtotal = guarantee_write(src, message_length);
            src->channel->dataflow += (long)sendtotal;
            printf("%d\n", sendtotal);

            memset(chatbuffer, 0, BUF_SIZE);
            strcpy(chatbuffer, "#TRANSFER_END");
            //printf("Message type: CMD, content: %s\n", chatbuffer);
            processTransfer(src);

        }  
        else if(strcmp((chatbuffer+(message_length-13)), "#TRANSFER_END") == 0){
            /*    The last part of the file...   */
            message_length = message_length-13;
            chatbuffer[message_length-2] = '\0';
            
            sendtotal = guarantee_write(src, message_length);
            src->channel->dataflow += (long)sendtotal;
            printf("%d\n", sendtotal);

            memset(chatbuffer, 0, BUF_SIZE);
            strcpy(chatbuffer, "#TRANSFER_END");
            //printf("Message type: CMD, content: %s\n", chatbuffer);
            processTransfer(src);

        }
        else{
            sendtotal = guarantee_write(src, message_length);
            src->channel->dataflow += (long)sendtotal;
            printf("%d\n", sendtotal);
        } 

        return 0;
    }

    if(chatbuffer[0] == '#'){
        printf("Message type: CMD, content: %s\n", chatbuffer);
        switch(ClientCmdRecognize(chatbuffer,message_length)){
        case 1:
            //  CHAT...
            processChat(src);
            break;

        case 2:
            //  QUIT...
            processQuit(src);
            break;

        case 3:
            //  TRANSFER...
            processTransfer(src);
            break;

        case 4:
            //  FLAG...
            processFlag(src);
            break;

        case 5:
            //  HELP...
            processHelp(src);
            break;

        case 6:
            //  DISCONNECT...
            processDisconnect(src);
            break;

        case 7:
            //  TRANSFER_END...
            printf("T_E\n");
            sleep(1);
            processTransfer(src);
            break;

        default:
            write(src->sockfd, "#IVLDCMD",9);
            break;        
        }
    }
    else{    /*    Chatting message...    */
        /*    If sender is allowed to chat...    */
        if(src->stat == CLT_STAT_CHAT){
            printf("Message type: MSG, content: %s\n", chatbuffer);
            write(PEER(src)->sockfd, chatbuffer,message_length);
            /*    Calculate the data flow...    */
            src->channel->dataflow += (long)message_length;
        }
        else{   /*    Not in chat status...    */
            write(src->sockfd, "#CHAT_PERMIT_DENY",18);
        }
    }

    return 0;
}

int auto_match(){
    UsrInfo_ptr peerA = NULL;
    UsrInfo_ptr peerB = NULL;
    CC_ptr channel = NULL;
    UsrInfo_ptr scnr = uihead;

    /*    Search possible new channel...    */
    while(scnr != NULL){
        if(scnr->stat == CLT_STAT_WAIT){
            if(peerA == NULL){
                peerA = scnr;
                scnr = scnr->next;
                continue;
            }
            else if(peerB == NULL){
                peerB = scnr;
                channel = AddChatChannel(peerA, peerB);
                peerA->stat = CLT_STAT_CHAT;
                peerA->channel = channel;
                peerB->stat = CLT_STAT_CHAT;
                peerB->channel = channel;
                printf("Channel construct: %d starts chatting with %d.\n", peerA->sockfd, peerB->sockfd);
                write(peerA->sockfd, "#CHAT_START",12);
                write(peerB->sockfd, "#CHAT_START",12);
                return 1;
            }
            else{
                // error...
                break;
            }
        }
        scnr = scnr->next;
    }

    return 0;
}


int RemoveUser(UsrInfo_ptr src){

    if(src->stat == CLT_STAT_CHAT || src->stat == CLT_STAT_TRANSFER){
        /*   Inform peer cut the channel...    */
        write(PEER(src)->sockfd, "#CHAT_CLOSE",12);
        PEER(src)->stat = CLT_STAT_CONNECT;
        PEER(src)->channel = NULL;
        RmvChatChannel(src->channel);
    }
    close(src->sockfd);
    RmvUsrInfo(src->sockfd);

    return 0;
}


int ipcmsg_handler(struct msgbuf * msg){
    /*    Respond to the ipc message accordingly...    */
    char mtext = msg->mtext[0];
    int sockfd = msg->sockfd;
    UsrInfo_ptr src = NULL;

    switch(mtext){
        case 69:    
            /*    Entry for END...    */
            LogutAll();
            DeleteList();
            //close(sockfd);
            printf("Subprocess closed...\n");
            exit(0);
            break;

        case 66:
            /*    Entry for BLOCK...    */
            if(GetUsrInfo(sockfd)->stat != CLT_STAT_CONNECT){
                printf("Unable to block user %d\n", sockfd);
            }
            else{
                /*    Block the user...    */
                GetUsrInfo(sockfd)->stat = CLT_STAT_BLOCK;
                printf("User %d has been blocked.\n",sockfd);
            }
            break;

        case 85:
            /*    Entry for UNBLOCK...    */
            if(GetUsrInfo(sockfd)->stat != CLT_STAT_BLOCK){
                printf("Unable to unblock user %d\n", sockfd);
            }
            else{
                /*    Unblock the user...    */
                GetUsrInfo(sockfd)->stat = CLT_STAT_CONNECT;
                printf("User %d has been unblocked.\n",sockfd);
            }
            break;

        case 84:
            /*    Entry for THROWOUT...    */
            src = GetUsrInfo(sockfd);
            if(src->stat == CLT_STAT_BLOCK
                    || src->stat == CLT_STAT_CONNECT){
                printf("Unable to throwout user %d\n", sockfd);
            }
            else if(src->stat == CLT_STAT_WAIT){
                src->stat = CLT_STAT_CONNECT;
                write(src->sockfd, "#CHAT_CLOSE",12);
                printf("User %d has been throwed out.\n",sockfd);
            }
            else{
                /*   Destroying channel...    */
                PEER(src)->stat = CLT_STAT_CONNECT;
                PEER(src)->channel = NULL;
                write(PEER(src)->sockfd, "#CHAT_CLOSE",12);
                write(src->sockfd, "#CHAT_CLOSE",12);
                RmvChatChannel(src->channel);
                src->stat = CLT_STAT_CONNECT;
                src->channel = NULL;
                printf("User %d has been throwed out.\n",sockfd);
            }
            break;
                
        case 83:
            /*    Entry for STATS...    */
            printStats();
            break;

        default:
            printf("Unknown ipc message...\n");
    }

    msg->mtext[0] = 0;
    msg->sockfd = 0;

    return 0;
}


int printStats(){
    FILE* fp;
    int usernumber = 0;
    int chatnumber = 0;
    int warnednumber = 0;

    /*    Print out the STATS...    */
    UsrInfo_ptr scnr = uihead;

    printf("\nList of status of users in system.\n");
    printf("------------------------------------------------------------------------------------------\n");
    printf("User ID\t\tName        \tState   \tWarning\t\tPeer ID\t\tData Flow\n");
    printf("------------------------------------------------------------------------------------------\n");
    while(scnr != NULL){
        if(scnr->channel == NULL){
            printf("%d\t\t%-12s\t%-8s\t%s\t\tN/A\t\tN/A\n", scnr->sockfd, scnr->nickname, CltStatTable[scnr->stat], CltWrnTable[scnr->warning]);
        }   
        else{
            printf("%d\t\t%-12s\t%-8s\t%s\t\t%d\t\t%d B\n", scnr->sockfd, scnr->nickname, CltStatTable[scnr->stat], CltWrnTable[scnr->warning], PEER(scnr)->sockfd, (int)(scnr->channel->dataflow));
            chatnumber++;
        }
        usernumber++;
        if(scnr->warning == WARNED){
            warnednumber++;
        }
        scnr = scnr->next;
    }
    printf("------------------------------------------------------------------------------------------\n");
    printf("Total user number: %d\nTotal chatting clients number: %d\nTotal flaged number: %d\n", usernumber, chatnumber, warnednumber);
    printf("------------------------------------------------------------------------------------------\n");

    /*    Save to the file...    */
    if((fp = fopen("stats","w")) == NULL){
        printf("Unable to open the file\n");
        return 0;
    }
    scnr = uihead;

    fprintf(fp, "\nList of status of users in system.\n");
    fprintf(fp, "-------------------------------------------------------------------------------------------\n");
    fprintf(fp, "User ID\t\tName        \tState   \tWarning\t\tPeer ID\t\tData Flow\n");
    fprintf(fp, "-------------------------------------------------------------------------------------------\n");
    while(scnr != NULL){
        if(scnr->channel == NULL){
            fprintf(fp, "%d\t\t%-12s\t%-8s\t%s\t\tN/A\t\tN/A\n", scnr->sockfd, scnr->nickname, CltStatTable[scnr->stat], CltWrnTable[scnr->warning]);
        }
        else{
            fprintf(fp, "%d\t\t%-12s\t%-8s\t%s\t\t%d\t\t%d B\n", scnr->sockfd, scnr->nickname, CltStatTable[scnr->stat], CltWrnTable[scnr->warning], PEER(scnr)->sockfd, (int)(scnr->channel->dataflow));
        }
        scnr = scnr->next;
    }
    fprintf(fp, "------------------------------------------------------------------------------------------\n");
    fprintf(fp, "Total user number: %d\nTotal chatting clients number: %d\nTotal flaged number: %d\n", usernumber, chatnumber, warnednumber);
    fprintf(fp, "------------------------------------------------------------------------------------------\n");



    fclose(fp);

    return 0;
}


int LogutAll(){
    UsrInfo_ptr scnr = uihead;

    while(scnr != NULL){
        write(scnr->sockfd, "#SRV_CLOSE", 11);
        scnr = scnr->next;
    }

    return 0;
}



int guarantee_write(UsrInfo_ptr src, int message_length){
    int sendlength = 0;
    int sendtotal = 0;
    int sentsize = 0;
    char temp_buf[BUF_SIZE] = {0};

    sentsize = message_length;
    while(TRUE){
        sendlength = write(PEER(src)->sockfd, chatbuffer, sentsize);
        if(sendlength == -1){
            sleep(1);
        }
        else if(sendlength + sendtotal < message_length){
            printf("%d--",sendlength);
            sendtotal += sendlength;
            sentsize = message_length - sendtotal;
            strncpy(temp_buf, chatbuffer+sendtotal, sentsize);
            memset(chatbuffer, 0, BUF_SIZE);
            strncpy(chatbuffer, temp_buf, sentsize);
            memset(temp_buf, 0, BUF_SIZE);
                
            sleep(0.3);
        }
        else{
            break;
        }

    }
    sendtotal += sendlength;

    return sendtotal;
}