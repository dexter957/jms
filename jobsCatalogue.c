#include "jobsCatalogue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




struct skipListNode
{
	int key; /*The key for the skip list nodes is the jobId*/
	pointerToJob theJob; /*The job*/
	pointerToSkipListNode* next;
};


int createSkipList(pointerToSkipListNode *first, pointerToSkipListNode *last)
{/*This function initialises the skip list creating the first and the last node*/
	(*first)=malloc(sizeof(struct skipListNode));
	if((*first)==NULL)
	{
		return FALSE;
	}
	else
	{
		printf("Allocated space for the first\n");
		(*first)->key=0;
		(*first)->theJob=NULL;
		(*first)->next=malloc(MAX_FORWARD*sizeof(pointerToSkipListNode));
		printf("Initialised the first node\n");
		if((*first)->next==NULL)
		{
			free((*first));
			return FALSE;
		}
		/*Everything is OK, now create the last*/
		printf("Going to allocate space for the last\n");
		(*first)->next[0]=malloc(sizeof(struct skipListNode));
		if((*first)->next[0]==NULL)
		{
			free((*first)->next);
			free((*first));
			return FALSE;
		}
		printf("Allocated space for the last\n");
		(*first)->next[0]->key=MAX_VALUE;
		(*first)->next[0]->next=malloc(MAX_FORWARD*sizeof(pointerToSkipListNode));
		printf("Initialised the last key %d\n",(*first)->next[0]->key);
		int i;
		for(i=1;i<MAX_FORWARD;++i)
		{
			/*Every entry of the first node points to the last node for now*/
			(*first)->next[i]=(*first)->next[0];
			/*Every entry of the last node point to the void*/
			(*first)->next[0]->next[i]=NULL;
		}
		(*first)->next[0]->next[0]=NULL;
		(*last)=(*first)->next[0];
		return TRUE;
	}
}


int insertInSkipList(pointerToSkipListNode *first, pointerToSkipListNode *last, int jobId, pointerToJob newJob)
{
	pointerToSkipListNode insertor=(*first);
	pointerToSkipListNode updates[MAX_FORWARD-1];
	pointerToJob oldJob;
	int i;
	for(i=MAX_FORWARD-1;i>=0;--i)
	{
		while(insertor->next[i]->key<jobId)
		{
			insertor=insertor->next[i];
		}
		updates[i]=insertor;
	}
	insertor=insertor->next[0];
	if(insertor->key==jobId)
	{
		oldJob=insertor->theJob;
		insertor->theJob=newJob;
	}
	else
	{
		printf("Inserting\n");
		//time_t t;
		//srand((unsigned) time(&t));
		//int position=(int)(rand()/RAND_MAX)*(0-(MAX_FORWARD-1)+0);/*Random number between*/
		int position=((int)rand())%MAX_FORWARD;/*Random number between*/
		printf("My position is:%d\n",position );
		insertor=malloc(sizeof(struct skipListNode));
		if(insertor==NULL)
		{
			return FALSE;
		}
		else
		{
			insertor->key=jobId;
			insertor->theJob=newJob;
			insertor->next=malloc(MAX_FORWARD*sizeof(pointerToSkipListNode));
			for(i=0;i<MAX_FORWARD;++i)
			{
				insertor->next[i]=(*last);
			}
			for(i=0;i<=position;++i)
			{
				//printf("Doing the thing\n");
				insertor->next[i]=updates[i]->next[i];
				updates[i]->next[i]=insertor;
			}
		}
	}
	return TRUE;
}


pointerToJob search(int searchKey, pointerToSkipListNode first)
{
	printf("Pool:My serachkey is:%d\n",searchKey );
	pointerToSkipListNode searcher=first;
	int i;
	for(i=MAX_FORWARD-1;i>=0;--i)
	{
		printf("Current key:%d\n",searcher->next[i]->key );
		while(searcher->next[i]->key<searchKey)
		{
			printf("In while\n");
			searcher=searcher->next[i];
		}
	}
	printf("Out of for\n");
	searcher=searcher->next[0];
	if(searcher->key==searchKey)
	{
		printf("Pool:will return sth\n");
		return searcher->theJob;
	}
	else
	{
		printf("Will return nothing\n");
		return NULL;
	}
}

