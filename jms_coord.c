
#include "poolList.h" /*This inclusion should not be in the header*/


/*Functions*/
void poolFifosNames(char** fromPool, char** toPool, int poolOrder);
int submit(pointerToPoolListNode *poolsList, char** execvpArray, int arrayLength,int jobsInTotal, int poolMax, int *numOfPools,
	char* commandLinePath, int fdw);
int status(int jobId, pointerToPoolListNode *listStart, int fdc);
void status_all(int timeDuration,int numOfPools, pointerToPoolListNode *listStart);
void show_active(int numOfPools, pointerToPoolListNode *listStart);
void show_pools(int numOfPools, pointerToPoolListNode *listStart);
void show_finished(pointerToPoolListNode *listStart, int numOfPools);
void suspend(int jobId, pointerToPoolListNode *listStart);
void resume(int jobId, pointerToPoolListNode *listStart);
void shutdown(pointerToPoolListNode *listStart, int numOfPools,int fdw, int maxPool);
void cleanup(int numOfPools, int fd1, int fd2,pointerToPoolListNode *listStart);
void poolsInput(int poolsToAnswer, int* asks, int** asksPids, int commandId, int pools, int* ids, int* pids, /*int* active, int* finished,*/
	jobStatus* jStatuses, int *poolsNumOfJobs, int fdw, pointerToPoolListNode *listStart, int deadPools, int maxPool, int* secsActive);
void allowExit(int asks, int *pids, pointerToPoolListNode *listStart, int* deadPools);
void addDeadpoolsToTheResult(int commandId, pointerToPoolListNode *listStart, int deadPools, int maxPool,
	int** jIds, int **jFinished, int *fCounter, int *jCounter,jobStatus **statuses);
void keepProtocol(int numOfPools,int *asks, int** asksPids, pointerToPoolListNode *listStart);
int readSubmit(int fdr,char*** execvpArray);
void freeExecvpArray(char*** execvpArray, int arrayLength);

