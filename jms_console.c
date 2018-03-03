
#include "jms_header.h"

/*Functions*/
int input_parsing(char* buffer, int fdw, int fdr);
void cleanup(int fd1, int fd2, char* fifo1, char* fifo2);
int consoleOutput(int fdr);
int tokenizer(char* buffer, int fdw);


int main(int argc, char** argv)
{
	/*Take input from the command line*/
	int i;
	int commandsFilePos=-1;
	int jmsInPos=-1;
	int jmsOutPos=-1;
//	printf("%d\n",argc );
	if(argc<4)
	{
		printf("No input!\nExiting . . .\n");
		exit(1);
	}
	else
	{
		for(i=0;i<argc;++i)
		{
			if(strcmp(argv[i],"-o")==0)
			{
				commandsFilePos=i+1;
				continue;
			}
			if(strcmp(argv[i],"-w")==0)
			{
				jmsInPos=i+1;
				continue;

			}
			if(strcmp(argv[i],"-r")==0)
			{
				jmsOutPos=i+1;
				continue;
			}
		}
	}
	/*Got input*/

	printf("Console\n");
	int fdr; /*File descriptor for jmsIn*/
	int fdw; /*File descriptor for jmsOut*/
	/*Make the fifo jmsIn*/
	if(mkfifo(argv[jmsInPos],FIFOPERMS)<0)
	{
		perror("Error creating fifo");
		/*Exit*/
		printf("(%d)Exiting . . .\n",getpid() );
		exit(1);
	}

	/*Make the fifo jmsOut */
	if(mkfifo(argv[jmsOutPos],FIFOPERMS)<0)
	{
		perror("Error creating fifo");
		/*Cleanup and exit*/
		cleanup(-1, -1, argv[jmsInPos], NULL);
		printf("(%d)Exiting . . .\n",getpid() );
		exit(1);
	}
	printf("Created the fifos to read and write;going to open them\n");
		/*Now open the writing end*/
	printf("Going to open the fifo %s to write\n",argv[jmsInPos] );
	fdw=open(argv[jmsInPos],O_WRONLY);
	if(fdw<0)
	{
		perror("Error opening fifo for writing");
		/*Cleanup and exit*/
		cleanup(fdr, -1, argv[jmsInPos], argv[jmsOutPos] );
		printf("(%d)Exiting . . .\n",getpid() );
		exit(1);
	}
	/*First open the reading end*/
	printf("Going to open the fifo %s to read\n",argv[jmsOutPos] );
	fdr=open(argv[jmsOutPos],O_RDONLY,O_NONBLOCK);
	if(fdr<0)
	{
		perror("Error opening fifo for reading");
		/*Cleanup and exit*/
		unlink(argv[jmsInPos]);
		unlink(argv[jmsOutPos]);
		cleanup(-1, -1, argv[jmsInPos], argv[jmsOutPos]);
		printf("(%d)Exiting . . .\n",getpid() );
		exit(1);
	}
	printf("Opened the fifos, going to get user input\n");
	FILE* fp;
	char buffer[CHARS_MAX];
	int retVal;
	switch((commandsFilePos>0))
	{
		/*There is input file;read from it*/
		case TRUE:	fp=fopen(argv[commandsFilePos],"r");
					if(fp==NULL)
					{
						perror("Cannot open input file");
					}
					else
					{/*Read from input file*/
						while(fgets(buffer,CHARS_MAX,fp)!=NULL)
						{/*Read line by line until EOF*/
							retVal=input_parsing(buffer,fdw,fdr);
						}
						fclose(fp);
						if(retVal==COMMAND9)
						{/*Shutdown*/
							break;
						}
					}
		/*Intentional lack of break statement*/
		/*Enter cli mode*/
		case FALSE:	while(retVal!=COMMAND9)
					{
						printf("Write your command>");
						fgets(buffer,CHARS_MAX,stdin);
						retVal=input_parsing(buffer,fdw,fdr);
						//printf("Parser will return %d\n",retVal );
						//return retVal;
					}
				break;
	}
	/*Cleanup and exit*/
	printf("(%d)Going to cleanup\n",getpid());
	sleep(15);/*Give coord the time to read from the fifo and handle exit*/
	cleanup(fdr,fdw,argv[jmsInPos],argv[jmsOutPos]);
	printf("(%d)Exiting . . .\n",getpid() );
	return 0;
}



