/************************************************************************
    
    DLL.c  

    Author:                 Yunhe Tang
                            Yumou Wang

    Complete Time:          4/15/2014 

    This code forms the base of the TCP layer and Physical layer
    that we build. It provides interfaces for upper layer to send
    and receive data to guarantee reliable data transmission. The
    physical layer will imitate the actual network condition by
    discarding packages in specific possibilities.

    Contribution:
    TCP layer               Yunhe Tang
    Physical layer          Yumou Wang

************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "dllgen.h"


//#define     DEBUG
//#define     TRACE

int Packet_Loss_Rate = 20;
int Packet_Corruption_Rate = 0;
float timer_interval = 1;

struct itimerval it_val;

void DLL_Init();
int DataLinkSend(int sockfd, char * buf, int len);
int DataLinkRecv(int sockfd, char * buf, int len);
void DLL_send_thread(void);
void DLL_recv_thread(void);
int DLL_StartSend();
int AddDLLHeader(char * , int , int );
int genChecksum(char * , int );
int CalSendAvalSpace();
int CalRecvAvalSpace();
int StringCopyBuffer(char* , int );
int StringCopyRecvBuffer(char* buf, int );
int StringManipulation(char * , int , int );
int PacketModification(char * , int ,char *);
void printDLL_scb(char *);
void printDLL_rcb(char *);
int printbuf(char * buf, int);
int DataLinkSendACK(int );
int Checksumcheck(char * , int );
int DataDeliver(char * buf, int pkg_len);
int ACK_handler(char * buf);
int Data_handler(char * buf, int pkg_len);
int PhysicalLayerSend(int sockfd, char *buf, int buf_length);
int PhysicalLayerRecv(int sockfd, char *buf, int buf_length);
int If_Packet_Loss(int rate);
void init_random();
int RandomGen();
int IF_Packet_Corruption(int rate);
int Packet_Corrupt(char * buf);
int RetransmitPkt();
int MaxSendLen(int data_unsend);
void TimerStart(int interval, int );
void Timer_handler(int signo);
void TimerStop(int );
void DLL_SetStr();


FILE* fp;
FILE* fp2;


/*********************************************************************
    DLL_Init
        Initialize the data link layer. Set up signal and random seed.
        Create send and recv thread.

**********************************************************************/

void DLL_Init(){
    pthread_t cons_tid1;
    pthread_t cons_tid2;

    // Initialize data link layer controll block
    DLL_SetStr();

    // Initialize data link layer structure
    memset(&it_val, 0, sizeof(it_val));

    // ...
    signal(SIGALRM, Timer_handler);

    // Initialize time seed for randgen
    init_random();

    // Initialize snd/rcv process
    if(pthread_create(&cons_tid1, NULL, (void *)DLL_send_thread, NULL)){
        printf("Error in creating thread...\n");
    }
    if(pthread_create(&cons_tid2, NULL, (void *)DLL_recv_thread, NULL)){
        printf("Error in creating thread...\n");
    }

}


/*********************************************************************
    DataLinkSend
        Interface provided to application layer to send data.

**********************************************************************/

int DataLinkSend(int sockfd, char * buf, int len){
    int avalspace = 0;

    #ifdef DEBUG
    #endif
    if(len <= MSG_SIZE)  
    {
        while(1)
        {
            avalspace = CalSendAvalSpace();
            if(avalspace < len){
                sleep(1);
                continue;
            }
            else{
                if((DLL_BUFSIZE - dll_scb.last - 1) > len){
                    if(dll_scb.status == 0){
                        memcpy(dll_scb.DLL_buffer+dll_scb.last, buf, len);
                        dll_scb.status = 1;
                        dll_scb.last += (len - 1);
                    }
                    else{
                        memcpy(dll_scb.DLL_buffer+dll_scb.last+1, buf, len);
                        dll_scb.last += len;
                    }
                    
                    #ifdef DEBUG
                    #endif
                }
                else{
                    dll_scb.last = StringCopyBuffer(buf, len);
                }

                #ifdef  TRACE
                printDLL_scb("DataLinkSend");
                #endif

                return len;
            }
        }
    }
    else
    {
        printf("ERROR:data to send larger than 100 Bytes!\n");
        return -1;
    }
    return -1;
}


