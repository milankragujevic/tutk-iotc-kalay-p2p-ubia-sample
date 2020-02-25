#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include "queue.h"

/*构造一个空队列*/
Queue *InitQueue()
{
	Queue *pqueue = (Queue *)malloc(sizeof(Queue));
	if(pqueue != NULL)
	{
		pqueue->front = NULL;
		pqueue->rear = NULL;
		pqueue->size = 0;
	}
	return pqueue;
}

/*判断队列是否为空*/
int IsEmpty(Queue *pqueue)
{
	if(pqueue->front==NULL && pqueue->rear==NULL && pqueue->size==0){
		return 1;
	}else{
		return 0;
	}
}

/*清空一个队列*/
void ClearQueue(Queue *pqueue)
{
	while(IsEmpty(pqueue)!=1){
		DeQueue(pqueue,NULL);
	}
}

/*销毁一个队列*/
void DestroyQueue(Queue *pqueue)
{
	if(IsEmpty(pqueue)!=1){
		ClearQueue(pqueue);
	}
	free(pqueue);
}

/*返回队列大小*/
int GetSize(Queue *pqueue)
{
	return pqueue->size;
}

/*返回队头元素*/
PNode GetFront(Queue *pqueue, Item *pitem)
{
	if(IsEmpty(pqueue)!=1 && pitem!=NULL){
		*pitem = pqueue->front->data;
	}else{
		return -1;
	}
	return pqueue->front;
}

/*返回队尾元素*/
PNode GetRear(Queue *pqueue, Item *pitem)
{
	if(IsEmpty(pqueue)!=1 && pitem!=NULL){
		*pitem = pqueue->rear->data;
	}else{
		return -1;
	}
	return pqueue->rear;
}

/*新元素入队*/
PNode EnQueue(Queue *pqueue, Item item)
{
	PNode pnode = (PNode)malloc(sizeof(Node));
	if(pnode!=NULL)
	{
		pnode->data = item;
		pnode->next = NULL;

		if(IsEmpty(pqueue)==1)
		{
			pqueue->front = pnode;
		}
		else
		{
			pqueue->rear->next = pnode;
		}
		pqueue->rear = pnode;
		pqueue->size++;
	}
	else
	{
		return -1;
	}
	return pnode;
}

/*队头元素出列*/
PNode DeQueue(Queue *pqueue,Item *pitem)
{
	PNode pnode = pqueue->front;
	if(IsEmpty(pqueue)!=1 && pnode!=NULL)
	{
		if(pitem != NULL){
			*pitem = pnode->data;
		}
		pqueue->size--;
		pqueue->front = pnode->next;
		free(pnode);
		if(pqueue->size==0){
			pqueue->rear = NULL;
		}
	}
	else
	{
		return -1;
	}
	return pqueue->front;
}

/*遍历队列并对各数据项调用visit函数*/
//void QueueTraverse(Queue *pqueue,void (*visit)())
//{
//    PNode pnode = pqueue->front;
//    int i = pqueue->size;
//    while(i--)
//    {
//        visit(pnode->data);
//        pnode = pnode->next;
//    }
//}