int input_parsing(char* buffer, int fdw, int fdr)
{/*Processes the input accordingly, and writes the command in the fifo*/
	char instruction[20];
	int jobPid;
	int jobId;
	//char jobName[JOB_MAX_NAME];
	int time_duration;
	int existence;
	int commandId;
	/*Instructions' repertoire*/
	if(sscanf(buffer,"%s",instruction))
	{
		if(strcmp(instruction,"submit")==0)
		{
			if(sscanf(buffer,"%s",instruction)==1)
			{
				//printf("%s \n",instruction );
				//int nameLength=strlen(jobName);
				//commandId=COMMAND1;
				//write(fdw,&commandId,sizeof(int));
				//write(fdw,&nameLength,sizeof(int));
				//write(fdw,jobName,nameLength);
				tokenizer( buffer,  fdw);/*Tokenize the input for execvp*/
			}
		//	printf("PARSER WILL RETURN %d\n", COMMAND1);
			while(consoleOutput(fdr)==FALSE)
			{
			//	printf("Console:Waiting from coord\n");
			}
			//printf("Console going to tokenizer\n");
			//tokenizer( buffer,  fdw);
		//	printf("Console back from tokenizer\n");
			return COMMAND1;
		}
		else if(strcmp(instruction,"status")==0)
		{
			if(sscanf(buffer,"%s %d",instruction,&jobId)==2)
			{
				//printf("%s %d\n",instruction,jobId );
				commandId=COMMAND2;
				write(fdw,&commandId,sizeof(int));
				write(fdw,&jobId,sizeof(int));
			}
		//	printf("PARSER WILL RETURN %d\n", COMMAND2);
			while(consoleOutput(fdr)==FALSE)
			{
		//		printf("Console:Waiting from coord\n");
			}
			return COMMAND2;
		}
		else if(strcmp(instruction,"status-all")==0)
		{
			if(sscanf(buffer,"%s %d",instruction,&time_duration)==2)
			{
			//	printf("%s %d\n",instruction,time_duration );
				existence=TRUE;
				commandId=COMMAND3;
				write(fdw,&commandId,sizeof(int));
				write(fdw,&existence,sizeof(int));
				write(fdw,&time_duration,sizeof(int));
				//printf("PARSER WILL RETURN %d\n", COMMAND3);
				while(consoleOutput(fdr)==FALSE)
				{
			//		printf("Console:Waiting from coord\n");
				}
				return COMMAND3;
			}
			else if(sscanf(buffer,"%s",instruction))
			{
			//	printf("%s\n",instruction );
				existence=FALSE;
				commandId=COMMAND3;
				write(fdw,&commandId,sizeof(int));
				write(fdw,&existence,sizeof(int));
				//printf("PARSER WILL RETURN %d\n", COMMAND3);
				while(consoleOutput(fdr)==FALSE)
				{
			//		printf("Console:Waiting from coord\n");
				}
				return COMMAND3;
			}
			else
			{
				fprintf(stderr, "Command not found\n" );
			//	printf("PARSER WILL RETURN %d\n", ERROR);
				return ERROR;
			}
		}
		else if(strcmp(instruction,"show-active")==0)
		{
			printf("%s\n",instruction );
			commandId=COMMAND4;
			write(fdw,&commandId,sizeof(int));
		//	printf("PARSER WILL RETURN %d\n", COMMAND4);
			while(consoleOutput(fdr)==FALSE)
			{
		//		printf("Console:Waiting from coord\n");
			}
			return COMMAND4;
		}
		else if(strcmp(instruction,"show-pools")==0)
		{
			//printf("%s\n",instruction );
			commandId=COMMAND5;
			write(fdw,&commandId,sizeof(int));
		//	printf("PARSER WILL RETURN %d\n", COMMAND5);
			while(consoleOutput(fdr)==FALSE)
			{
		//		printf("Console:Waiting from coord\n");
			}
			return COMMAND5;
		}
		else if(strcmp(instruction,"show-finished")==0)
		{
			//printf("%s\n",instruction );
			commandId=COMMAND6;
			write(fdw,&commandId,sizeof(int));	
		//	printf("PARSER WILL RETURN %d\n", COMMAND6);	
			while(consoleOutput(fdr)==FALSE)
			{
		//		printf("Console:Waiting from coord\n");
			}	
			return COMMAND6;

		}
		else if(strcmp(instruction,"suspend")==0)
		{
			if(sscanf(buffer,"%s %d",instruction,&jobId)==2)
			{
				//printf("%s %d\n",instruction,jobId );
				commandId=COMMAND7;
				write(fdw,&commandId,sizeof(int));
				write(fdw,&jobId,sizeof(int));
		//		printf("PARSER WILL RETURN %d\n", COMMAND7);
			//	while(consoleOutput(fdr)==FALSE)
			//	{
			//		printf("Console:Waiting from coord\n");
			//	}
				printf("Sent suspend signal to jobID:%d\n",jobId);
				return COMMAND7;
			}
			else
			{
				printf("Command not found\n");
			//	printf("PARSER WILL RETURN %d\n", ERROR);
				return ERROR;
			}
		}
		else if(strcmp(instruction,"resume")==0)
		{
			if(sscanf(buffer,"%s %d",instruction,&jobId)==2)
			{
				//printf("%s %d\n",instruction,jobId );
				commandId=COMMAND8;
				write(fdw,&commandId,sizeof(int));
				write(fdw,&jobId,sizeof(int));		
		//		printf("PARSER WILL RETURN %d\n", COMMAND8);
				//while(consoleOutput(fdr)==FALSE)
				//{
				//	printf("Console:Waiting from coord\n");
				//}
				printf("Sent resume signal to jobID:%d\n",jobId );		
				return COMMAND8;
			}
			else
			{
			//	printf("Command not found\n");
			//	printf("PARSER WILL RETURN %d\n", ERROR);
				return ERROR;
			}
		}
		else if(strcmp(instruction,"shutdown")==0)
		{
			//printf("%s\n",instruction );
			commandId=COMMAND9;
			write(fdw,&commandId,sizeof(int));
		//	printf("PARSER WILL RETURN %d\n", COMMAND9);
			while(consoleOutput(fdr)==FALSE)
			{
		//		printf("Console:Waiting from coord\n");
			}
			return COMMAND9;
		}
		else if(strcmp(instruction,"exit")==0)
		{
			return EXIT;
		}
		else
		{
			fprintf(stderr, "Command not found\n" );
			return ERROR;
		}
	}
}

