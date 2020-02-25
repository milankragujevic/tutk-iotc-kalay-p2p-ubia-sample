#ifndef QUEUE_H_
#define QUEUE_H_

typedef float Item;
typedef struct node * PNode;
typedef struct node
{
	Item data;
	PNode next;
}Node;

typedef struct
{
	PNode front;
	PNode rear;
	int size;
}Queue;

Queue *InitQueue();
int IsEmpty(Queue *pqueue);
void ClearQueue(Queue *pqueue);
void DestroyQueue(Queue *pqueue);
int GetSize(Queue *pqueue);
PNode GetFront(Queue *pqueue, Item *pitem);
PNode GetRear(Queue *pqueue, Item *pitem);
PNode EnQueue(Queue *pqueue, Item data);
PNode DeQueue(Queue *pqueue,Item *pitem);

#endif