int DataLinkRecv(int sockfd, char * buf, int len){
    #ifdef DEBUG
    #endif


    while(TRUE){
        if(dll_rcb.status == 0){
            sleep(1);
            continue;
        }

        if(dll_rcb.last >= dll_rcb.nextread){
            int Recv_Unread = dll_rcb.last - dll_rcb.nextread + 1;
            if(len >= (Recv_Unread)){
                memcpy(buf, dll_rcb.DLL_buffer + dll_rcb.nextread, Recv_Unread);
                dll_rcb.nextread = (dll_rcb.nextread + Recv_Unread)%DLL_BUFSIZE;
                dll_rcb.avalsize += Recv_Unread;

                #ifdef  TRACE
                printDLL_rcb("DataLinkRecv");
                #endif

                return Recv_Unread;
            }
            else{
                memcpy(buf, dll_rcb.DLL_buffer + dll_rcb.nextread, len);
                dll_rcb.nextread = (dll_rcb.nextread + len)%DLL_BUFSIZE;
                dll_rcb.avalsize += Recv_Unread;

                #ifdef  TRACE
                printDLL_rcb("DataLinkRecv");
                #endif

                return len;
            }
        }
        else if(dll_rcb.nextread == dll_rcb.last + 1)
        { 
            sleep(1);
            continue;
        }
        else{
            int Recv_Unread = DLL_BUFSIZE - dll_rcb.nextread + dll_rcb.last + 1;
            if(len >= (Recv_Unread)){
                StringCopyRecvBuffer(buf, Recv_Unread);
                dll_rcb.nextread = (dll_rcb.nextread + Recv_Unread + 1)%DLL_BUFSIZE;
                dll_rcb.avalsize += Recv_Unread;

                #ifdef  TRACE
                //printDLL_rcb("DataLinkRecv");
                #endif

                return Recv_Unread;
            }
            else{
                memcpy(buf, dll_rcb.DLL_buffer + dll_rcb.nextread, len);
                dll_rcb.nextread = StringCopyRecvBuffer(buf, Recv_Unread) + 1;
                dll_rcb.avalsize += Recv_Unread;

                #ifdef  TRACE
                //printDLL_rcb("DataLinkRecv");
                #endif

                return len;
            }
        }
    }
    return -1;
}


void DLL_send_thread(void){

    while(TRUE){
        if(dll_scb.status == 0){
            continue;
        }
        else if(dll_scb.nextsend == dll_scb.last+1){
            //  there is no data pending to be send out...
            continue;
        }
        else{
            // there is some data pending to be send out...
            DLL_StartSend();
        }
    }
}

//this is a receive thread. Receiving "ACK" or content
void DLL_recv_thread(void){
    int msg_len = 0;
    int pkg_len = 0;
    int checksum = 0;
    int acknum = 0;
    char buf[BUF_SIZE] = {0};
    char realPacket[BUF_SIZE] = {0};  //real packet after truncating unecessary headers
    char tb[BUF_SIZE] = {0};
    while(TRUE)
    {
        pkg_len = msg_len = 0;
        checksum = 0;
        acknum = 0;
        memset(buf, 0, sizeof(buf));
        memset(realPacket, 0, sizeof(realPacket));

        pkg_len = PhysicalLayerRecv(clientfd,buf,sizeof(buf));
        if(pkg_len <= 0){
            sleep(1);
            continue;
        }

        #ifdef TRACE
        #endif
        
        //  Check checksum
        if(Checksumcheck(buf, pkg_len) != 1){
            //  Data corrupted,discrad packet...
            continue;
        }

        if(buf[0] == '1'){
            // Received ACK packet...
            if(pkg_len != ACK_LEN){
                printf("Error in DLL_recv_thread.\n");
                continue;
            }
            ACK_handler(buf);
        }
        else{
            //  Received data packet...
            Data_handler(buf, pkg_len);

        }
    }

}