int main(int argc, char** argv)
{
	if(argc<9)
	{
		printf("Not enough input.\nExiting\n");
		exit(1);
	}
	/*Read the input*/
	int pathPos=-1;
	int jobs_pool;
	int jmsOutPos=-1;
	int jmsInPos=-1;
	int i;
	for(i=0;i<argc;++i)
	{
		if(strcmp(argv[i],"-l")==0)
		{
			pathPos=i+1;
			continue;
		}
		if(strcmp(argv[i],"-n")==0)
		{
			sscanf(argv[i+1],"%d",&jobs_pool);
			continue;
		}
		if(strcmp(argv[i],"-w")==0)
		{
			jmsOutPos=i+1;
			continue;
		}
		if(strcmp(argv[i],"-r")==0)
		{
			jmsInPos=i+1;
			continue;
		}
	}
	printf("(%d)In jms_coord\n",getpid() );
	/*Input read, open the pipes with the console*/
	int fdw; /*File descriptor for jms_Out*/
	int fdr; /*File descriptor for jms_In*/

	/*Open the pipes (console has already created them)*/
	printf("Going to open the fifo %s to read\n",argv[jmsInPos] );
	fdr=open(argv[jmsInPos],O_RDONLY,O_NONBLOCK);
	if(fdr<0)
	{
		perror("Cannot open fifo to read");
		exit(1);
	}
	printf("Going to open the fifo %s to write\n",argv[jmsOutPos] );
	fdw=open(argv[jmsOutPos],O_WRONLY);
	if(fdw<0)
	{
		perror("Cannot open fifo to write");
		exit(1);
	}
	/*Pipes opened ok*/

	/*Now create the list of pools you're about to use*/
	pointerToPoolListNode start;
	poolListInit(&start);
	/*Pools' list ready and empty*/
	
	int numOfPools=0;/*Total number of pools, alive or dead*/
	int numOfJobs=0;/*Total number of jobs*/
	int deadPools=0;/*Number of dead pools*/
	int exitRequests=0;/*Number of requests from pools to exit*/
	int* exitPids=NULL;/*Stores the pids of pools that request to exit*/
	printf("Coord going to get into while loop\n");

	struct pollfd fdArray[1];
	int rc;
	int commandId;
	int jobId;
	int jobPid;
	int existence;
	int timeDuration;
	int arrayLength;
	char** execvpArray;
	/*These are for the answers*/
	int activeCounter;
	int finishedCounter;
	int *ids;
	int *poolsPids;
	int *poolsJobs;
	jobStatus *statuses;
	int* secsActive;
	/*Get the client's messages*/
	while(1)
	{
		//printf("Coord:In while\n");
		/*Initiate poll parameters*/
		fdArray[0].fd=fdr;
		fdArray[0].events=POLLIN;
		/*Wait for incoming data or poll timeout*/
		rc=poll(fdArray,1,300);
		if(rc==0)
		{
			//printf("Poll timed out;will try again\n");
			/*No input from console,check the pools*/
			exitPids=malloc(numOfPools*sizeof(int));
			keepProtocol(numOfPools,&exitRequests, &exitPids, &start);
			if(exitRequests>0)
			{
				allowExit(exitRequests, exitPids, &start,&deadPools);
				printf("Came back from allow exit\n");
				exitRequests=0;
			}
			if(exitPids!=NULL)
			{
				free(exitPids);
				exitPids=NULL;
			}
		}
		else
		{
			//printf("Read for poll\n");
			if((rc==1)&&(fdArray[0].revents==POLLIN))
			{
			//	printf("FIRST IF\n");
				if(fdArray[0].fd==fdr)
				{
			//		printf("SECOND IF\n");
					int bytesRead=read(fdr,&commandId,sizeof(int));
					//printf("Read:%d\n",i);
					switch(commandId)
					{
						printf("Coord:Switching\n");
						/*Submit-should read one more number indicating the length*/
						case COMMAND1:	arrayLength=readSubmit(fdr,&execvpArray);
										submit(&start, execvpArray, arrayLength, numOfJobs, jobs_pool, &numOfPools,argv[pathPos],fdw);
										++numOfJobs;
										printf("Coord:Back in main\n");
										printf("Coord:Now going to check the pools\n");
										exitPids=malloc(numOfPools*sizeof(int));
										ids=malloc(1*sizeof(int));
										printf("Coord:Allocated space for exit pids\n");
										poolsInput(1, &exitRequests, &exitPids, commandId, numOfPools, ids, NULL,NULL, NULL, 
											fdw, &start, deadPools, jobs_pool,NULL);
										printf("Coord:Back from pools input\n");
										freeExecvpArray( &execvpArray, arrayLength);
										free(ids);
						break;
						/*Status-should read one more number*/
						case COMMAND2:	read(fdr,&jobId,sizeof(int));
										printf("Coord:Status %d\n",jobId );
										if(status(jobId, &start,fdw)==TRUE)
										{/*Found in alive pool*/
											ids=malloc(1*sizeof(int));
											statuses=malloc(1*sizeof(jobStatus));
											poolsInput(1, &exitRequests, &exitPids, commandId, numOfPools, ids, NULL,statuses, NULL, 
											fdw, &start, deadPools, jobs_pool,NULL);
											free(ids);
											free(statuses);
										}
										else
										{/*Found in dead pool*/
											;
										}
						break;
						/*Status all-should read one more number*/
						case COMMAND3:	read(fdr,&existence,sizeof(int));
										if(existence==TRUE)
										{/*Read the duration*/
											read(fdr,&timeDuration,sizeof(int));
											printf("Coord:Status-all %d\n",timeDuration );
											status_all(timeDuration, numOfPools, &start);
										}
										else
										{	
											printf("Coord:Status-all\n");
											status_all(-1, numOfPools, &start);
										}
										exitPids=malloc(numOfPools*sizeof(int));
										ids=malloc(numOfJobs*sizeof(int));
										statuses=malloc(numOfJobs*sizeof(int));
										secsActive=malloc(numOfJobs*sizeof(int));
										poolsInput((numOfPools-deadPools),  &exitRequests, &exitPids,commandId,numOfPools, 
											ids, NULL,statuses, NULL, fdw,&start, deadPools, jobs_pool,secsActive);
										free(ids);
										free(statuses);
										free(secsActive);
						break;
						/*Show active*/
						case COMMAND4:	printf("Coord:Show active\n");
										/*Answer back*/
										show_active(numOfPools, &start);
										exitPids=malloc(numOfPools*sizeof(int));
										//activeCounter=0;
										ids=malloc(numOfJobs*sizeof(int));
										poolsInput((numOfPools-deadPools), &exitRequests, &exitPids, commandId, numOfPools, 
										ids, NULL,NULL, NULL, fdw, &start, deadPools,jobs_pool,NULL);
										free(ids);
						break;
						/*Show pools*/
						case COMMAND5:	printf("Coord:Show pools\n");
										if((numOfPools-deadPools)>0)
										{/*Do this IF there ARE pools*/
											show_pools(numOfPools, &start);
											exitPids=malloc(numOfPools*sizeof(int));
											poolsPids=malloc((numOfPools-deadPools)*sizeof(int));
											poolsJobs=malloc((numOfPools-deadPools)*sizeof(int));
											poolsInput((numOfPools-deadPools),  &exitRequests, &exitPids, commandId, numOfPools, 
												NULL, poolsPids, NULL, poolsJobs, fdw, &start,deadPools, jobs_pool,NULL);
											free(poolsJobs);
											free(poolsPids);
										}
										else
										{
											activeCounter=0;/*Carry the number of pools for a while*/
											write(fdw,&commandId,sizeof(int));
											write(fdw,&activeCounter,sizeof(int));
										}
						break;
						/*Show-finished*/
						case COMMAND6:	printf("Coord:Show-finished\n");
										show_finished(&start, numOfPools);
										exitPids=malloc(numOfPools*sizeof(int));
										ids=malloc(numOfJobs*sizeof(int));
										poolsInput((numOfPools-deadPools),  &exitRequests, &exitPids, commandId,numOfPools, 
										ids, NULL, NULL, NULL, fdw, &start, deadPools,jobs_pool,NULL);
										free(ids);
										/*Answer back*/
						break;
						/*Suspend-should read a job id*/
						case COMMAND7:	read(fdr,&jobId,sizeof(int));
										printf("Coord:Suspend %d\n",jobId );
										suspend(jobId, &start);
										exitPids=malloc(numOfPools*sizeof(int));
										keepProtocol(numOfPools,&exitRequests, &exitPids, &start);
						break;
						/*Resume-should read a job id*/
						case COMMAND8:	read(fdr,&jobId,sizeof(int));
										printf("Coord:Resume %d\n",jobId );
										resume(jobId, &start);
										exitPids=malloc(numOfPools*sizeof(int));
										keepProtocol(numOfPools,&exitRequests, &exitPids, &start);
						break;	
						/*Shutdown*/
						case COMMAND9:	printf("Coord:Shutdown\n");
										shutdown(&start, numOfPools,fdw, jobs_pool);
						break;

						default:	;
						break;
					}
					//fflush(stdout);
					if(commandId==COMMAND9)
					{
						break;
					}
					if(exitRequests>0)
					{
						printf("Coord:There are %d exit requests\n",exitRequests );
						allowExit(exitRequests, exitPids, &start,&deadPools);
						printf("Came back from allow exit\n");
						exitRequests=0;
					}
					if(exitPids!=NULL)
					{
						free(exitPids);
						exitPids=NULL;
					}
					
				}
			}
		}
	}
	cleanup(numOfPools,fdr,fdw,&start);
	printf("(%d)Coord exiting  . . .\n",getpid());
	return 0;
}



