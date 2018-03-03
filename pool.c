
#include "jobsCatalogue.h" /*This inclusion should not be in the header*/


/*SIGTERM HANDLING*/
void catchSIGTERM(int signo){
	printf("\nPool Intercepting: signo=%d\n",signo);
	//goodbye();
	//printf("Catching: returning\n");
	//exit(0);
	}

/*The functions*/
void submit(char* directoryUnder, char** execvpArray, pointerToSkipListNode* header, pointerToSkipListNode* last, int jobsAlready, int fd);
void status(int fd,int jobId, pointerToSkipListNode header);
void status_all(int fd, pointerToSkipListNode *header, pointerToSkipListNode *tail, int timeDuration);
void show_active(int fd, pointerToSkipListNode *header);
void show_pools(int fd, pointerToSkipListNode *header, pointerToSkipListNode *tail);
void show_finished(int fd,pointerToSkipListNode *header);
void suspend(int jobId,pointerToSkipListNode* header);
void resume(int jobId, pointerToSkipListNode *header);
void shutdown(int fd,pointerToSkipListNode *header, pointerToSkipListNode *tail);
void cleanup(pointerToSkipListNode header, pointerToSkipListNode tail, int fd1, int fd2);
void jobOutputFileName(char** fileName, int jobId, char* programDirectory);
void jobErrorFileName(char** fileName, int jobId, char* programDirectory);
void directoryName(char* dirPath, int jobId, char**dirName);
void dateConvertor(char** dateTimeBasedName);
void periodicalWait(pointerToSkipListNode* header, pointerToSkipListNode* tail, int* finishedJobs);
int readSubmit(int fdr,char*** execvpArray);
void freeExecvpArray(char*** execvpArray, int arrayLength);

