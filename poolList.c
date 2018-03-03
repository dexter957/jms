#include "poolList.h"
#include <stdio.h>
#include <stdlib.h>


/*
This list describes the pools in use by the jms_coord. Each new pool is inserted at the start of the list, for efficiency reasons.
*/

struct poolListNode
{
	pointerToPool thePool;
	pointerToPoolListNode next;
};


void poolListInit(pointerToPoolListNode *listStart)
{
	(*listStart)=NULL;
}

int poolListEmpty(pointerToPoolListNode *listStart)
{
	if((*listStart)==NULL)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int insertAPool(pointerToPoolListNode *listStart, pointerToPool newPool)
{
	pointerToPoolListNode newPoolNode=malloc(sizeof(struct poolListNode));
	if(newPoolNode==NULL)
	{
		return FALSE;
	}
	else
	{
		newPoolNode->thePool=newPool;
		if(poolListEmpty(listStart)==TRUE)
		{/*This is the first pool node to be inserted*/
			newPoolNode->next=NULL;
			(*listStart)=newPoolNode;
			return TRUE;
		}
		else
		{/*This is not the first node to be inserted*/
			newPoolNode->next=(*listStart);
			(*listStart)=newPoolNode;
			return TRUE;
		}
	}
}


void deletePoolList(pointerToPoolListNode *listStart)
{
	pointerToPoolListNode assistant=(*listStart);
	pointerToPoolListNode toDelete;
	while(assistant!=NULL)
	{
		toDelete=assistant;
		assistant=assistant->next;
		free(toDelete->thePool->poolFifoRead);
		free(toDelete->thePool->poolFifoWrite);
		free(toDelete->thePool->jobIds);
		free(toDelete->thePool);
		free(toDelete);
	}
}

int listLength(pointerToPoolListNode *listStart)
{
	pointerToPoolListNode assistant=(*listStart);
	int nodes=0;
	while(assistant!=NULL)
	{
		++nodes;
		assistant=assistant->next;
	}
	return nodes;
}

pointerToPool* getLatestPool(pointerToPoolListNode *listStart)
{
	return &((*listStart)->thePool);
}


/*Other functions*/

pointerToPool getPoolFromJid(int jobId, pointerToPoolListNode* listStart)
{
	printf("Coord:Getting pool from jId\n");
	pointerToPoolListNode assistant=(*listStart);
	int i;
	while(assistant!=NULL)
	{
		for(i=0;i<(assistant->thePool->jobsServing);++i)
		{
			printf("%d\n",assistant->thePool->jobIds[i] );
			if((assistant->thePool->jobIds[i])==jobId)
			{
				return assistant->thePool;
			}
		}
		assistant=assistant->next;
	}
	return NULL;
}
pointerToPool* getPoolFromItsPid(int pId, pointerToPoolListNode* listStart)
{
	printf("Coord:Getting pool from pId\n");
	pointerToPoolListNode assistant=(*listStart);
	int i;
	while(assistant!=NULL)
	{
		if(assistant->thePool->poolPid==pId)
		{
			return &(assistant->thePool);
		}
		assistant=assistant->next;
	}
	return NULL;
}

void getAllFileDescriptorsW(pointerToPoolListNode* listStart, int** fds)
{
	printf("Getting all file descriptors to write\n");
	pointerToPoolListNode assistant=(*listStart);
	int i=0;
	while(assistant!=NULL)
	{
		(*fds)[i]=assistant->thePool->fdw;
		++i;
		assistant=assistant->next;
	}
	printf("Got them;returning\n");
	//for(i=0;i<arrayLength;++i)
	//{
	//	(*fds)[i]=assistant->fdw;		
	//}
	return;
}

void getAllFileDescriptorsR(pointerToPoolListNode* listStart, int** fds)
{
	pointerToPoolListNode assistant=(*listStart);
	int i=0;
	while(assistant!=NULL)
	{
		(*fds)[i]=assistant->thePool->fdr;
		++i;
		assistant=assistant->next;
	}
	//for(i=0;i<arrayLength;++i)
	//{
	//	(*fds)[i]=assistant->fdw;		
	//}
	return;
}

void getAllPoolPids(pointerToPoolListNode* listStart, int** pids)
{/*Get all pids*/
	pointerToPoolListNode assistant=(*listStart);
	int i=0;
	while(assistant!=NULL)
	{
		(*pids)[i]=assistant->thePool->poolPid;
		++i;
		assistant=assistant->next;
	}
	//for(i=0;i<arrayLength;++i)
	//{
	//	(*fds)[i]=assistant->fdw;		
	//}
	return;
}

void getAllPoolJids(pointerToPoolListNode* listStart, int** jids)
{/*Get all pids*/
	pointerToPoolListNode assistant=(*listStart);
	int i=0;
	while(assistant!=NULL)
	{
		(*jids)[i]=assistant->thePool->poolId;
		++i;
		assistant=assistant->next;
	}
	//for(i=0;i<arrayLength;++i)
	//{
	//	(*fds)[i]=assistant->fdw;		
	//}
	return;
}

int getFdRFromPid(pointerToPoolListNode* listStart, int jId)
{
	pointerToPoolListNode assistant=(*listStart);
	while(assistant!=NULL)
	{
		if(assistant->thePool->poolId==jId)
		{
			return assistant->thePool->fdr;
		}
		assistant=assistant->next;
	}
	return -1;/*Not found*/
}

void getAllFirstJidsFromDeadPools(pointerToPoolListNode *listStart, int** jIds)
{
	pointerToPoolListNode assistant=(*listStart);
	int counter=0;
	while(assistant!=NULL)
	{
		if(assistant->thePool->alive==FALSE)
		{
			(*jIds)[counter]=assistant->thePool->jobIds[0];
			++counter;
		}
		assistant=assistant->next;
	}
	return;
}