int submit(pointerToPoolListNode *poolsList, char** execvpArray, int arrayLength, int jobsInTotal, 
	int poolMax, int *numOfPools,char* commandLinePath, int fdw)
{
	/*First get the latest pool from the list. If it is full, fork another one*/
	printf("Coord:In submit function\n");
	if(poolListEmpty(poolsList)==FALSE)
	{
		printf("Coord:Pool list is not empty\n");
		pointerToPool* lastPool=getLatestPool(poolsList);
		if((*lastPool)->full==FALSE)
		{
			printf("Coord:There is still space in this latest pool\n");
			++((*lastPool)->jobsServing); /*This pool now serves one more job*/
			if((*lastPool)->jobsServing==poolMax)
			{
				printf("Coord:Latest pool is now full\n");
				(*lastPool)->full=TRUE;
			}
			/*Now write the info to the pool*/
			printf("Coord:Writing info to the pool\n");
			int commandId=COMMAND1;
			write((*lastPool)->fdw,&commandId,sizeof(int));
			write((*lastPool)->fdw,&arrayLength,sizeof(int));
			int i;
			int wordLength;
			for(i=0;i<arrayLength;++i)
			{
				wordLength=strlen(execvpArray[i]);
				write((*lastPool)->fdw,&wordLength,sizeof(int));
				write((*lastPool)->fdw,execvpArray[i],wordLength);
			}
			(*lastPool)->jobIds[((*lastPool)->jobsServing)-1]=jobsInTotal+1;
			printf("Coord:Gave it to the pool;returning\n");
		}
		else
		{/*Latest pool is full, so we create and fork a brand new one*/
			printf("Coord:The pool is full, going to create a new one and add it to the list\n");
			pointerToPool newPool=malloc(sizeof(struct pool));
			newPool->poolId=(*numOfPools)+1;
			(*numOfPools)=(*numOfPools)+1;
			newPool->full=FALSE;
			newPool->alive=TRUE;
			newPool->jobsServing=1;
			newPool->jobIds=malloc(poolMax*sizeof(int));
			newPool->jobIds[0]=jobsInTotal+1;
			printf("Coord:Malloced and initialised;now going to create the fifos\n");
			char* fromPool;
			char* toPool;
			poolFifosNames(&fromPool, &toPool, (*numOfPools));/*Get fifos names and create them*/
			printf("Coord:fromPool fifo name:%s\n",fromPool );
			printf("Coord:toPool fifo name:%s\n",toPool );
			if(mkfifo(fromPool,FIFOPERMS)<0)
			{
				return FALSE;
			}
			if(mkfifo(toPool,FIFOPERMS)<0)
			{
				return FALSE;
			}
			printf("Coord:fifos created, going to fork the pool\n");
			/*Now fork the pool*/
			/*Turn all args into strings*/
			char* jobsNum=malloc(INT_MAX_LENGTH*sizeof(char));
			sprintf(jobsNum,"%d",jobsInTotal);
			char* poolMaxChar=malloc(INT_MAX_LENGTH*sizeof(char));
			sprintf(poolMaxChar,"%d",poolMax);
			printf("Coord:The jobs in total to send is:%s\n", jobsNum);
			int poolPid=fork();
			if(poolPid==0)
			{
				execlp(POOLPATH,POOLEXEC,"-i",toPool,"-o",fromPool,"-l",commandLinePath,"-j",jobsNum,"-n",poolMaxChar,(char*)NULL);
				perror("Not execing");
			}
			else
			{/*Open the fifos*/
				free(jobsNum);
				free(poolMaxChar);
				printf("Coord:pool forked, going to open the fifos\n");
				newPool->fdr=open(fromPool,O_RDONLY,O_NONBLOCK);
				newPool->fdw=open(toPool,O_WRONLY);
				printf("Fifos opened\n");
				newPool->poolId=(*numOfPools);
				newPool->poolPid=poolPid;
				/*Write to the fifo*/
				int commandId=COMMAND1;
				write(newPool->fdw,&commandId,sizeof(int));
				write(newPool->fdw,&arrayLength,sizeof(int));
				int i;
				int wordLength;
				for(i=0;i<arrayLength;++i)
				{
					wordLength=strlen(execvpArray[i]);
					write(newPool->fdw,&wordLength,sizeof(int));
					write(newPool->fdw,execvpArray[i],wordLength);
				}
				printf("Coord:gave pool the info, returning\n");
				/*Keep the fifos names*/
				newPool->poolFifoRead=malloc((strlen(toPool)+1)*sizeof(char));
				newPool->poolFifoWrite=malloc((strlen(fromPool)+1)*sizeof(char));
				strcpy(newPool->poolFifoRead,toPool);
				strcpy(newPool->poolFifoWrite,fromPool);
				insertAPool(poolsList, newPool);
				free(fromPool);
				free(toPool);
			}
		}
	}
	else
	{/*Create the very first pool*/
		printf("Coord:Going to create the very first pool\n");
		pointerToPool newPool=malloc(sizeof(struct pool));
		newPool->poolId=(*numOfPools)+1;
		(*numOfPools)=(*numOfPools)+1;
		newPool->full=FALSE;
		newPool->alive=TRUE;
		newPool->jobsServing=1;
		printf("Coord:Malloced and initialised;now going to create the fifos\n");
		char* fromPool;
		char* toPool;
		poolFifosNames(&fromPool, &toPool, (*numOfPools));/*Get fifos names and create them*/
		printf("Coord:fromPool fifo name:%s\n",fromPool );
		printf("Coord:toPool fifo name:%s\n",toPool );
		newPool->jobIds=malloc(poolMax*sizeof(int));
		newPool->jobIds[0]=jobsInTotal+1;
		if(mkfifo(fromPool,FIFOPERMS)<0)
		{
			return FALSE;
		}
		if(mkfifo(toPool,FIFOPERMS)<0)
		{
			return FALSE;
		}
		printf("Coord:fifos created, going to fork the pool\n");
		/*Now fork the pool*/
		/*Turn all args into strings*/
		char* jobsNum=malloc(INT_MAX_LENGTH*sizeof(char));
		sprintf(jobsNum,"%d",jobsInTotal);
		char* poolMaxChar=malloc(INT_MAX_LENGTH*sizeof(char));
		sprintf(poolMaxChar,"%d",poolMax);
		printf("Coord:The jobs in total to send is:%s\n", jobsNum);
		int poolPid=fork();
		if(poolPid==0)
		{
			execlp(POOLPATH,POOLEXEC,"-i",toPool,"-o",fromPool,"-l",commandLinePath,"-j",jobsNum,"-n",poolMaxChar,(char*)NULL);
			perror("Not execing");
		}
		else
		{/*Open the fifos*/
			free(jobsNum);
			free(poolMaxChar);
			printf("Coord:pool forked, going to open the fifos\n");
			newPool->fdr=open(fromPool,O_RDONLY,O_NONBLOCK);
			newPool->fdw=open(toPool,O_WRONLY);
			printf("Fifos opened %d %d\n",newPool->fdr,newPool->fdw);
			newPool->poolId=(*numOfPools);
			newPool->poolPid=poolPid;
			/*Write to the fifo*/
			int commandId=COMMAND1;
			write(newPool->fdw,&commandId,sizeof(int));
			write(newPool->fdw,&arrayLength,sizeof(int));
			printf("Coord:Wrote %d arrayLength\n",arrayLength );
			int i;
			int wordLength;
			for(i=0;i<arrayLength;++i)
			{
				wordLength=strlen(execvpArray[i]);
				write(newPool->fdw,&wordLength,sizeof(int));
				write(newPool->fdw,execvpArray[i],wordLength);
				printf("Coord:Wrote %s and %d to the pool\n",execvpArray[i],wordLength );
			}
			printf("Coord:gave pool the info, returning\n");
			/*Keep the fifos names*/
			newPool->poolFifoRead=malloc((strlen(toPool)+1)*sizeof(char));
			newPool->poolFifoWrite=malloc((strlen(fromPool)+1)*sizeof(char));
			strcpy(newPool->poolFifoRead,toPool);
			strcpy(newPool->poolFifoWrite,fromPool);
			insertAPool(poolsList, newPool);
			free(fromPool);
			free(toPool);
		}
	}
	printf("Coord:Coord returning from submit\n");
	return TRUE;
}