int main(int argc, char** argv)
{
	int i;
	int inputFifoPos=-1;
	int outputFifoPos=-1;
	int pathPos=-1;
	int jobsAlready=-1;
	int maxPool=-1;
	for(i=0;i<argc;++i)
	{
		if(strcmp(argv[i],"-i")==0)
		{
			inputFifoPos=i+1;
			continue;
		}
		if(strcmp(argv[i],"-o")==0)
		{
			outputFifoPos=i+1;
			continue;
		}
		if(strcmp(argv[i],"-l")==0)
		{
			pathPos=i+1;
			continue;
		}
		if(strcmp(argv[i],"-j")==0)
		{
			sscanf(argv[i+1],"%d",&jobsAlready);
			continue;
		}	
		if(strcmp(argv[i],"-n")==0)
		{
			sscanf(argv[i+1],"%d",&maxPool);
			continue;
		}	
	}
/**************************************************/
	/*SIGTERM HANDLING*/
	static struct sigaction act;

	act.sa_handler=catchSIGTERM;
	sigfillset(&(act.sa_mask));

	sigaction(SIGTERM, &act, NULL);
	/*END OF SIGTERM HANDLING*/
/**************************************************/

	printf("Pool:Got input arguments, going to create the skip list\n");
	/*Got the arguments, now go create the skip list*/
	pointerToSkipListNode header=NULL;
	pointerToSkipListNode tail=NULL;
	createSkipList(&header,&tail);
	printf("Pool:Skip list created\n");
	/*Seed rand for skip list insertion*/
	time_t t;
	srand((unsigned) time(&t));

	/*Got the arguments, now go open the fifos*/
	int fdw;
	int fdr;
	/*Open the fifo to write*/
	printf("Pool:Going to open writing fifo:%s\n",argv[outputFifoPos] );
	fdw=open(argv[outputFifoPos],O_WRONLY);
	if(fdw<0)
	{
		perror("Cannot open fifo to write");
		exit(1);
	}
	/*Open the fifo to read*/
	printf("Pool:Going to open reading fifo:%s\n",argv[inputFifoPos] );
	fdr=open(argv[inputFifoPos],O_RDONLY,O_NONBLOCK);
	if(fdr<0)
	{
		perror("Cannot open fifo to read");
		close(fdw);
		exit(1);
	}
	printf("Pool:fifos opened ok, going to main job\n");
	/*Fifos opened ok, now go to your main job*/
	/*Check jms_coord's input and do what you have to*/
	struct pollfd fdArray[1];
	int rc;
	int commandId;
	int jobId;
	int jobPid;
	int existence;
	int timeDuration;
	int arrayLength;
	char** execvpArray;

	int finishedJobs=0;
	int askedToExit=FALSE;
	/*Get the client's messages*/
	while(1)
	{
		//printf("%s\n", );
		/*Initiate poll parameters*/
		fdArray[0].fd=fdr;
		fdArray[0].events=POLLIN;
		/*Wait for incoming data or poll timeout*/
		rc=poll(fdArray,1,300);
		if(rc==0)
		{
			///printf("Poll timed out;will try again\n");
			;
		}
		else
		{
			//printf("Read for poll\n");
			if((rc==1)&&(fdArray[0].revents==POLLIN))
			{
				if(fdArray[0].fd==fdr)
				{
					int bytesRead=read(fdr,&commandId,sizeof(int));
					//printf("Read:%d\n",i);
					switch(commandId)
					{
						printf("Pool:Switching\n");
						/*Submit-should read one more number indicating the length*/
						case COMMAND1:	arrayLength=readSubmit(fdr,&execvpArray);
										submit(argv[pathPos], execvpArray, &header, &tail, jobsAlready, fdw);
										//free(programName);
										++jobsAlready;
										freeExecvpArray(&execvpArray, arrayLength);
										printf("Pool:Now we have %d jobs\n",jobsAlready );
						break;
						/*Status-should read one more number*/
						case COMMAND2:	read(fdr,&jobId,sizeof(int));
										printf("Pool:Status %d\n",jobId );
										status(fdw,jobId, header);
						break;
						/*Status all-should read one more number*/
						case COMMAND3:	read(fdr,&existence,sizeof(int));
										if(existence==TRUE)
										{/*Read the duration*/
											read(fdr,&timeDuration,sizeof(int));
											printf("Pool:Status-all %d\n",timeDuration );
											status_all(fdw, &header, &tail,timeDuration);
										}
										else
										{	
											printf("Pool:Status-all\n");
											status_all(fdw, &header, &tail,-1);
										}
						break;
						/*Show active*/
						case COMMAND4:	printf("Pool:Show active\n");
										/*Answer back*/
										show_active(fdw, &header);
						break;
						/*Show pools*/
						case COMMAND5:	printf("Pool:Show pools\n");
										show_pools(fdw, &header, &tail);
						break;
						/*Show-finished*/
						case COMMAND6:	printf("Pool:Show-finished\n");
										/*Answer back*/
										show_finished(fdw,&header);
						break;
						/*Suspend-should read a job id*/
						case COMMAND7:	read(fdr,&jobId,sizeof(int));
										printf("Pool:Suspend %d\n",jobId );
										suspend(jobId,&header);
						break;
						/*Resume-should read a job id*/
						case COMMAND8:	read(fdr,&jobId,sizeof(int));
										printf("Pool:Resume %d\n",jobId );
										resume(jobId, &header);
						break;	
						/*Shutdown*/
						case COMMAND9:	printf("Pool:Shutdown\n");
										shutdown(fdw,&header, &tail);
						break;

						case YESEXIT: 	printf("Pool:Coord let me exit\n");
										close(fdr);
										close(fdw);
										exit(0);
						break;

						default:	;
						break;
					}
					//fflush(stdout);
					if(commandId==COMMAND9)
					{
						break;
					}
				}
			}
		}
		periodicalWait(&header, &tail,&finishedJobs);/*Check if a child has exited*/
		if((finishedJobs==maxPool)&&(askedToExit==FALSE))
		{/*Max num of jobs has finished, so the pool must terminate*/
			printf("Pool:I WILL REQUEST PERMISSION TO EXIT\n");
			int permission=EXIT;
			write(fdw,&permission,sizeof(int));
			askedToExit=TRUE;/*Will only ask once*/
		}
	}
	cleanup(header, tail, fdr, fdw);
	printf("(%d)Pool exiting . . .\n",getpid() );
	return 0;
}


