#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define COMMAND 100
#define TOCOORD "./toCoordFifo"
#define FROMCOORD "./fromCoodFifo"
#define COORDPATH "./coord"
#define COORDEXEC "coord"
#define POOLPATH "./pool"
#define POOLEXEC "pool"
#define FEXTE ".fifo"

/*Output and error files base names*/
#define OUTPUTFILE "stdout_"
#define ERRORFILE  "stderr_"

/*Pools' fifos*/
#define FROMPOOL 	"./fromPool_"
#define TOPOOL 		"./toPool_"

#define FIFOPERMS 0666
#define TRUE 1
#define FALSE 0
#define CHARS_MAX 200
#define JOB_MAX_NAME 100
#define INT_MAX_LENGTH 50
#define	PERMS 0777

/*Commands' repertoire*/
#define COMMAND1 11
#define COMMAND2 12
#define COMMAND3 13
#define COMMAND4 14
#define COMMAND5 15
#define COMMAND6 16
#define COMMAND7 17
#define COMMAND8 18
#define COMMAND9 19
#define YESEXIT	 20
#define EXIT 	 10
#define ERROR 	 -1


typedef enum {ACTIVE,SUSPENDED,TERMINATED} jobStatus;

struct job
{
	int jPid; /*Job's pid*/
	int jId;  /*Job's id*/
	char* name; /*Job's name (for human convenience)*/
	jobStatus status; /*Job's status*/
	long int lastStartTime; /*Last activation time*/
	long int totalActiveTime; /*Total active time*/
	long int timeSubmitted;	 /*When was this job submitted*/
};

typedef struct job *pointerToJob;

struct pool
{
	int poolId; /*Logical id of a pool*/
	int poolPid; /*Process id of a pool*/
	int jobsServing; /*How many jobs the pool is handling*/
	int full; /*If the pool has reached max number of jobs*/
	int alive; /*If the pool is alive or has finished (as process)*/
	char* poolFifoRead; /*The name for the pool to read commands from jms_coord*/
	char* poolFifoWrite; /*The name for the pool fifo to write to jms_coord*/
	int fdr; /*File desciptor for writing(coord READS from this fd)*/
	int fdw; /*File desciptor for reading(coord WRITES in this fd)*/
	int* jobIds; /*An array with the job Ids this pool is serving*/
};

typedef struct pool *pointerToPool;