int readSubmit(int fdr,char*** execvpArray)
{/*This function reads the submit from the one fifo and writes to the other fifo*/
	printf("Coord:In read submit\n");
	int arrayLength;
	int wordLength;
	read(fdr,&arrayLength,sizeof(int));
	printf("Coord:The arrayLength is:%d\n",arrayLength );
	(*execvpArray)=malloc(arrayLength*sizeof(char*));
	int i;
	printf("Coord:Going to read\n");
	char* token;
	for(i=0;i<arrayLength;++i)
	{
		read(fdr,&wordLength,sizeof(int));
		token=malloc((wordLength+1)*sizeof(char));
		printf("Coord:Read wordLength:%d\n",wordLength );
		(*execvpArray)[i]=malloc((wordLength+1)*sizeof(char));
		read(fdr,token,wordLength);
		token[wordLength]='\0';
		printf("Coord:Read %s\n",token);
		strcpy((*execvpArray)[i],token);
		printf("Coord:Read token:%s\n",(*execvpArray)[i] );
	}
	printf("Coord:Done reading, returning\n");
	return arrayLength;
}

void freeExecvpArray(char*** execvpArray, int arrayLength)
{/*Releases the memory occupied by the execvp array*/
	int i;
	for(i=0;i<arrayLength;++i)
	{
		free((*execvpArray)[i]);
	}
	free((*execvpArray));
}

int status(int jobId, pointerToPoolListNode *listStart, int fdc)
{
	/*Find the right pool based on the jobId*/
	pointerToPool thePool=getPoolFromJid(jobId, listStart);
	int commandId=COMMAND2;
	if(thePool->alive==TRUE)
	{
		/*Now write the command to the fifo*/
		write(thePool->fdw,&commandId,sizeof(int));
		write(thePool->fdw,&jobId,sizeof(int));
		return TRUE;
	}
	else
	{/*Write to the console*/
		jobStatus theStatus=TERMINATED;
		write(fdc,&commandId,sizeof(int));
		write(fdc,&jobId,sizeof(int));
		write(fdc,&theStatus,sizeof(jobStatus));
		return FALSE;
	}
}