void submit(char* directoryUnder, char** execvpArray, pointerToSkipListNode* header, pointerToSkipListNode* last, 
	int jobsAlready, int fd)
{/*Job submission*/
/*First create the job struct*/
	printf("Pool:In submit function\n");
	pointerToJob newJob=malloc(sizeof(struct job));
	printf("Pool:Allocated space\n");
	/*Fork the child and exec*/
	int pid=fork();
	if(pid==0)
	{
		/*Child will create its directory*/
		char* programDirectory;
		directoryName(directoryUnder, (jobsAlready+1), &programDirectory);
		mkdir(programDirectory,PERMS);
		/*Now the output file*/
		char* outputFile;
		jobOutputFileName(&outputFile, (jobsAlready+1), programDirectory);
		printf("Pool Child:Created output file %s\n",outputFile );
		/*Now the error file*/
		char* errorFile;
		jobErrorFileName(&errorFile, (jobsAlready+1), programDirectory);
		printf("Created error file %s\n",errorFile );
		/*And now map the file descriptors*/
		int fdOut=open(outputFile,O_WRONLY|O_CREAT, PERMS);
		int fdErr=open(errorFile,O_WRONLY|O_CREAT,PERMS);
		dup2(fdOut,1);
		dup2(fdErr,2);
		close(fdOut);
		close(fdErr);
		/*Last cleanup before exec*/
		free(programDirectory);
		free(outputFile);
		free(errorFile);
		/*Go to your code*/
		//execlp(program,program,(char*)NULL);
		printf("Pool:Sending %s for execution\n",execvpArray[0] );
		execvp(execvpArray[0],&execvpArray[0]);
		perror("Pool not execing");
	}
	/*Add process struct in the list*/
	printf("Pool intialising the rest of the kid\n");
	newJob->jPid=pid;
	newJob->jId=jobsAlready+1;
	newJob->name=malloc((strlen(execvpArray[0])+1)*sizeof(char));
	strcpy(newJob->name,execvpArray[0]);
	newJob->status=ACTIVE;
	newJob->timeSubmitted=time(NULL);
	newJob->lastStartTime=time(NULL);
	printf("Pool:Inserting in skip list\n");
	newJob->totalActiveTime=0;
	insertInSkipList(header, last,jobsAlready+1, newJob);
	int commandId=COMMAND1;
	printf("Pool:Going to write in fifo\n");
	write(fd,&commandId,sizeof(int));
	write(fd,&pid,sizeof(int));
	printf("Pool:Wrote %d and %d in fifo\n",commandId,pid );
	printf("Pool:Inserted in skip list\n");
	return;
}

int readSubmit(int fdr,char*** execvpArray)
{/*This function reads the submit from the one fifo and writes to the other fifo*/
	printf("Pool:In read submit\n");
	int arrayLength;
	int wordLength;
	read(fdr,&arrayLength,sizeof(int));
	printf("Pool:The arrayLength is:%d\n",arrayLength );
	(*execvpArray)=malloc(arrayLength*sizeof(char*));
	int i;
	printf("Pool:Going to read\n");
	for(i=0;i<arrayLength;++i)
	{
		read(fdr,&wordLength,sizeof(int));
		printf("Pool:Read wordLength:%d\n",wordLength );
		(*execvpArray)[i]=malloc((wordLength+1)*sizeof(char));
		read(fdr,(*execvpArray)[i],wordLength);
		(*execvpArray)[i][wordLength]='\0';
		printf("Pool:Read token:%s\n",(*execvpArray)[i] );
	}
	printf("Pool:Done reading, returning\n");
	return arrayLength;
}

void freeExecvpArray(char*** execvpArray, int arrayLength)
{/*Releases the memory occupied by the array fro execvp*/
	int i;
	for(i=0;i<arrayLength;++i)
	{
		free((*execvpArray)[i]);
	}
	free((*execvpArray));
}

