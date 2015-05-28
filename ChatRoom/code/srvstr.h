// srvstr.h
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#ifndef _INCLUDE_SRVGEN_H
#define _INCLUDE_SRVGEN_H
	#include "srvgen.h"
#endif

struct ChatChannel;

typedef struct UsrInfo{
	int sockfd;
	char nickname[MAX_KICKNM_LEN+1];
	int stat;
	int warning;
	struct UsrInfo* next;
	struct ChatChannel* channel;
}UsrInfo, * UsrInfo_ptr;


typedef struct ChatChannel{
	UsrInfo_ptr userA;
	UsrInfo_ptr userB;
	long dataflow;
	struct ChatChannel* next;
}ChatChannel, * CC_ptr;


UsrInfo_ptr uihead = NULL;
CC_ptr cqhead = NULL;


int AddUsrInfo(int sockfd);
int RmvUsrInfo(int sockfd);
UsrInfo_ptr GetUsrInfo(int sockfd);
int GetUsrNum();
int DeleteList();
CC_ptr AddChatChannel(UsrInfo_ptr , UsrInfo_ptr );
int RmvChatChannel(CC_ptr );
int UsrCount();



int AddUsrInfo(int sockfd){
	UsrInfo_ptr scnr;
	UsrInfo_ptr newnode = (UsrInfo_ptr)calloc(1,sizeof(UsrInfo));
	newnode->sockfd = sockfd;
	newnode->stat = 0;
	newnode->warning = NO_WARNING;

	//pthread_mutex_lock(&work_mutex);
	if(uihead == NULL){
		uihead = newnode;
	}
	else{
		scnr = uihead;
		while(scnr->next != NULL){
			scnr = scnr->next;
		}
		scnr->next = newnode;
	}
	//pthread_mutex_unlock(&work_mutex);

	return 0;
}


int RmvUsrInfo(int sockfd){
	UsrInfo_ptr scnr;
 	UsrInfo_ptr target;
 	int delete = 0;
	if(uihead == NULL){
		return 0;
	}
	else{
		scnr = uihead;
		if(scnr->sockfd == sockfd){
			uihead = scnr->next;
			free(scnr);
		}
		else{
			while(scnr->next!= NULL){
				if(scnr->next->sockfd == sockfd){
					target = scnr->next;
					scnr->next = scnr->next->next;
					free(target);
					delete = 1;
					break;
				}
				else{
					scnr = scnr->next;
					continue;
				}
			}
			if(delete == 0){
				printf("Error In RmvUsrInfo: user not found.\n");
			}
		}
	}

	return 0;
}


UsrInfo_ptr GetUsrInfo(int sockfd){
	UsrInfo_ptr scnr;

 	scnr = uihead;

	//pthread_mutex_lock(&work_mutex);
 	while(scnr != NULL){
 		if(scnr->sockfd == sockfd){
 			return scnr;
 		}
 		else{
 			scnr = scnr->next;
 		}
 	}
 	//pthread_mutex_unlock(&work_mutex);

	return NULL;
}

int GetUsrNum(){
	UsrInfo_ptr scnr;
	int total = 0;

 	scnr = uihead;
 	while(scnr != NULL){
 		total++;
 		scnr = scnr->next;
 	}

	return total;

}



int DeleteList(){
	UsrInfo_ptr scnr = uihead;
	UsrInfo_ptr temp = NULL;

	while(scnr != NULL){
		close(scnr->sockfd);
		temp = scnr;
		scnr = scnr->next;
		free(temp);
	}
	uihead = NULL;
	
	return 0;
}


CC_ptr AddChatChannel(UsrInfo_ptr userA, UsrInfo_ptr userB){
	CC_ptr scnr;
	CC_ptr newnode = (CC_ptr)calloc(1,sizeof(ChatChannel));
	newnode->userA = userA;
	newnode->userB = userB;
	newnode->dataflow = 0;
	if(cqhead == NULL){
		cqhead = newnode;
	}
	else{
		scnr = cqhead;
		while(scnr->next != NULL){
			scnr = scnr->next;
		}
		scnr->next = newnode;
	}

	return newnode;
}

int RmvChatChannel(CC_ptr rmvnode){
	CC_ptr scnr;
	int rmv_success = 0;

	scnr = cqhead;
	if(scnr != NULL){
		if(scnr == rmvnode){
			cqhead = scnr->next;
			free(scnr);
			rmv_success = 1;
		}
		else{
			while(scnr->next != NULL){
				if(scnr->next == rmvnode){
					scnr->next = rmvnode->next;
					free(rmvnode);
					rmv_success = 1;
					break;
				}
				else{
					scnr = scnr->next;
					continue;
				}
			}
		}
	}

	if(rmv_success == 0){
		printf("Error in RmvChatChannel.\n");
	}

	return 0;

}


int UsrCount(){
	UsrInfo_ptr scnr = uihead;
	int num = 0;

	while(scnr != NULL){
		num++;
		scnr = scnr->next;
	}

	return num;

}