pointerToJob* searchDP(int searchKey, pointerToSkipListNode first)
{
	printf("Pool:My serachkey is:%d\n",searchKey );
	pointerToSkipListNode searcher=first;
	int i;
	for(i=MAX_FORWARD-1;i>=0;--i)
	{
		printf("Current key:%d\n",searcher->next[i]->key );
		while(searcher->next[i]->key<searchKey)
		{
			printf("In while\n");
			searcher=searcher->next[i];
		}
	}
	printf("Out of for\n");
	searcher=searcher->next[0];
	if(searcher->key==searchKey)
	{
		printf("Pool:will return sth\n");
		return &(searcher->theJob);
	}
	else
	{
		printf("Will return nothing\n");
		return NULL;
	}
}

void destroySkipList(pointerToSkipListNode first, pointerToSkipListNode last)
{
	printf("Destroying skip list\n");
	pointerToSkipListNode assistant=first->next[0];
	pointerToSkipListNode toDelete;
	while(assistant->next[0]!=NULL)
	{
		toDelete=assistant;
		assistant=assistant->next[0];
		free(toDelete->theJob->name);
		free(toDelete->theJob);
		free(toDelete);
	}
	/*Free the memory allocated for the first and the last node*/
	free(first->next);
	free(last->next);
	free(first);
	free(last);
}

void printSkipList(pointerToSkipListNode first)
{
	pointerToSkipListNode assistant=first->next[0];
	while(assistant->next[0]!=NULL)
	{
		printf("%d\n",assistant->key );
		assistant=assistant->next[0];
	}
}

int skipListLength(pointerToSkipListNode first, pointerToSkipListNode last)
{
	pointerToSkipListNode assistant=first->next[0];
	int numOfJobs=0;
	while(assistant->next[0]!=NULL)
	{
		++numOfJobs;
		assistant=assistant->next[0];
	}
	return numOfJobs;
}
/*Special functions*/

void suspendJob(pointerToSkipListNode *first, int searchKey )
{/*Records that this particular job has been suspended*/
	/*First reach the right node*/
	pointerToSkipListNode searcher;
	int i;
	for(i=MAX_FORWARD-1;i>=0;--i)
	{
		while(searcher->next[i]->key<searchKey)
		{
			searcher=searcher->next[i];
		}
	}
	searcher=searcher->next[0];
	if(searcher->key==searchKey)
	{
		searcher->theJob->status=SUSPENDED;
	}
	else
	{
		;
	}
	return;
}

void activateJob(pointerToSkipListNode *first, int searchKey )
{/*Records that this particular job has been activated*/
	/*First reach the right node*/
	pointerToSkipListNode searcher;
	int i;
	for(i=MAX_FORWARD-1;i>=0;--i)
	{
		while(searcher->next[i]->key<searchKey)
		{
			searcher=searcher->next[i];
		}
	}
	searcher=searcher->next[0];
	if(searcher->key==searchKey)
	{
		searcher->theJob->status=ACTIVE;
	}
	else
	{
		;
	}
	return;
}


void terminateJob(pointerToSkipListNode *first, int searchKey )
{/*Records that this particular job has been terminated*/
	/*First reach the right node*/
	pointerToSkipListNode searcher;
	int i;
	for(i=MAX_FORWARD-1;i>=0;--i)
	{
		while(searcher->next[i]->key<searchKey)
		{
			searcher=searcher->next[i];
		}
	}
	searcher=searcher->next[0];
	if(searcher->key==searchKey)
	{
		searcher->theJob->status=TERMINATED;
	}
	else
	{
		;
	}
	return;
}

int countActiveJobs(pointerToSkipListNode *first)
{
	pointerToSkipListNode assistant=(*first)->next[0];
	int active=0;
	while(assistant->next[0]!=NULL)
	{
		if((assistant->theJob->status)==ACTIVE)
		{
			++active;
		}
		assistant=assistant->next[0];
	}
	return active;
}

int countSuspendedJobs(pointerToSkipListNode *first)
{
	pointerToSkipListNode assistant=(*first)->next[0];
	int suspended=0;
	while(assistant->next[0]!=NULL)
	{
		if((assistant->theJob->status)==SUSPENDED)
		{
			++suspended;
		}
		assistant=assistant->next[0];
	}
	return suspended;
}