void status(int fd,int jobId, pointerToSkipListNode header)
{
	/*First get the job from the skip list*/
	printf("Pool:In status function\n");
	pointerToJob theJob=search(jobId, header);
	int commandId=COMMAND2;
	int howMuchToRead=1;
	int seconds;
	/*Now get your info*/
	//(*status)=theJob->status;
	if(theJob->status==ACTIVE)
	{
		printf("Pool:Need to write time also\n");
		++howMuchToRead;
		//(*seconds)=(int)theJob->totalActiveTime+(int)((theJob->lastStartTime)-time(NULL));
		printf("Active time %ld\n",theJob->totalActiveTime );
		printf("Last start time %ld\n",theJob->lastStartTime );
		seconds=(int)(theJob->totalActiveTime)+(int)(time(NULL)-(theJob->lastStartTime));
	}
	printf("Pool:Going to write now\n");
	write(fd,&commandId,sizeof(int));
	write(fd,&howMuchToRead,sizeof(int));
	write(fd,&jobId,sizeof(int));
	write(fd,&(theJob->status),sizeof(int));
	printf("Pool:wrote %d %d %d\n",commandId,howMuchToRead,theJob->status );
	if(howMuchToRead==2)
	{
		write(fd,&seconds,sizeof(int));
		printf("Pool:Wrote %d\n",seconds );
	}
	return;
}

void status_all(int fd, pointerToSkipListNode *header, pointerToSkipListNode *tail, int timeDuration)
{/*Get status for all processes of a pool*/
	printf("Pool:In status-all\n");
	int jobsNum=skipListLength((*header), (*tail));
	int* pJids=malloc(jobsNum*sizeof(int));
	int* pPids=malloc(jobsNum*sizeof(int));
	jobStatus* pJstatuses=malloc(jobsNum*sizeof(jobStatus));
	/*Now get all the info*/
	getItAll((*header), &pJids, &pPids, &pJstatuses);
	int activeJobs=countActiveJobs(header);
	long int *times=malloc(activeJobs*sizeof(long int));
	long int *timesStart=malloc(activeJobs*sizeof(long int));
	long int *timesSubmitted=malloc(jobsNum*sizeof(long int));
	getActiveTimes(header, &times);
	getActiveTimesStart(header, &timesStart);
	getAllTimesSubmitted(header,&timesSubmitted);
	int activeTime;
	/*Now for each job, write their status into the fifo*/
	int i;
	int aCounter=0;
	int actual=jobsNum;
	for(i=0;i<jobsNum;++i)
	{/*If you have timeDuration, you may need to write less than all of the jobs*/
		if((timeDuration!=-1)&&((int)(time(NULL)-timesSubmitted[i])>timeDuration))
		{
			--actual;
			continue;
		}
	}
	int commandId=COMMAND3;
	write(fd,&commandId,sizeof(int));
	write(fd,&actual,sizeof(int));
	for(i=0;i<jobsNum;++i)
	{
		if((timeDuration!=-1)&&((int)(time(NULL)-timesSubmitted[i])>timeDuration))
		{
			continue;
		}
		write(fd,&(pJids[i]),sizeof(int));
		write(fd,&(pJstatuses[i]),sizeof(int));
		if(pJstatuses[i]==ACTIVE)
		{
			activeTime=(int)(times[aCounter]+(time(NULL)-timesStart[aCounter]));
			write(fd,&activeTime,sizeof(int));
			printf("Pool:Wrote active time %d\n",activeTime);
			++aCounter;
		}
		printf("Pool:Wrote %d %d\n", pJids[i],pJstatuses[i]);
	}
	free(pJids);
	free(pPids);
	free(pJstatuses);
	free(timesStart);
	free(times);
	return;
}