int DLL_StartSend(){
    char segbuffer[BUF_SIZE] = {0};
    int maxsendsize = 0;
    int a = 0;
    int data_unsend = 0;
    int i = 0;
    int ascii = 0;

    if(dll_scb.last >= dll_scb.nextsend){
        data_unsend = dll_scb.last - dll_scb.nextsend + 1;
    }
    else{
        data_unsend = DLL_BUFSIZE - dll_scb.nextsend + dll_scb.last + 1;
    }

    maxsendsize = MaxSendLen(data_unsend);

    #ifdef DEBUG
    #endif

    if(maxsendsize == 0){
        // can not send out data...
        return 0;
    }

    // Add DLL header...
    memcpy(segbuffer, dll_scb.DLL_buffer+dll_scb.nextsend, maxsendsize);
    // Start timer...
    if(dll_scb.nextsend == dll_scb.base){
        //  Start timer...
        TimerStart(timer_interval, 1);
    }

    AddDLLHeader(segbuffer, dll_scb.nextsend, maxsendsize);
    fp = fopen("stats","a");
    fprintf(fp , "%d , %d ,%c\n",dll_scb.nextsend, maxsendsize, segbuffer[maxsendsize+14]);
    fclose(fp);


    // Deliver sending data to lower layer...
    a = PhysicalLayerSend(clientfd, segbuffer, maxsendsize + 25);

    // Modify DLL controll block...
    dll_scb.nextsend = (dll_scb.nextsend + maxsendsize) % DLL_BUFSIZE;

    #ifdef  TRACE
    printDLL_scb("DLL_StartSend");
    #endif

    //  Return the actual len of 
    return maxsendsize;
}


int AddDLLHeader(char * sendbuffer, int seq, int sendsize){
    char segbuffer[BUF_SIZE] = {0};
    char temp_buf1[BUF_SIZE] = {0};
    char temp_buf2[BUF_SIZE] = {0};
    int len = 0;
    int checksum = 0;

    // Build the header...
    segbuffer[0] = '0';                 //  It is not an ACK...

    //  Add sequence number...
    sprintf(temp_buf1, "%d", seq);
    if((len=strlen(temp_buf1)) != SEQ_LEN){
        while(SEQ_LEN - len != 0){
            strcat(temp_buf2, "0");
            len++;
        }
    }    
    strcat(temp_buf2, temp_buf1);
    strcat(segbuffer, temp_buf2);

    //  Add window size...
    memset(temp_buf1, 0, sizeof(temp_buf1));
    memset(temp_buf2, 0, sizeof(temp_buf2));
    len = 0;
    sprintf(temp_buf1, "%d", dll_rcb.avalsize);
    if((len=strlen(temp_buf1)) != WND_LEN){
        while(WND_LEN - len != 0){
            strcat(temp_buf2, "0");
            len++;
        }
    }    
    strcat(temp_buf2, temp_buf1);
    strcat(segbuffer, temp_buf2);

    //  Add message...
    memcpy(segbuffer + 17, sendbuffer, sendsize);

    //  Calculate checksum...
    memset(temp_buf1, 0, sizeof(temp_buf1));
    memset(temp_buf2, 0, sizeof(temp_buf2));
    len = 0;
    checksum = genChecksum(segbuffer, sendsize+25);

    //  Add checksum...
    sprintf(temp_buf1, "%d", checksum);
    if((len=strlen(temp_buf1)) != CKS_LEN){
        while(CKS_LEN - len != 0){
            strcat(temp_buf2, "0");
            len++;
        }
    }
    strcat(temp_buf2, temp_buf1);
    memcpy(segbuffer + 17 + sendsize, temp_buf2, 8);

    memset(sendbuffer, 0, sizeof(sendbuffer));
    memcpy(sendbuffer, segbuffer, sendsize + 25);

    return 0;
}