int tokenizer(char* buffer, int fdw)
{/*Tokenizes the submit input and sends it to the coordinator*/
//printf("Console:In tokenizer\n");
	char* position;
	char* bufferCopy=malloc((strlen(buffer)+1)*sizeof(char));
	strcpy(bufferCopy,buffer);
	position=memchr(buffer,' ',strlen(buffer));
	//printf("Console:Going to tokenize vol 1\n");
	if(position!=NULL)
	{
	//	printf("Console:Should tokenize\n");
		char* token=strtok(buffer," ");
		int i=0;
		//int ret=1;
		int arrayLength=0;
		while(token!=NULL)
		{
		//	printf("token:%s\n",token );
			//ret*=readCommand(start,token);
			if(strcmp(token,"submit")!=0)
			{
				++arrayLength;
		//		printf("Console:Now arraylength:%d\n",arrayLength );
			}
			token=strtok(NULL," ");
		}
		/*Now create the array*/
		char** execvpArray=malloc(arrayLength*sizeof(char*));
		token=strtok(bufferCopy," ");
	//	printf("Console:Going to tokenizing vol 2\n");
		int wordLength;
		int tokensSoFar=0;
		while(token!=NULL)
		{
		//	printf("token:%s\n",token);
			if(strcmp(token,"submit")!=0)
			{

				if(tokensSoFar+1==arrayLength)
				{
					wordLength=strlen(token);
		//			printf("Console:token %s length:%d\n",token ,wordLength );
					token[wordLength-1]='\0';
					execvpArray[i]=malloc(wordLength*sizeof(char));
					strcpy(execvpArray[i],token);
					++i;
				}
				else
				{	
					wordLength=strlen(token)+1;
					execvpArray[i]=malloc(wordLength*sizeof(char));
					strcpy(execvpArray[i],token);
					++i;
				}
				++tokensSoFar;
			}
			token=strtok(NULL," ");
		}
		//printf("Console:tokenized everything, going to write\n");
		/*Now write to the fifo*/
		int commandId=COMMAND1;
		write(fdw,&commandId,sizeof(int));
		write(fdw,&arrayLength,sizeof(int));
		
		for(i=0;i<arrayLength;++i)
		{
			wordLength=strlen(execvpArray[i]);
			write(fdw,&wordLength,sizeof(int));
			write(fdw,execvpArray[i],wordLength);
	//		printf("Console:wrote %d and %s\n",wordLength ,execvpArray[i]);
			free(execvpArray[i]);
		}
		free(execvpArray);
	}
	free(bufferCopy);
}