void show_active(int fd, pointerToSkipListNode *header)
{
	printf("Pool:In show active\n");
	/*First get the number of active jobs*/
	printf("Pool:Going to count the active jobs\n");
	int activeJobs=countActiveJobs(header);
	printf("Pool:Counted active jobs\n");
	/*Now get each active job, as well as their running time*/
	int* jids=malloc(activeJobs*sizeof(int));
	printf("Pool:Going to get the active jids\n");
	getActiveJids(header, &jids);
	/*Now write in the fifo*/
	/*First write the command you are answering*/
	int commandId=COMMAND4;
	write(fd,&commandId,sizeof(int));
	/*Now write how many active jobs there are*/
	write(fd,&activeJobs,sizeof(int));
	/*Now write for each active job the pid and for how long it has been running*/
	int i;
	for(i=0;i<activeJobs;++i)
	{
		write(fd,&jids[i],sizeof(int));
		printf("Pool:Wrote %d jid\n",jids[i] );
	}
	//free(times);
	free(jids);
	return;
}

void show_pools(int fd, pointerToSkipListNode *header, pointerToSkipListNode *tail)
{
	printf("Pool:In show_pools\n");
	/*Show how many jobs you have yourself*/
	printf("Pool:Going to get how many jobs I serve\n");
	int myJobs=skipListLength((*header), (*tail));
	int mypid=getpid();
	int commandId=COMMAND5;
	write(fd,&commandId,sizeof(int));
	write(fd,&mypid,sizeof(int));
	write(fd,&myJobs,sizeof(int));
	printf("Pool:Sent %d %d and %d\n",commandId,mypid,myJobs );
	return;
}

void show_finished(int fd,pointerToSkipListNode *header)
{
	/*First get how many finished jobs you have*/
	int terminatedJobs=countTerminatedJobs(header);
	/*Now get their jids*/
	int* jids=malloc(terminatedJobs*sizeof(int));
	getTerminatedJids(header, &jids);
	/*Now write in the fifo*/
	int commandId=COMMAND6;
	write(fd,&commandId,sizeof(int));
	write(fd,&terminatedJobs,sizeof(int));
	int i;
	for(i=0;i<terminatedJobs;++i)
	{
		write(fd,&jids[i],sizeof(int));
	}
	free(jids);
	return;
}

void suspend(int jobId,pointerToSkipListNode* header)
{
	printf("Pool:In suspend function\n");
	/*Send a signal that suspends the job*/
	pointerToJob* theJob=searchDP(jobId, (*header));
	printf("Pool:Got the job\n");
	//long int lastStartTime=(*theJob)->lastStartTime;
	kill((*theJob)->jPid,SIGSTOP);
	printf("Pool:Sent suspend signal\n");
	printf("Pool:totalActiveTime so far %ld\n",(*theJob)->totalActiveTime );
	(*theJob)->totalActiveTime=(*theJob)->totalActiveTime+(time(NULL)-(*theJob)->lastStartTime);
	printf("Pool:totalActiveTime now %ld\n",(*theJob)->totalActiveTime );
	/*Record the suspension*/
	(*theJob)->status=SUSPENDED;
	(*theJob)->lastStartTime=0; /*To measure active time more accurately*/
	return;
}

void resume(int jobId, pointerToSkipListNode *header)
{
	printf("Pool:In resume function\n");
	/*Send a signal that re-activates the job*/
	pointerToJob* theJob=searchDP(jobId, (*header));
	printf("Pool:Got the job\n");
	kill((*theJob)->jPid,SIGCONT);
	printf("Pool:Sent the signal\n");
	/*Record the re-activation*/
	(*theJob)->status=ACTIVE;
	(*theJob)->lastStartTime=time(NULL);
	printf("Pool:Done resuming\n");
	return;
}


