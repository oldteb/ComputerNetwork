/************************************************************************
    
    dllgen.h  

    This file contains macros, variables, and 
    definitions of data structure that are used 
    by the system.
    
************************************************************************/

#define         TRUE            1

#define     DLL_BUFSIZE     8388608     // 8M tcp buffer...
#define     WINDOW_SIZE     2097152        // 2M Send window size
#define     TOP             ((dll_scb.base + WINDOW_SIZE)%DLL_BUFSIZE)
#define     MSS             1024        // DLL segment maximum length
#define     BUF_SIZE        2048        //  General Buffer size
#define     DLL_HEADER_LEN  25          //  DLL header length
#define     ACK_LEN         28          //  Length of ACK packet
#define     MSG_SIZE        100         //  Maximum message size
#define     SEQ_LEN         8           //  Length of sequence number domain...
#define     WND_LEN         8           //  Length of window size domian...
#define     LRM_LEN         3           //  Length of last recv msg length domain...
#define     CKS_LEN         8           //  Length of Checksum domain..
#define     INIT_AVAL_SIZE  DLL_BUFSIZE //  

#define     MIN(a,b)        (a>b)?b:a

//#define     DEBUG
//#define     TRACE
 

//  Globel variable...
typedef struct DLL_send{
    char DLL_buffer[DLL_BUFSIZE];       // DLL sender buffer
    int base;               // Send window base pointer
    int nextsend;           // Point to the first byte that have not been sent yet
    int last;               // Point to the last byte that need to be sent out
    int avalsize;         // Peer's window size for flow control 
    int status;           // Status of the DLL send buffer...
}DLL_send;

typedef struct DLL_recv{
    char DLL_buffer[DLL_BUFSIZE];       // DLL receiver buffer
    //int base;               // Send window base pointer
    int nextread;           // Point to the first byte that have not been sent yet
    int last;               // Point to the last byte that need to be sent out
    int avalsize;         // Avaliable space in recv buffer...
    int status;           // Status of the DLL recv buffer...
    int last_recv_len;    // Keep track of length of latest recv message...
}DLL_recv;

typedef struct{
    char chr;       /*tip char*/
    char *title;    /*tip string*/
    int style;      /*progress style*/
    int max;        /*maximum value*/
    float offset;
    char *pro;
}progress_t;

DLL_send dll_scb;       // Data link layer sender control block
DLL_recv dll_rcb;       // Data link layer receiver control block

int clientfd;

time_t t;