void status_all(int timeDuration,int numOfPools, pointerToPoolListNode *listStart)
{
	printf("Coord:In status_all\n");
	int* fds=malloc(numOfPools*sizeof(int));
	getAllFileDescriptorsW(listStart, &fds);
	int i;
	int commandId=COMMAND3;
	int existence;
	if(timeDuration<0)
	{
		existence=FALSE;
	}
	else
	{
		existence=TRUE;
	}
	for(i=0;i<numOfPools;++i)
	{
		write(fds[i],&commandId,sizeof(int));
		write(fds[i],&existence,sizeof(int));
		if(existence==TRUE)
		{
			write(fds[i],&timeDuration,sizeof(int));
		}
	}
	free(fds);
	return;
}

void show_active(int numOfPools, pointerToPoolListNode *listStart)
{
	printf("Pool:In show active\n");
	int* fds=malloc(numOfPools*sizeof(int));
	printf("Pool:Going to get all file descriptors\n");
	getAllFileDescriptorsW(listStart, &fds);
	printf("Pool:Got all file descriptors\n");
	int i;
	int commandId=COMMAND4;
	printf("Pool:Going to write to every pool\n");
	for(i=0;i<numOfPools;++i)
	{
		printf("Pool:Sending msg\n");
		write(fds[i],&commandId,sizeof(int));
	}
	printf("Pool:Wrote to everyone, returning\n");
	free(fds);
	return;
}

void show_pools(int numOfPools, pointerToPoolListNode *listStart)
{
	printf("Coord:In show_pools\n");
	int* fds=malloc(numOfPools*sizeof(int));
	printf("Coord:Going to get the fds\n");
	getAllFileDescriptorsW(listStart, &fds);
	printf("Coord:Got the fds, going to write to them all\n");
	int i;
	int commandId=COMMAND5;
	for(i=0;i<numOfPools;++i)
	{
		write(fds[i],&commandId,sizeof(int));
	}
	printf("Coord:Wrote to all pools\n");
	free(fds);
	return;

}

void show_finished(pointerToPoolListNode *listStart, int numOfPools)
{
	int* fds=malloc(numOfPools*sizeof(int));
	getAllFileDescriptorsW(listStart, &fds);
	int i;
	int commandId=COMMAND6;
	for(i=0;i<numOfPools;++i)
	{
		write(fds[i],&commandId,sizeof(int));
	}
	free(fds);
	return;
}

void suspend(int jobId, pointerToPoolListNode *listStart)
{/*Find the right pool based on the jobId*/
	printf("Coord:In suspend\n");
	pointerToPool thePool=getPoolFromJid(jobId, listStart);
	printf("Coord:Got the pool\n");
	/*Now write the command to the fifo*/
	int commandId=COMMAND7;
	printf("Coord:Writing to the pool\n");
	write(thePool->fdw,&commandId,sizeof(int));
	write(thePool->fdw,&jobId,sizeof(int));
	printf("Coord:Wrote to the pool, returning\n");
	return;
}

void resume(int jobId, pointerToPoolListNode *listStart)
{/*Find the right pool based on the jobId*/
	printf("Coord:In resume\n");
	pointerToPool thePool=getPoolFromJid(jobId, listStart);
	printf("Coord:Got the pool\n");
	/*Now write the command to the fifo*/
	int commandId=COMMAND8;
	printf("Coord:Writing to the pool\n");
	write(thePool->fdw,&commandId,sizeof(int));
	write(thePool->fdw,&jobId,sizeof(int));
	printf("Coord:Wrote to the pool, returning\n");
	return;
}

void shutdown(pointerToPoolListNode *listStart, int numOfPools, int fdw, int maxPool)
{/*Send SIGTERM signal to all your children*/
	printf("Coord:Sending SIGTERM to pools\n");
	int* poolPids=malloc(numOfPools*sizeof(int));
	getAllPoolPids(listStart, &poolPids);
	printf("Coord:Got all pids\n");
	printf("Coord:Going to get all fdws\n");
	int* fds=malloc(numOfPools*sizeof(int));
	getAllFileDescriptorsW(listStart,&fds);
	int i;
	int commandId=COMMAND9;
	for(i=0;i<numOfPools;++i)
	{
		kill(poolPids[i],SIGTERM);/*Send SIGTERM*/
		write(fds[i],&commandId,sizeof(int));/*Tell them to shutdown*/
	}
	/*Now get their answers*/
	int jobsServedInTotal=0;
	int stillActive=0;
	int helper;
	int answered=FALSE;
	/*Now wait for your children*/
	int status;
	int counter=0;
	printf("Coord:Going to wait\n");
	while(wait(&status)>0)
	{
		;
	}
	printf("Waited, going to close all fifos\n");
	/*Close your writing end*/
	printf("Cood:Going to close my writing ends\n");
	for(i=0;i<numOfPools;++i)
	{
		close(fds[i]);
	}
	printf("Coord:Going to close my reading ends\n");
	getAllFileDescriptorsR(listStart,&fds);
	/*Close your reading end*/
	for(i=0;i<numOfPools;++i)
	{
		if(fds[i]==-1)
		{
			jobsServedInTotal+=maxPool;
			continue;
		}
		answered=FALSE;
		while(answered==FALSE)
		{
			read(fds[i],&commandId,sizeof(int));
			if(commandId==COMMAND9)
			{
				answered=TRUE;
			}
		}
		read(fds[i],&helper,sizeof(int));
		jobsServedInTotal+=helper;
		read(fds[i],&helper,sizeof(int));
		stillActive+=helper;
		close(fds[i]);
	}
	free(fds);	
	free(poolPids);
	write(fdw,&commandId,sizeof(int));
	write(fdw,&jobsServedInTotal,sizeof(int));
	write(fdw,&stillActive,sizeof(int));
	printf("Coord:Wrote %d %d %d\n",commandId,jobsServedInTotal,stillActive );
	return;
}