void shutdown(int fd,pointerToSkipListNode *header, pointerToSkipListNode *tail)
{
	printf("Pool:Shuting down\n");
	printf("Poll:Going to get skipListLength\n");
	int totalJobs=skipListLength((*header), (*tail));
	printf("Pool:Got skiplist length\n");
	printf("Pool:Going to get active jobs\n");
	int activeJobs=countActiveJobs(header);
	printf("Got active jobs\n");
	int suspendedJobs=countSuspendedJobs(header);
	/*Get all the jIds, pIds and statuses*/
	int *jids=malloc(totalJobs*sizeof(int));
	int *pids=malloc(totalJobs*sizeof(int));
	jobStatus *statuses=malloc(totalJobs*sizeof(jobStatus));
	getItAll((*header),&jids,&pids,&statuses);
	/*Now terminate every non alredy terminated process*/
	printf("Going to send SIGTERM\n");
	int i;
	for(i=0;i<totalJobs;++i)
	{
		if(statuses[i]!=TERMINATED)
		{
			printf("Pool:Sending SIGTERM to %d\n",pids[i] );
			kill(pids[i],SIGTERM);
		}
	}
	/*Now write in the fifo*/
	int commandId=COMMAND9;
	activeJobs+=suspendedJobs;
	write(fd,&commandId,sizeof(int));
	write(fd,&totalJobs,sizeof(int));
	write(fd,&activeJobs,sizeof(int));
	printf("Pool:wrote %d %d and %d\n",commandId,totalJobs,activeJobs );
	free(jids);
	free(pids);
	free(statuses);
	return;
}


void cleanup(pointerToSkipListNode header, pointerToSkipListNode tail, int fd1, int fd2)
{
	/*Close the file descriptors*/
	close(fd1);
	close(fd2);	
	/*Free the memory occupied by the skip list*/
	destroySkipList(header, tail);
	return;
}


void jobOutputFileName(char** fileName, int jobId, char* programDirectory)
{/*name of job stdout file*/
	char* jIdChar=malloc(INT_MAX_LENGTH*sizeof(char));
	sprintf(jIdChar,"%d",jobId);
	(*fileName)=malloc((strlen(programDirectory)+strlen("/")+strlen(jIdChar)+strlen(OUTPUTFILE)+1)*sizeof(char));
	strcpy((*fileName),programDirectory);
	strcat((*fileName),"/");
	strcat(*fileName,OUTPUTFILE);
	strcat((*fileName),jIdChar);
	free(jIdChar);
	return;
}

void jobErrorFileName(char** fileName, int jobId, char* programDirectory)
{/*name of job stderr file*/
	char* jIdChar=malloc(INT_MAX_LENGTH*sizeof(char));
	sprintf(jIdChar,"%d",jobId);
	(*fileName)=malloc((strlen(programDirectory)+strlen("/")+strlen(jIdChar)+strlen(ERRORFILE)+1)*sizeof(char));
	strcpy((*fileName),programDirectory);
	strcat((*fileName),"/");
	strcat(*fileName,ERRORFILE);
	strcat((*fileName),jIdChar);
	free(jIdChar);
	return;
}


void directoryName(char* dirPath, int jobId, char**dirName)
{/*Make the name of the directory of process X*/
	/*Turn jobId into a string*/
	char* jobIdChar=malloc(INT_MAX_LENGTH*sizeof(char));
	sprintf(jobIdChar,"%d",jobId);
	/*Get your pid into a string*/
	char* pidChar=malloc(INT_MAX_LENGTH*sizeof(char));
	sprintf(pidChar,"%d",getpid());
	/*Now get the date and time name*/
	char* dateTime;
	dateConvertor(&dateTime);
	(*dirName)=malloc((strlen(dirPath)+strlen("/")+strlen("_")+strlen("_")+strlen(jobIdChar)+
		strlen(pidChar)+strlen(dateTime)+1)*sizeof(char));
	strcpy((*dirName),dirPath);
	strcat((*dirName),"/");
	strcat((*dirName),jobIdChar);
	strcat((*dirName),"_");
	strcat((*dirName),pidChar);
	strcat((*dirName),"_");
	strcat((*dirName),dateTime);
	free(jobIdChar);
	free(pidChar);
	free(dateTime);
	return;
}