int countTerminatedJobs(pointerToSkipListNode *first)
{
	pointerToSkipListNode assistant=(*first)->next[0];
	int terminated=0;
	while(assistant->next[0]!=NULL)
	{
		if((assistant->theJob->status)==TERMINATED)
		{
			++terminated;
		}
		assistant=assistant->next[0];
	}
	return terminated;
}

void getActiveJids(pointerToSkipListNode *first, int** jids)
{
	pointerToSkipListNode assistant=(*first)->next[0];
	int active=0;
	while(assistant->next[0]!=NULL)
	{
		if((assistant->theJob->status)==ACTIVE)
		{
			(*jids)[active]=assistant->theJob->jId;
			++active;
		}
		assistant=assistant->next[0];
	}
	return;
}

void getActiveTimes(pointerToSkipListNode *first, long int **times)
{
	pointerToSkipListNode assistant=(*first)->next[0];
	int active=0;
	while(assistant->next[0]!=NULL)
	{
		if((assistant->theJob->status)==ACTIVE)
		{
			(*times)[active]=assistant->theJob->totalActiveTime;
			printf("Time active:%ld\n",(*times)[active] );
			++active;
		}
		assistant=assistant->next[0];
	}
	return;
}

void getActiveTimesStart(pointerToSkipListNode *first, long int **timesStart)
{
	pointerToSkipListNode assistant=(*first)->next[0];
	int active=0;
	while(assistant->next[0]!=NULL)
	{
		if((assistant->theJob->status)==ACTIVE)
		{
			(*timesStart)[active]=assistant->theJob->lastStartTime;
			printf("Laste started:%ld\n",(*timesStart)[active] );
			++active;
		}
		assistant=assistant->next[0];
	}
	return;
}
void getAllTimesSubmitted(pointerToSkipListNode* first,long int** timesSubmitted)
{
	pointerToSkipListNode assistant=(*first)->next[0];
	int i=0;
	while(assistant->next[0]!=NULL)
	{
		(*timesSubmitted)[i]=assistant->theJob->timeSubmitted;
		printf("Time submitted:%ld\n",(*timesSubmitted)[i] );
		++i;
		assistant=assistant->next[0];
	}
	return;
}

void getTerminatedJids(pointerToSkipListNode *first, int **jids)
{
	pointerToSkipListNode assistant=(*first)->next[0];
	int terminated=0;
	while(assistant->next[0]!=NULL)
	{
		if((assistant->theJob->status)==TERMINATED)
		{
			(*jids)[terminated]=assistant->theJob->jId;
			++terminated;
		}
		assistant=assistant->next[0];
	}
	return;	
}

void setReactivationTime(pointerToSkipListNode *first, int searchKey, long int saidTime)
{
	pointerToSkipListNode searcher;
	int i;
	for(i=MAX_FORWARD-1;i>=0;--i)
	{
		while(searcher->next[i]->key<searchKey)
		{
			searcher=searcher->next[i];
		}
	}
	searcher=searcher->next[0];
	if(searcher->key==searchKey)
	{
		searcher->theJob->lastStartTime=saidTime;
	}
	else
	{
		;
	}
	return;
}

void getItAll(pointerToSkipListNode first, int** jobIds, int** procIds, jobStatus** statuses)
{
	pointerToSkipListNode assistant=first->next[0];
	int i=0;
	while(assistant->next[0]!=NULL)
	{
		(*jobIds)[i]=assistant->theJob->jId;
		(*procIds)[i]=assistant->theJob->jPid;
		(*statuses)[i]=assistant->theJob->status;
		++i;
		assistant=assistant->next[0];
	}
	return;
}


pointerToJob* searchByPid(pointerToSkipListNode first,pointerToSkipListNode last, int pid)
{
	pointerToSkipListNode assistant=first->next[0];
	int i=0;
	while(assistant->next[0]!=NULL)
	{
		if(assistant->theJob->jPid==pid)
		{
			return &(assistant->theJob);
		}
		assistant=assistant->next[0];
	}
	return NULL;
}