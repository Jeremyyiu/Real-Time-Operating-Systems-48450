#define FIFONAME "/tmp/fifo_demo"
#define MSGLENGTH 100


typedef struct {
	int processId;
	int arriveTime;
	int burstTime;
	int waitTime;
	int remainingTime;
	int turnAroundTime;
} struct_process_info;

typedef struct Node_t {
	struct_process_info *data;
	struct Node_t *prev;
} NODE;

/* the HEAD of the Queue, hold the amount of node's that are in the queue*/
typedef struct Queue {
	NODE *head;
	NODE *tail;
	int size;
	int limit;
} Queue;

Queue *ConstructQueue(int limit);
void DestructQueue(Queue *queue);
int Enqueue(Queue *pQueue, NODE *item);
NODE *Dequeue(Queue *pQueue);
int isEmpty(Queue* pQueue);
NODE * front(Queue *pQueue);

typedef struct {
	sem_t *sem_write_fifo;
	sem_t *sem_read_fifo;
	int timeQuantum;
	int *fifofd;
} struct_thread1_info;

typedef struct {
	sem_t *sem_read_fifo;
	sem_t *sem_write_fifo;
	FILE *fp;
	int *fifofd;
} struct_thread2_info;

void roundRobin(struct_process_info *processes, int arraySize, int timeQuantum);
int writeDataToFile(FILE *f, char *buffer);
int getNumber(int *number);
void cleanInput(void);
void instructions(void);
double averageWaitTime(struct_process_info *processes, int arraySize);
double averageTurnAroundTime(struct_process_info *processes, int arraySize);
int sortByArrivalTimes(struct_process_info *processes, int arraySize);
void initialiseProcesses(struct_process_info *processes, int arraySize);
void initialiseFifo(void);
int writeToFile(FILE *f, char *buffer);
int isPositiveNumber(char number[]);
int initialiseSemaphores(sem_t *sem_write, sem_t *sem_read);
void unLinkAllSemaphores(void);
void printProcesses(struct_process_info *processes, int arraySize, int timeQuantum);

void *thread1_routine(struct_thread1_info * data);
void *thread2_routine(struct_thread2_info * data);