int genChecksum(char * buffer, int len){
    int i;
    int checksum = 0;
    for(i = 0; i < len; i++){
        checksum += buffer[i];
    }
    checksum = abs(checksum);

    return checksum;
}

int CalSendAvalSpace(){

    if((dll_scb.last + 1)%DLL_BUFSIZE == dll_scb.base){
        return DLL_BUFSIZE;
    }
    else if((dll_scb.last + 1)%DLL_BUFSIZE < dll_scb.base){
        return (dll_scb.base - dll_scb.last -1);
    }
    else{
        return (DLL_BUFSIZE - dll_scb.last - 1 + dll_scb.base);
    }
}

int CalRecvAvalSpace(){
    if((dll_rcb.last + 1)%DLL_BUFSIZE == dll_rcb.nextread){
        return DLL_BUFSIZE;
    }
    else if((dll_rcb.last + 1)%DLL_BUFSIZE < dll_rcb.nextread){
        return (dll_rcb.nextread - dll_rcb.last -1);
    }
    else{
        return (DLL_BUFSIZE - dll_rcb.last - 1 + dll_rcb.nextread);
    }
}

int StringCopyBuffer(char* buf, int msg_len)
{
    int len = msg_len;
    int i,j;
    int flag = 0;
    for(i = j = dll_scb.last; i<DLL_BUFSIZE; i++)
    {
        dll_scb.DLL_buffer[i+1] = buf[i-j];
        flag++;
    }
    for(i=0;i<(len-flag);i++)
    {
        dll_scb.DLL_buffer[i] = buf[i+flag];
    }
    return (len-flag);
}

int StringCopyRecvBuffer(char* buf, int Recv_Unread)
{
    int i,j;
    int flag = 0;
    for(i = j = dll_rcb.nextread; i<DLL_BUFSIZE; i++)
    {
        buf[i-j] = dll_rcb.DLL_buffer[i];
        flag++;
    }
    for(i=0;i<(Recv_Unread-flag);i++)
    {
        buf[i+flag] = dll_rcb.DLL_buffer[i];
    }
    return (Recv_Unread-flag);
}

//this function extracts a substring from beginning place to final place.
int StringManipulation(char* buf,int first,int last)
{
    char BufTemp[BUF_SIZE] = {0};
    int i;
    for (i=first;i<last+1;i++)
    {
        BufTemp[i-first] = buf[i];
    }

    i= atoi(BufTemp);
    return i;
}

//this functions helps to extract the content/payload of that function.
int PacketModification(char* buf, int len, char * buffer)
{
    int i = 17;
    for (i=17;i<len-8;i++)
    {
        buffer[i]=buf[i];
    }
    return 0;
}



void printDLL_scb(char * buf){
    printf("\nPrint SEND in %s start:\n",buf);
    printf("base: %d\nnextsend: %d\nlast: %d\navalsize: %d\n", dll_scb.base, dll_scb.nextsend, dll_scb.last, dll_scb.avalsize);
    printf("Print SEND end.\n\n");
}


void printDLL_rcb(char * buf){
    printf("\nPrint RECV in %s start:\n", buf);
    printf("nextread: %d\nlast: %d\navalsize: %d\n", dll_rcb.nextread, dll_rcb.last, dll_rcb.avalsize);
    printf("Print RECV end.\n\n");
}


int printbuf(char * buf, int len){
    int i = 0;
    for(; i<len;i++){
        printf("%c", buf[i]);
    }
    printf("\n");

}


