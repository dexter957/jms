#include "jms_header.h"


#define MAX_FORWARD 30 /*Forward pointers for the skip list*/
#define MAX_VALUE 1000



typedef struct skipListNode *pointerToSkipListNode;

int createSkipList(pointerToSkipListNode *first, pointerToSkipListNode *last);
int insertInSkipList(pointerToSkipListNode *first, pointerToSkipListNode *last, int jobId, pointerToJob newJob);
pointerToJob search(int searchKey, pointerToSkipListNode first);
pointerToJob* searchDP(int searchKey, pointerToSkipListNode first);
void destroySkipList(pointerToSkipListNode first, pointerToSkipListNode last);
void printSkipList(pointerToSkipListNode first);
int skipListLength(pointerToSkipListNode first, pointerToSkipListNode last);


void suspendJob(pointerToSkipListNode *first, int searchKey );
void activateJob(pointerToSkipListNode *first, int searchKey );
void terminateJob(pointerToSkipListNode *first, int searchKey );
int countActiveJobs(pointerToSkipListNode *first);
int countSuspendedJobs(pointerToSkipListNode *first);
int countTerminatedJobs(pointerToSkipListNode *first);
void getActiveJids(pointerToSkipListNode *first, int** jids);
void getActiveTimes(pointerToSkipListNode *first, long int **times);
void getActiveTimesStart(pointerToSkipListNode *first, long int **timesStart);
void getAllTimesSubmitted(pointerToSkipListNode* first,long int** timesSubmitted);
void getTerminatedJids(pointerToSkipListNode *first, int **jids);
void setReactivationTime(pointerToSkipListNode *first, int searchKey, long int saidTime);
void getItAll(pointerToSkipListNode first, int** jobIds, int** procIds, jobStatus** statuses);
pointerToJob* searchByPid(pointerToSkipListNode first,pointerToSkipListNode last, int pid);