void dateConvertor(char** dateTimeBasedName)
{/*Makes the date time part of the directory name*/
	time_t rawtime;
	struct tm *info;
	char buffer[80];
	time(&rawtime);
	info=localtime(&rawtime);
	/*First turn everything into a string*/
	char* yearString=malloc(INT_MAX_LENGTH*sizeof(char));
	char* monthString=malloc(INT_MAX_LENGTH*sizeof(char));
	char* mdayString=malloc(INT_MAX_LENGTH*sizeof(char));
	char* hourString=malloc(INT_MAX_LENGTH*sizeof(char));
	char* minsString=malloc(INT_MAX_LENGTH*sizeof(char));
	char* secsString=malloc(INT_MAX_LENGTH*sizeof(char));
	char* zero=malloc(INT_MAX_LENGTH*sizeof(char));

	/*Turn the integers into strings*/
	sprintf(yearString,"%d",((*info).tm_year+1900));
	sprintf(monthString,"%d",((*info).tm_mon+1));
	sprintf(mdayString,"%d",(*info).tm_mday);
	sprintf(hourString,"%d",(*info).tm_hour);
	sprintf(minsString,"%d",(*info).tm_min);
	sprintf(secsString,"%d",(*info).tm_sec);
	sprintf(zero,"%d",0);

	/*Now see where you need to put zeroes*/
	int numOfZeroes=0;
	int monthDay=FALSE;
	int month=FALSE;
	int hour=FALSE;
	int mins=FALSE;
	int secs=FALSE;

	if((*info).tm_mday<10)
	{
		++numOfZeroes;
		monthDay=TRUE;
	}
	if((*info).tm_mon<10)
	{
		++numOfZeroes;
		month=TRUE;
	}
	if((*info).tm_sec<10)
	{
		++numOfZeroes;
		secs=TRUE;
	}
	if((*info).tm_min<10)
	{
		++numOfZeroes;
		mins=TRUE;
	}
	if((*info).tm_hour<10)
	{
		++numOfZeroes;
		hour=TRUE;
	}

	/*Allocate space for the name*/
	(*dateTimeBasedName)=malloc((strlen(yearString)+strlen(monthString)+strlen(mdayString)
		+strlen(hourString)+strlen(minsString)+strlen(secsString)+strlen("_")+numOfZeroes*strlen(zero)+1)*sizeof(char));
	
	strcpy((*dateTimeBasedName),yearString);
	if(month==TRUE)
	{
		strcat((*dateTimeBasedName),zero);
	}
	strcat((*dateTimeBasedName),monthString);
	if(monthDay==TRUE)
	{
		strcat((*dateTimeBasedName),zero);
	}	
	strcat((*dateTimeBasedName),mdayString);
	strcat((*dateTimeBasedName),"_");
	if(hour==TRUE)
	{
		strcat((*dateTimeBasedName),zero);
	}
	strcat((*dateTimeBasedName),hourString);
	if(mins==TRUE)
	{
		strcat((*dateTimeBasedName),zero);
	}
	strcat((*dateTimeBasedName),minsString);
	if(secs==TRUE)
	{
		strcat((*dateTimeBasedName),zero);
	}
	strcat((*dateTimeBasedName),secsString);

	/*Clean up*/
	free(yearString);
	free(monthString);
	free(mdayString);
	free(hourString);
	free(minsString);
	free(secsString);
	free(zero);
	return;
}


void periodicalWait(pointerToSkipListNode* header, pointerToSkipListNode* tail, int *finishedJobs)
{/*In this function, a pool checks if any child has finished*/
	//printf("Pool:Came to wait for one child at most\n");
	int status;
	int finishedPid=waitpid(0,&status,WNOHANG);
	//printf("Pool:Finished pid:%d\n",finishedPid );
	if(finishedPid==0)
	{/*No child has finished*/
		//printf("Pool:No child has finished\n");
		return;
	}
	else
	{
		//printf("Pool:A child has finished\n");
		pointerToJob* finishedJob=searchByPid((*header),(*tail), finishedPid);
		if(finishedJob!=NULL)
		{
			(*finishedJob)->status=TERMINATED;
			++(*finishedJobs);
			printf("Pool:Changed child status and returning %d\n",(*finishedJobs));
		}
		return;
	}
}
