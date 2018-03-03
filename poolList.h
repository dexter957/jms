#include "jms_header.h"




typedef struct poolListNode *pointerToPoolListNode;


void poolListInit(pointerToPoolListNode *listStart);
int poolListEmpty(pointerToPoolListNode *listStart);
int insertAPool(pointerToPoolListNode *listStart, pointerToPool newPool);
void deletePoolList(pointerToPoolListNode *listStart);
int listLength(pointerToPoolListNode *listStart);
pointerToPool* getLatestPool(pointerToPoolListNode *listStart);

pointerToPool getPoolFromJid(int jobId, pointerToPoolListNode* listStart);
pointerToPool* getPoolFromItsPid(int pId, pointerToPoolListNode* listStart);
void getAllFileDescriptorsW(pointerToPoolListNode* listStart, int** fds);
void getAllFileDescriptorsR(pointerToPoolListNode* listStart, int** fds);
void getAllPoolPids(pointerToPoolListNode* listStart, int** pids);
void getAllPoolJids(pointerToPoolListNode* listStart, int** jids);
int getFdRFromPid(pointerToPoolListNode* listStart, int jId);
void getAllFirstJidsFromDeadPools(pointerToPoolListNode *listStart, int** jIds);