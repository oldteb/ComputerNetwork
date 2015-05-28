//	srvgen.h
#define		SRVCOMMDSIZE	6
#define     CLTCOMMDSIZE    8
#define     SRVSTATS        
#define     TRUE            1
#define     FALSE           0
#define     BUF_SIZE		4096
#define		SBUF_SIZE		128
#define		MAX_KICKNM_LEN	12
#define 	MAX_USR_NUM		2

#define 	CLT_STAT_CONNECT	0
#define		CLT_STAT_CHAT		1
#define 	CLT_STAT_WAIT		2
#define 	CLT_STAT_BLOCK		3
#define		CLT_STAT_TRANSFER	4

#define 	SRV_STAT_START		0
#define		SRV_STAT_END		1

#define  	NO_WARNING			0
#define  	WARNED				1

//#define   	SIZE_OF_


#define  PEER(a) ((a == a->channel->userA)?a->channel->userB:a->channel->userA)



char* SrvCommands[SRVCOMMDSIZE] = {"#START","#END","#BLOCK","#UNBLOCK","#THROWOUT","#STATS"};
char* CltCommands[CLTCOMMDSIZE] = {"#CONNECT","#CHAT","#QUIT","#TRANSFER","#FLAG","#HELP","#DISCONNECT","#TRANSFER_END"};

char* CltStatTable[5] = {"Connect","Chat","Wait","Block","Transfer"};
char* CltWrnTable[2] = {" ","Warned"};

char* HelpCommands[5] = {"#CHAT\n#FLAG\n#HELP\n#DISCONNECT",
						  "#QUIT\n#TRANSFER\n#FLAG\n#HELP\n#DISCONNECT\n",
						  "#QUIT\n#FLAG\n#HELP\n",
						  "#HELP\n#DISCONNECT\n",
						  "#QUIT\n#FLAG\n#HELP\n,#DISCONNECT\n"};

//long datamount = 0;

int (*srvcommdFunc[10])(void);
//int (*cltcommdFunc[10])(void);
// int SERVEREND = FALSE;
// int SRVSTART  = FALSE;
int sockfd;

int serverstate = SRV_STAT_END;


char chatbuffer[BUF_SIZE] = {0};


pthread_mutex_t work_mutex;