int DataLinkSendACK(int acknum){
    char buffer[BUF_SIZE] = {0};
    char temp_buf1[BUF_SIZE] = {0};
    char temp_buf2[BUF_SIZE] = {0};
    int len = 0;
    int checksum = 0;
    int a = 0;

    //  This is a ACK packet...
    buffer[0] = '1';

    //  Add ACK number...
    sprintf(temp_buf1, "%d", acknum);
    if((len=strlen(temp_buf1)) != SEQ_LEN){
        while(SEQ_LEN - len != 0){
            strcat(temp_buf2, "0");
            len++;
        }
    }    
    strcat(temp_buf2, temp_buf1);
    strcat(buffer, temp_buf2);

    //  Add window size...
    memset(temp_buf1, 0, sizeof(temp_buf1));
    memset(temp_buf2, 0, sizeof(temp_buf2));
    len = 0;
    sprintf(temp_buf1, "%d", dll_rcb.avalsize);
    if((len=strlen(temp_buf1)) != WND_LEN){
        while(WND_LEN - len != 0){
            strcat(temp_buf2, "0");
            len++;
        }
    }    
    strcat(temp_buf2, temp_buf1);
    strcat(buffer, temp_buf2);

    //  Add last recv massage len...
    memset(temp_buf1, 0, sizeof(temp_buf1));
    memset(temp_buf2, 0, sizeof(temp_buf2));
    len = 0;
    sprintf(temp_buf1, "%d", dll_rcb.last_recv_len);
    if((len=strlen(temp_buf1)) != LRM_LEN){
        while(LRM_LEN - len != 0){
            strcat(temp_buf2, "0");
            len++;
        }
    }    
    strcat(temp_buf2, temp_buf1);
    strcat(buffer, temp_buf2);

    //  Calculate checksum...
    memset(temp_buf1, 0, sizeof(temp_buf1));
    memset(temp_buf2, 0, sizeof(temp_buf2));
    len = 0;
    checksum = genChecksum(buffer, 20);

    //  Add checksum...
    sprintf(temp_buf1, "%d", checksum);
    if((len=strlen(temp_buf1)) != CKS_LEN){
        while(CKS_LEN - len != 0){
            strcat(temp_buf2, "0");
            len++;
        }
    }
    strcat(temp_buf2, temp_buf1);
    strcat(buffer, temp_buf2);

    //
    a = write(clientfd, buffer, ACK_LEN);
    if(a != ACK_LEN){
        printf("Error in DataLinkSendACK...\n");
    }
    usleep(20000);

    return 0;
}



int Checksumcheck(char * buf, int len){
    int gencs = genChecksum(buf,len-8);
    int getcs = StringManipulation(buf,len-8,len-1);

    if(gencs == getcs){
        return 1;
    }
    else{
        return 0;
    }
}

int DataDeliver(char * buf, int pkg_len){
    char realPacket[BUF_SIZE] = {0};
    int msg_len = 0;

    // Store message data...
    msg_len = pkg_len - DLL_HEADER_LEN;
    memcpy(realPacket,buf+17,msg_len);

    //  Check if have enough space to store data...
    if (msg_len > CalRecvAvalSpace())
    {
        // Discard packet...
        printf("Not enough space.\n");
        memset(buf, 0, sizeof(buf));
        //pthread_mutex_unlock(&lock);
    }
    else{
        //  Enough space to store data...
        //  Refresh sender avalspace...
        dll_scb.avalsize = StringManipulation(buf,9,16);

        if ((dll_rcb.last + msg_len) > DLL_BUFSIZE)
        {
            dll_rcb.last = StringCopyRecvBuffer(realPacket, msg_len) - 1;            
        }
        else
        {
            if(dll_rcb.status == 0){
                memcpy(dll_rcb.DLL_buffer+dll_rcb.last,realPacket , msg_len);
                dll_rcb.last +=  (msg_len -1);
                dll_rcb.status = 1;
            }
            else{
                memcpy(dll_rcb.DLL_buffer+dll_rcb.last+1,realPacket , msg_len);
                dll_rcb.last +=  msg_len;
                printf("last:%d\n", dll_rcb.last);
            }
    
            dll_rcb.avalsize = dll_rcb.avalsize - msg_len;
            
            #ifdef  TRACE
            //printDLL_rcb("DLL_recv_thread");
            #endif
        }    
    }

    // Refresh last recv len...
    dll_rcb.last_recv_len = msg_len;

    //  Send out ACK...
    DataLinkSendACK((dll_rcb.last + 1)%DLL_BUFSIZE);

    return 0;
}