int consoleOutput(int fdr)
{/*Checks if there is incoming data from the coord*/
//printf("In console output\n");
	struct pollfd fdArray[1];
	int rc;
	int commandId;
	int jobId;
	int jobPid;
	int totalNum;
	int numOfJobs;
	long int timeActive;
	int seconds;
	jobStatus jStatus;
	int bytesRead=-1;
	int retVal=FALSE;
	/*Initiate poll parameters*/
	fdArray[0].fd=fdr;
	fdArray[0].events=POLLIN;
	/*Wait for incoming data or poll timeout*/
	rc=poll(fdArray,1,300);
	if(rc==0)
	{
//		printf("Poll timed out;will try again\n");
		;
	}
	else
	{
		if((rc==1)&&(fdArray[0].revents==POLLIN))
		{/*Read the command*/
	//		printf("Console:Going to read the instruction\n");
			read(fdr,&commandId,sizeof(int));
	//		printf("Console:Read instruction:%d\n",commandId );
			switch(commandId)
			{
				case COMMAND1:	//printf("In command 1\n");
								while(bytesRead==-1)
								{
									bytesRead=read(fdr,&jobPid,sizeof(int));
								}
								bytesRead=-1;
								/*Print on screen message*/
								printf("PID:%d\n",jobPid );
								retVal=TRUE;
				break;
				case COMMAND2:	//printf("Console:In command 2\n");
								while(bytesRead==-1)
								{
									bytesRead=read(fdr,&jobId,sizeof(int));
								}
								bytesRead=-1;
								while(bytesRead==-1)
								{
									bytesRead=read(fdr,&jStatus,sizeof(jobStatus));
								}
								bytesRead=-1;
								/*Print on screen message*/
								printf("JobID %d Status: ",jobId );
								if(jStatus==ACTIVE)
								{
									printf("Active ");
									while(bytesRead==-1)
									{
										bytesRead=read(fdr,&seconds,sizeof(int));
									}
									bytesRead=-1;
									printf("(running for %d seconds)\n",seconds );
								}
								else if(jStatus==SUSPENDED)
								{
									printf("Suspended\n");
								}
								else if(jStatus==TERMINATED)
								{
									printf("Finished\n");
								}
								retVal=TRUE;
				break;
				case COMMAND3:	//printf("Console:In command 3\n");
								while(bytesRead==-1)
								{
									bytesRead=read(fdr,&totalNum,sizeof(int));
								}
								bytesRead=-1;
								while(totalNum>0)
								{
									while(bytesRead==-1)
									{
										bytesRead=read(fdr,&jobId,sizeof(int));
									}
									bytesRead=-1;
									while(bytesRead==-1)
									{
										bytesRead=read(fdr,&jStatus,sizeof(jobStatus));
									}
									bytesRead=-1;
									/*Print on screen message*/
									printf("JobID %d Status: ",jobId );
									if(jStatus==ACTIVE)
									{
										printf("Active ");
										while(bytesRead==-1)
										{
											bytesRead=read(fdr,&seconds,sizeof(int));
										}
										bytesRead=-1;
										printf("(running for %d seconds)\n",seconds );
									}
									else if(jStatus==SUSPENDED)
									{
										printf("Suspended\n");
									}
									else if(jStatus==TERMINATED)
									{
										printf("Finished\n");
									}
									--totalNum;
								}
								retVal=TRUE;
				break;
				case COMMAND4:	//printf("Console:In command 4\n");
								printf("Active jobs:\n");
								//printf("Console:Going to read totalNum from fifo\n");
								while(bytesRead==-1)
								{
								//	printf("Console:In first while\n");
									bytesRead=read(fdr,&totalNum,sizeof(int));
								//	printf("Console:totalNum:%d and bytesRead:%d\n",totalNum,bytesRead );
								}
								bytesRead=-1;
								while(totalNum>0)
								{
									//printf("Console:In second while:%d\n",totalNum);
									while(bytesRead==-1)
									{
									//	printf("Console:Going to read the jobId\n");
										bytesRead=read(fdr,&jobId,sizeof(int));
									//	printf("Console:totalNum:%d and bytesRead:%d\n",totalNum,bytesRead );
									}
									bytesRead=-1;
									//while(bytesRead=-1)
									//{
									//	printf("Console:Going to read time active\n");
									//	bytesRead=read(fdr,&timeActive,sizeof(long int));
									//	printf("timeActive=%ld\n",timeActive );
									//}
									/*Print on screen message*/
									//printf("Console:Will print on screen\n");
									printf("JobID:%d\n",jobId );
									--totalNum;
								}
								retVal=TRUE;
				break;
				case COMMAND5:	//printf("Console:In command 5\n");
								printf("Pool & NumOfJobs:\n");
								while(bytesRead==-1)
								{
									bytesRead=read(fdr,&totalNum,sizeof(int));
								}
								bytesRead=-1;
								//printf("Console:Got number of pools:totalNum:%d\n",totalNum );
								while(totalNum>0)
								{
									while(bytesRead==-1)
									{
										bytesRead=read(fdr,&jobPid,sizeof(int));
									//	printf("Console:Read pid:%d\n",jobPid );
									}
									bytesRead=-1;
									while(bytesRead==-1)
									{
										bytesRead=read(fdr,&numOfJobs,sizeof(int));
									//	printf("Console:Read jobsServing:%d\n",numOfJobs );
									}
									/*Print on screen message*/
									bytesRead=-1;
									printf("%d %d\n",jobPid,numOfJobs );
									--totalNum;
								}
								retVal=TRUE;
				break;
				case COMMAND6:	//printf("Console:In command 6\n");
								printf("Finished jobs:\n");
								while(bytesRead==-1)
								{
									bytesRead=read(fdr,&totalNum,sizeof(int));
								}
								bytesRead=-1;
								while(totalNum>0)
								{
									while(bytesRead==-1)
									{
										bytesRead=read(fdr,&jobId,sizeof(int));
									}
									bytesRead=-1;
									/*Print on screen message*/
									printf("JobID %d\n",jobId );
									--totalNum;
								}
								retVal=TRUE;
				break;
				case COMMAND7:	;
				break;
				case COMMAND8:	;
				break;
				case COMMAND9:	printf("Console:In command 9\n");
								while(bytesRead==-1)
								{
									bytesRead=read(fdr,&totalNum,sizeof(int));
								}
								bytesRead=-1;
								printf("Served %d jobs, ",totalNum );
								while(bytesRead==-1)
								{
									bytesRead=read(fdr,&totalNum,sizeof(int));
								}
								bytesRead=-1;
								printf("%d were still in progress\n",totalNum );
								retVal=TRUE;
				break;
			}
		}
	}
	//printf("Console output will return %d\n",retVal );
	return retVal;
}

void cleanup(int fd1, int fd2, char* fifo1, char* fifo2)
{
	if(fd1>0)
	{
		close(fd1);
	}
	if(fd2>0)
	{
		close(fd2);
	}
	if(fifo1!=NULL)
	{
		unlink(fifo1);
	}
	if(fifo2!=NULL)
	{
		unlink(fifo2);
	}
}