void poolFifosNames(char** fromPool, char** toPool, int poolOrder)
{
	char* poolNum=malloc(INT_MAX_LENGTH*sizeof(char));
	/*Turn the number into a string*/
	sprintf(poolNum,"%d",poolOrder);
	(*fromPool)=malloc((strlen(FROMPOOL)+strlen(poolNum)+1)*sizeof(char));
	strcpy((*fromPool),FROMPOOL);
	strcat((*fromPool),poolNum);
	(*toPool)=malloc((strlen(TOPOOL)+strlen(poolNum)+1)*sizeof(char));
	strcpy((*toPool),TOPOOL);
	strcat((*toPool),poolNum);
	free(poolNum);
}

void cleanup(int numOfPools, int fd1, int fd2,pointerToPoolListNode *listStart)
{
	/*Unlink all fifos created for the pools*/
	char* fromPool;
	char* toPool;
	int i;
	for(i=1;i<=numOfPools;++i)
	{
		poolFifosNames(&fromPool, &toPool, i);
		unlink(fromPool);
		unlink(toPool);
		free(fromPool);
		free(toPool);
	}
	printf("Coord:Done communicating, closing fifos and exiting\n");
	/*Close the fifos*/
	sleep(3);/*Give console the time to read*/
	close(fd1);
	close(fd2);
	/*Destroy the list*/
	deletePoolList(listStart);
}