int ACK_handler(char * buf){
    int acknum = 0;
    int lrl = 0;
    //  Refresh window size...
    dll_scb.avalsize = StringManipulation(buf,9,16);

    //  Get ACK number...
    acknum = StringManipulation(buf,1,8);
    if(acknum == dll_scb.base){
        //  Retransmission data from base to nextsend...
        TimerStop(0);
        TimerStart(timer_interval,2);
        printf("fully retrans\n");
        RetransmitPkt();
    }
    else if(acknum == dll_scb.nextsend){
        //  Success...
        dll_scb.base = acknum;
        TimerStop(1);
    }
    else if(acknum > dll_scb.base && acknum < dll_scb.nextsend){
        // Get last recv len...
        lrl = StringManipulation(buf,17,19);
        if(acknum - lrl == dll_scb.base){
            //  Success...
            dll_scb.base = acknum;
            //TimerStop(1);
        }
        else{
            //  Retransmission data from base to nextsend...
            TimerStop(0);
            TimerStart(timer_interval,2);
            printf("Partially retrans\n");
            RetransmitPkt();
        }
    }
    else{
        //  Error...
        printf("Error in DLL_recv_thread\n");
    }

    return 0;
}


int Data_handler(char * buf, int pkg_len){
    int acknum = 0;
    //  Get sequence number...
    acknum = StringManipulation(buf,1,8);


    //Check if this data packet should be store...
    if(dll_rcb.status == 0){
        //  seq = 0 expected...
        if(acknum == 0){
            // First packet received successfully...
            DataDeliver(buf, pkg_len);
        }
        else{
            //  Discard packet...
            DataLinkSendACK(0);
            acknum = 0;
            memset(buf, 0, sizeof(buf));
        }
    }
    else{
        //  seq = last +1 expected...
        if(acknum == (dll_rcb.last + 1)%DLL_BUFSIZE){
            fp2 = fopen("rcvstats","a");
            fprintf(fp2 , "%d , %d, %c\n",acknum, pkg_len, buf[pkg_len-11]);
            fclose(fp2);
            //printbuf(buf, pkg_len);
            DataDeliver(buf, pkg_len);
        }
        if(acknum > (dll_rcb.last + 1)%DLL_BUFSIZE){
            //  Data not in order, Discard packet, Send ACK...
            DataLinkSendACK((dll_rcb.last + 1)%DLL_BUFSIZE);
        }
        else{
            //  Duplicate packet received...
            //  Discard packet...
        }
    }
    return 0;
}


int PhysicalLayerSend(int sockfd, char *buf, int buf_length){
    int len = 0;

    if(If_Packet_Loss(Packet_Loss_Rate) == 0){
        //  Packet send...
        if(IF_Packet_Corruption(Packet_Corruption_Rate) == 1)
        {  
            //  Packet corrupt... 
            printf("packet corrupt.\n");
            Packet_Corrupt(buf);
            len = send(clientfd, buf, buf_length, 0);
        }
        else
        {
            len = send(clientfd, buf, buf_length, 0);
        }
    }
    else{
        //  Packet loss...
        printf("packet loss.\n");
        len = 0;
    }
    return len;
}
 
int PhysicalLayerRecv(int sockfd, char *buf, int buf_length){
    char buffer[BUF_SIZE] = {0};
    memset(buffer,0,sizeof(buffer));
    int len = recv(clientfd, buffer, sizeof(buffer),0);
    if(len == -1){
        return -1;
    }
    memcpy(buf, buffer, len);
    return len;
}

int If_Packet_Loss(int rate){
    int RandomNum = RandomGen();
    if(RandomNum >= rate){
        return 0;
    }
    else if(RandomNum < rate){
        return 1;
    }
    return 1;
}

void init_random()
{
    srand((unsigned) time(&t));
}
 
int RandomGen()
{
    return random()%100;
}
 
int IF_Packet_Corruption(int rate)
{  
    int RandomNum = RandomGen();
    if(RandomNum >= rate){
        return 0;
    }
    else if(RandomNum < rate){
        return 1;
    }
    return 0;
}
 
