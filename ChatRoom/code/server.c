// server.c
/**	 
	server.c
	port number: 8787
	
	Develope Hisory:
	1. Support CONNECT command.


*/



#include"server.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>


//#define	MAXUSERNUM	8



int main(int argc, char* argv[]){
  

    char nextAct[10];	

    printf("This is Text ChatRoullette v1.0.\nAvaliable commands are:\n");
    printf("\tSTART\n\tEND\n\tBLOCK\n\tUNBLOCK\n\tTHROWOUT\n\tSTATS\n");	

    commdInit();

    do{
		printf("Admin:");
		gets(nextAct);
		if(nextAct[0] == '\0')  continue;
		(*(srvcommdFunc[srvcommandValid(nextAct)]))();

    }while(TRUE);
	

	return 0;
}

    