void poolsInput(int poolsToAnswer, int* asks, int** asksPids, int commandId, int pools, int* ids, int* pids, 
	jobStatus* jStatuses, int *poolsNumOfJobs, int fdw, pointerToPoolListNode *listStart, int deadPools, int maxPool, int* secsActive)
{
	printf("Coord:In pools input\n");
	/*First get all pool ids and all file descriptors*/
	int *fdrs=malloc(pools*sizeof(int));/*File descriptors to read from the pools*/
	int *pJids=malloc(pools*sizeof(int));/*Jids of pools*/
	int *pPids=malloc(pools*sizeof(int));/*Get pids of pools*/
	getAllFileDescriptorsR(listStart, &fdrs);
	getAllPoolJids(listStart, &pJids);
	getAllPoolPids(listStart,&pPids);
	printf("Coord:Got everything I need\n");
	/*Now the other variables*/
	struct pollfd fdArray[1];
	int rc;
	long int timeActive;
	int counter=0;
	int totalFinished=0;
	int totalActive=0;
	int howMuchToRead;
	int seconds;
	int i;
	int command;
	int totalNum;
	int bytesRead=-1;
	int aCounter=0;
	printf("Coord:I expect answers from %d pools, I have %d dead pools and %d pools in total\n",poolsToAnswer,deadPools,pools );

	while(poolsToAnswer>0)
	{/*Until all the expected pools have answered to console's command*/
		for(i=0;i<pools;++i)
		{/*Listen to every pool though*/
			if(fdrs[i]==-1)
			{
				/*This is a dead pool, so move*/
				printf("Coord:Found a dead pool\n");
				continue;
			}
			/*Initiate poll parameters*/
			fdArray[0].fd=fdrs[i];
			fdArray[0].events=POLLIN;
			/*Wait for incoming data or poll timeout*/
			rc=poll(fdArray,1,300);
			if(rc==0)
			{
				;/*Poll timeout*/
			}
			else
			{/*Now the pool has either answered, or requested to terminate*/
			//	printf("Coord:Came to read a command or a request\n");
				read(fdrs[i],&command,sizeof(int));
				//printf("Read:%d\n",command );
				if(command==EXIT)
				{/*Pools has requested to exit*/
					printf("Coord:Pools with pid %d has requested to exit\n",pPids[i] );
					(*asksPids)[(*asks)]=pPids[i];
					(*asks)=(*asks)+1;
				}
				else
				{
					if(commandId==command)
					{/*You can never be too safe*/
						switch(command)
						{
							case COMMAND1:	printf("In command 1\n");
											while(bytesRead==-1)
											{
												bytesRead=read(fdrs[i],&(ids[counter]),sizeof(int));
											}
											bytesRead=-1;
											/*Now write all this stuff in the fifo to the console*/
											printf("Coord:Will send in console:jobPid:%d\n",ids[counter] );
											write(fdw,&commandId,sizeof(int));
											printf("Coord:Wrote %d\n",commandId );
											write(fdw,&(ids[counter]),sizeof(int));
											printf("Coord:Wrote %d\n", ids[counter]);
											--poolsToAnswer;
							break;
							case COMMAND2:	printf("In command 2\n");
											while(bytesRead==-1)
											{
												bytesRead=read(fdrs[i],&howMuchToRead,sizeof(int));
											}
											bytesRead=-1;
											printf("Coord:read %d\n",howMuchToRead );
											while(bytesRead==-1)
											{
												bytesRead=read(fdrs[i],&(ids[counter]),sizeof(int));
											}
											bytesRead=-1;
											printf("Coord:read %d\n",ids[counter] );
											while(bytesRead==-1)
											{
												bytesRead=read(fdrs[i],&(jStatuses[counter]),sizeof(jobStatus));
											}
											bytesRead=-1;
											printf("Coord:read %d\n",jStatuses[counter] );
											if(howMuchToRead==2)
											{
												while(bytesRead==-1)
												{
													bytesRead=read(fdrs[i],&seconds,sizeof(int));
												}
												printf("Coord:read:%d\n",seconds );
											}
											bytesRead=-1;
											printf("Coord:Will send in console:jobId:%d\n", ids[counter]);
											write(fdw,&commandId,sizeof(int));
											write(fdw,&(ids[counter]),sizeof(int));
											write(fdw,&(jStatuses[counter]),sizeof(jobStatus));
											if (howMuchToRead==2)
											{
												printf("Coord:Will write in fifo:%d\n",seconds );
												write(fdw,&seconds,sizeof(int));
											}
											--poolsToAnswer;
							break;
							case COMMAND3:	printf("In command 3\n");
											while(bytesRead==-1)
											{
												bytesRead=read(fdrs[i],&totalNum,sizeof(int));
											}
											bytesRead=-1;
											printf("Read:totalNum:%d\n",totalNum );
											write(fdw,&totalNum,sizeof(int));
											while(totalNum>0)
											{
												while(bytesRead==-1)
												{
													bytesRead=read(fdrs[i],&(ids[counter]),sizeof(int));
												}
												bytesRead=-1;
												while(bytesRead==-1)
												{
													bytesRead=read(fdrs[i],&(jStatuses[counter]),sizeof(jobStatus));
												}
												bytesRead=-1;
												if(jStatuses[counter]==ACTIVE)
												{
													while(bytesRead==-1)
													{
														bytesRead=read(fdrs[i],&(secsActive[aCounter]),sizeof(int));
													}
													bytesRead=-1;
													printf("Coord:Read %d for active time\n",secsActive[aCounter] );
													++aCounter;
												}
												printf("Coord:Will send in console:jobId:%d\n",ids[counter] );
												--totalNum;
												++counter;
											}
											--poolsToAnswer;
							break;
							case COMMAND4:	printf("In command 4\n");
											//write(fdw,&commandId,sizeof(int));
											while(bytesRead==-1)
											{
												printf("Coord:In first while\n");
												bytesRead=read(fdrs[i],&totalNum,sizeof(int));
												printf("Coord:bytesRead:%d totalNum:%d\n",bytesRead,totalNum );
											}
											bytesRead=-1;
											/*Write the total number of expecred answers to the console*/
											//write(fdw,&totalNum,sizeof(int));
											totalActive+=totalNum;
											printf("Now totalActive:%d\n",totalActive );
											while(totalNum>0)
											{
												printf("Coord:In second while\n");
												while(bytesRead==-1)
												{
													printf("Coord:In third while\n");
													bytesRead=read(fdrs[i],&(ids[counter]),sizeof(int));
												}
												bytesRead=-1;
												printf("Coord:Will send in console:jobId:%d\n",ids[counter] );
												--totalNum;
												++counter;
											}
											--poolsToAnswer;
							break;
							case COMMAND5:	printf("In command 5\n");
											while(bytesRead==-1)
											{
												bytesRead=read(fdrs[i],&(pids[counter]),sizeof(int));
												printf("Coord:Will send pid:%d\n",pids[i]);
											}
											bytesRead=-1;
											while(bytesRead==-1)
											{
												bytesRead=read(fdrs[i],&(poolsNumOfJobs[counter]),sizeof(int));
												printf("Coord:Will send numofJobs:%d\n", poolsNumOfJobs[counter] );
											}
											bytesRead=-1;
											--poolsToAnswer;
											++counter;
							break;
							case COMMAND6:	printf("In command 6\n");
											while(bytesRead==-1)
											{
												bytesRead=read(fdrs[i],&totalNum,sizeof(int));
											}
											bytesRead=-1;
											totalFinished+=totalNum;
											printf("Now totalFinished:%d\n",totalFinished );
											while(totalNum>0)
											{
												while(bytesRead==-1)
												{
													bytesRead=read(fdrs[i],&(ids[counter]),sizeof(int));
												}
												bytesRead=-1;
												printf("Coord:Will send in console:jobId:%d\n",ids[counter] );
												--totalNum;
												++counter;
											}
											--poolsToAnswer;
							break;
							default: ;
							break;
						}
					}
					
				}

			}

		}
	}
	/*Now write the results to the console*/
	switch(commandId)
	{
		case COMMAND3:	/*First write the command*/
						write(fdw,&commandId,sizeof(int));
						printf("Coord:Wrote %d\n",commandId );
						if(deadPools>0)
						{
							addDeadpoolsToTheResult(commandId,listStart,deadPools, maxPool,&ids,NULL,NULL,&counter,&jStatuses);
						}
						write(fdw,&counter,sizeof(int));
						printf("Coord:Wrote %d\n",counter );
						aCounter=0;
						for(i=0;i<counter;++i)
						{
							write(fdw,&ids[i],sizeof(int));
							write(fdw,&jStatuses[i],sizeof(jobStatus));
							if(jStatuses[i]==ACTIVE)
							{
								write(fdw,&(secsActive[aCounter]),sizeof(int));
								++aCounter;
							}
							printf("Coord:Wrote to console %d\n",ids[i] );
						}
		break;
		case COMMAND4:	/*First write the command*/
						write(fdw,&commandId,sizeof(int));
						printf("Coord:Wrote %d\n",commandId );
						printf("Coord: counter %d and totalActive %d\n",counter, totalActive );
						write(fdw,&counter,sizeof(int));
						for(i=0;i<counter;++i)
						{
							write(fdw,&ids[i],sizeof(int));
							printf("Coord:Wrote:%d\n",ids[i] );
						}
		break;
		case COMMAND5:	/*First write the command*/
						write(fdw,&commandId,sizeof(int));
						printf("Coord:Wrote %d\n",commandId );
						write(fdw,&counter,sizeof(int));
						for(i=0;i<counter;++i)
						{
							write(fdw,&(pids[i]),sizeof(int));
							write(fdw,&(poolsNumOfJobs[i]),sizeof(int));
							printf("Coord:Wrote:%d and %d\n",pids[i],poolsNumOfJobs[i] );
						}
		break;
		case COMMAND6:	/*First write the command*/
						write(fdw,&commandId,sizeof(int));
						printf("Coord:Wrote %d\n",commandId );
						if(deadPools>0)
						{
							addDeadpoolsToTheResult(commandId,listStart,deadPools, maxPool,NULL,&ids,&totalFinished,NULL,NULL);
						}
						write(fdw,&totalFinished,sizeof(int));
						for(i=0;i<totalFinished;++i)
						{
							write(fdw,&ids[i],sizeof(int));
							printf("Coord:Wrote finished job %d\n",ids[i] );
						}
		break;
		default:	;
		break;
	}
	free(fdrs);
	free(pJids);
	free(pPids);
	return;
}