int Packet_Corrupt(char * buf)
{
    char buffer[10] = {"11111"};
    memcpy(buf+10, buffer, 5);
    return  0;
}


int RetransmitPkt(){
    int sendlen = 0;
    int i = 0;
    char buffer[BUF_SIZE] = {0};
    int maxsend = 0;
    int temp = 0;
    int seq = 0;

    printf("Retrans start...seq = %d\n",dll_scb.base);
    if(dll_scb.nextsend < dll_scb.base){
        //  recycle...
        printf("Oops.\n");
    }
    else if(dll_scb.nextsend > dll_scb.base){
        sendlen = dll_scb.nextsend - dll_scb.base;
        while(sendlen > 0)
        {
            maxsend = MaxSendLen(sendlen);
            if(maxsend > 0){
                seq = (dll_scb.base + temp)%DLL_BUFSIZE;
                memcpy(buffer, dll_scb.DLL_buffer + seq, maxsend);
                AddDLLHeader(buffer, seq, maxsend);
                i = write(clientfd, buffer, maxsend + 25);
                if(i != maxsend + 25){
                    printf("~~~Retransmit ERROR.\n");
                    getchar();
                }
                sendlen = sendlen - i;
                temp = temp + i;
            }
            usleep(20000);
        }
        //printf("Resend completed...\n");
    }
    else{
        printf("Retransmit ERROR.\n");
    }

}



int MaxSendLen(int data_unsend){
    int maxsendsize = 0;

    if(TOP >= dll_scb.nextsend){
        maxsendsize = TOP - dll_scb.nextsend;
    }
    else{
        maxsendsize = DLL_BUFSIZE - dll_scb.nextsend + TOP;
    }
    // Determine the minimum size of message...
    maxsendsize = MIN(dll_scb.avalsize, maxsendsize);
    maxsendsize = MIN(maxsendsize, MSS-DLL_HEADER_LEN);
    maxsendsize = MIN(maxsendsize, data_unsend);

    return maxsendsize;
}


void TimerStop(int i) {
    it_val.it_value.tv_sec = 0;
    it_val.it_value.tv_usec = 0;
    it_val.it_interval.tv_sec = 0;
    it_val.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &it_val, NULL);  
}
    
void Timer_handler(int signo)
{
    //Retransmit data...
    TimerStop(2);
    TimerStart(timer_interval, 3);
    printf("timer retrans\n");
    RetransmitPkt();
    
}


void TimerStart(int interval, int i) {

    it_val.it_value.tv_sec = interval;
    it_val.it_value.tv_usec = 500000;
    it_val.it_interval.tv_sec = interval;
    it_val.it_interval.tv_usec = 500000;
    setitimer(ITIMER_REAL, &it_val, NULL); 
}

void DLL_SetStr(){
    float PLR = 0;
    float PCR = 0;
    memset(&dll_scb, 0, sizeof(dll_scb));
    memset(&dll_rcb, 0, sizeof(dll_rcb));

    dll_scb.avalsize = INIT_AVAL_SIZE;
    dll_rcb.avalsize = INIT_AVAL_SIZE;

    printf("Please input your Packet loss rate:");
    scanf("%f",&PLR);
    printf("Please input your Packet corrupt rate:");
    scanf("%f",&PCR);
    getchar(); 

    //  if valid...
    if(PLR < 0 || PLR >= 1 || PCR < 0 || PCR >= 1){
        printf("Invalid rate!\n");
        printf("Using default rate:\n");
    }
    else{
        Packet_Loss_Rate = (int)(PLR*100);
        Packet_Corruption_Rate = (int)(PCR*100);
    }


    printf("Packet loss rate is %d%%\nPacket corrupt rate is %d%%\nRetransmission waiting interval is %f sec.\n"
        ,Packet_Loss_Rate, Packet_Corruption_Rate, timer_interval);
    printf("Data Link Layer initialized...\n");
}