void allowExit(int asks, int *pids, pointerToPoolListNode *listStart, int* deadPools)
{/*Handles the asks from pools to leave*/
	printf("Coord:In allow exit there are %d pools who wish to exit\n",asks );
	int i;
	pointerToPool *dPool;
	int exitPerm=YESEXIT;
	int status;
	for(i=0;i<asks;++i)
	{
		printf("Coord:Pool pid %d\n",pids[i] );
		dPool=getPoolFromItsPid(pids[i], listStart);
		printf("Coord:Got the pool\n");
		/*Get each writing end and tell them to exit*/
		write((*dPool)->fdw,&exitPerm,sizeof(int));
		printf("Coord:Game permission to exit\n");
		sleep(3);/*Give the pools time to read*/
		/*Close the file descriptors*/
		close((*dPool)->fdw);
		close((*dPool)->fdr);
		/*Indicate the closed state of these file descriptors*/
		(*dPool)->fdw=-1;
		(*dPool)->fdr=-1;
		/*Set their state as dead in the list*/
		(*dPool)->alive=FALSE;
		++(*deadPools);
		printf("Now there are %d \n",(*deadPools) );
	}
	for(i=0;i<asks;++i)
	{
		/*Wait*/
		printf("Waiting . . .\n");
		wait(&status);
	}
	printf("Coord:Waited\n");
	return;
}


void addDeadpoolsToTheResult(int commandId, pointerToPoolListNode *listStart, int deadPools, int maxPool,
	int** jIds, int **jFinished, int *fCounter, int *jCounter,jobStatus **statuses)
{
	/*First get the smaller jId each dead pool served*/
	printf("Came to add dead pools in the result\n");
	int *smJids=malloc(deadPools*sizeof(int));
	getAllFirstJidsFromDeadPools(listStart,&smJids);
	int i;
	int j;
	if(commandId==COMMAND3)
	{
		for(j=0;j<deadPools;++j)
		{
			for(i=0;i<maxPool;++i)
			{
				printf("smJid[%d]:%d\n",j,smJids[j] );
				(*jIds)[(*jCounter)]=smJids[j]+i;
				(*statuses)[(*jCounter)]=TERMINATED;
				++(*jCounter);
			}
		}
	}
	else if(commandId==COMMAND6)
	{
		for(j=0;j<deadPools;++j)
		{
			for(i=0;i<maxPool;++i)
			{
				(*jFinished)[(*fCounter)]=smJids[j]+i;
				++(*fCounter);
			}
		}

	}
	free(smJids);
	return;
}

void keepProtocol(int numOfPools,int *asks, int** asksPids, pointerToPoolListNode *listStart)
{/*Called when poolsInput not called*/
/*Checks if pools have anything to say*/
	printf("Coord:In keep protocol\n");
	/*First get all pool ids and all file descriptors*/
	int *fdrs=malloc(numOfPools*sizeof(int));/*File descriptors to read from the pools*/
	int *pJids=malloc(numOfPools*sizeof(int));/*Jids of pools*/
	int *pPids=malloc(numOfPools*sizeof(int));/*Get pids of pools*/
	getAllFileDescriptorsR(listStart, &fdrs);
	getAllPoolJids(listStart, &pJids);
	getAllPoolPids(listStart,&pPids);
	printf("Coord:Got everything I need\n");
	int i;
	int command;
	/*Now the other variables*/
	struct pollfd fdArray[1];
	int rc;
	for(i=0;i<numOfPools;++i)
	{
		if(fdrs[i]==-1)
		{
			/*This is a dead pool, so move*/
			printf("Coord:Found a dead pool\n");
			continue;
		}
		/*Initiate poll parameters*/
		fdArray[0].fd=fdrs[i];
		fdArray[0].events=POLLIN;
		/*Wait for incoming data or poll timeout*/
		rc=poll(fdArray,1,300);
		if(rc==0)
		{
			;/*Poll timeout*/
		}
		else
		{/*Now the pool has either answered, or requested to terminate*/
			printf("Coord:Came to read a command or a request\n");
			read(fdrs[i],&command,sizeof(int));
			printf("Read:%d\n",command );
			if(command==EXIT)
			{/*Pools has requested to exit*/
				printf("Coord:Pools with pid %d has requested to exit\n",pPids[i] );
				(*asksPids)[(*asks)]=pPids[i];
				(*asks)=(*asks)+1;
			}
		}
	}
	free(fdrs);
	free(pJids);
	free(pPids);
	return;
}