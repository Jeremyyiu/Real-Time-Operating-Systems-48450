/*! @file
 *
 *  @brief This is a multi-threaded CPU scheduling program that uses a round robin algorithm. It also uses a named pipe (FIFO) to
 *         move data to different threads and proceeds to write the data into a file.
 *
 *  This contains the structure and "methods" for operating a multi-threaded CPU scheduling program that uses a round robin algorithm
 *  and writes the process data into a file.
 *
 *  To use this program, make sure you have src.txt and data.txt in your folder
 *  To compile this file - write in the terminal : gcc -o Prg_1 Prg_1.c -lpthread -lrt
 *  then write in the terminal: ./Prg_1 4 output.txt
 *
 *  @author Jeremy Yiu
 *  @date 2017-05-27
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>  /* required for pthreads */
#include <semaphore.h> /* required for semaphores */
#include "Prg_1.h"

/* *************************** User Instructions *******************************/
void instructions(void)
{

	printf("\n           ****************************************************************************\n");
	printf("           *************************** Program Instructions ***************************\n");
	printf("           ****************** RTOS Assignment 3a by Jeremy Yiu 11656206 ***************\n");
	printf("           *********** To compile this file - write in the terminal :- ****************\n");
	printf("           ***************** gcc -o Prg_1 Prg_1.c -lpthread -lrt **********************\n");
	printf("           ************* then write in the terminal:  ./Prg_1 4 output.txt ************\n");
	printf("           ****************************************************************************\n\n");

}
/* ************************* End of User Instructions ***************************** */




/* ************************ Main Thread  **************************** */

int main(int argc, char* argv[])
{
	FILE *fp; //file pointer
	int timeQuantum;
	int fifofd; //fifo file descriptor
	pthread_t thread1, thread2;    /* pthread defintions */
	sem_t sem_read, sem_write; /* semaphore definitions */

	instructions();	//print instructions
	remove(FIFONAME); //Ensure that the FIFO file doesn't exist when next created.

	if (argc != 3) //check if there are enough arguments placed
	{
		printf("Not enough arguments placed in command line \n");
		printf("usage: ./Prg_1 4 output.txt \n");
		return -1;
	}

	if (isPositiveNumber(argv[1]) != 0) //check if the time quantum given by user input is a positive integer
	{
		printf("Incorrect input, try again\n"); //if not, print error and close the program.
		printf("Number must be a positive integer \n");
		return (-1);
	}
	timeQuantum = atoi(argv[1]); //set timequantum to integer given by the user

	fp = fopen(argv[2], "w"); //open the file to write
	if (!fp) //if unable to open print error and exit the program
	{
		perror("Error opening file");
		return (-1);
	}

	initialiseSemaphores(&sem_write, &sem_read);  //initailise semaphores so that they can used in the threads
	/* put values into structs so that they can be passed to the threads */
	struct_thread1_info a = {&sem_write, &sem_read, timeQuantum, &fifofd};
	struct_thread2_info b = {&sem_read, &sem_write, fp, &fifofd};

	/* create new threads */
	if (pthread_create(&thread1, NULL, (void *)thread1_routine, &a) != 0 ||
	        pthread_create(&thread2, NULL, (void *)thread2_routine, &b) != 0)
	{
		perror("pthread_create"); //if unable to create threads, print error and exit program
		return (-1);
	}

	pthread_join(thread1, NULL); /* to identify if the thread-termination was completed */
	pthread_join(thread2, NULL);

	unlink(FIFONAME); //deletes name from file system
	fclose(fp); //close file
	return 0;
}


/* ************************ End of Main Thread  **************************** */






/* ************************ Methods and functions for multi-threading **************************** */

void *thread1_routine(struct_thread1_info * data)
{
	sem_wait(data->sem_write_fifo); /* wait until read pipe is available */

	int numOfProcesses, i;
	do {
		printf("Enter number of processes ");
	}
	while (getNumber(&numOfProcesses) != 0); //get number of processes - number must be a non-negative integer

	struct_process_info processes[numOfProcesses];
	initialiseProcesses(processes, numOfProcesses); //get process data - how many processes, arrival times and burst time
	printProcesses(processes, numOfProcesses, data->timeQuantum); //print process data

	/*** Round Robin ***/
	sortByArrivalTimes(processes, numOfProcesses); //sort the processes according to arrival times in ascending order
	roundRobin(processes, numOfProcesses, data->timeQuantum); //run round robin algorithm

	double avgWaitTime = averageWaitTime(processes, numOfProcesses); //calculate average wait time
	double avgTurnAroundTime = averageTurnAroundTime(processes, numOfProcesses); //calculate average turnaround time

	char string[80]; //string used to store average wait time and turnaround time

	initialiseFifo();
	sprintf(string, "Average Wait Time: %lf \nAverage Turnaround Time: %lf \n", avgWaitTime, avgTurnAroundTime);
	//can put a fuction that can either open via read only or write only and get FIFONAME from parameter and open FIFO file descriptor
	if (*data->fifofd = open(FIFONAME, O_WRONLY) < 0) //open the FIFO as write only
	{
		perror("open");
		exit(1);
	}
	write(*data->fifofd, string, strlen(string)); //write string to FIFO
	close(*data->fifofd); //close FIFO

	sem_post(data->sem_read_fifo); /* relinquish access to read sem */

}

void *thread2_routine(struct_thread2_info * data)
{
	sem_wait(data->sem_read_fifo); /* wait until read pipe is available */
	int n;
	char datafromFIFO[MSGLENGTH];

	if (*data->fifofd = open(FIFONAME,  O_RDONLY | O_NONBLOCK) < 0) //open the FIFO as read only and does not wait for write
	{
		perror("open");
		exit(1);
	}
	n = read(*data->fifofd, datafromFIFO, MSGLENGTH); //get data from FIFO

	if (n == 0) //nothing in the FIFO
		printf("FIFO is empty.\n");
	else
	{
		writeToFile(data->fp, datafromFIFO); //write data from the FIFO and write it to the file
	}
	/* close the FIFO */
	close(*data->fifofd);
	sem_post(data->sem_write_fifo); /* relinquish access to write pipe */
}

/* ************************ End of methods and functions for using multi-threading **************************** */





/* ************************ Methods and functions for setting up semaphores and FIFOs **************************** */
int initialiseSemaphores(sem_t *sem_write, sem_t *sem_read)
{
	if (sem_init(sem_read, 0, 0) == -1 || sem_init(sem_write, 0, 1) == -1) //initalise semaphores if they fail print error
	{
		printf("sem_init failed: %s\n", strerror(errno));
		return (-1);
	}
	return 0;
}

void initialiseFifo(void)
{
	int fifofd;
	if (mkfifo(FIFONAME, S_IRUSR | S_IWUSR) < 0) //create fifo file
	{
		perror("mkfifo");
		exit(1);
	}

	if (fifofd = open(FIFONAME,  O_RDONLY | O_NONBLOCK) < 0) //ensure that the fifo file is properly created and make it read only and non-blocking
	{
		perror("open");
		exit(1);
	}
	/* close the FIFO */
	close(fifofd);
}

/* ************************ End of Methods and functions for setting up semaphores and FIFOs **************************** */





/* ************************ Methods and functions for read/writing to files **************************** */
int writeToFile(FILE *f, char *buffer)
{
	fputs(buffer, f); /*put contents of the buffer into the file  */
	printf("Content written to file:-\n%s", buffer);
	return 0;
}

/* ************************ End of Methods and functions for read/writing to files **************************** */





/* ************************ Methods for handling process data **************************** */


void initialiseProcesses(struct_process_info *processes, int arraySize)
{
	int index;
	for (index = 0; index < arraySize; index++)
	{
		processes[index].processId = index + 1;
		processes[index].waitTime = 0;
		printf("Process[%d]: What is the arrive Time? ", processes[index].processId); //get arrive time of the process from the user
		getNumber(&processes[index].arriveTime);

		printf("Process[%d]: What is the burst Time? ", processes[index].processId); //get burst time from the user
		getNumber(&processes[index].burstTime);

		processes[index].remainingTime = processes[index].burstTime; //set remainingtime to the burst time
		processes[index].turnAroundTime = 0; //set turnaround time and wait time as 0 so that they are initalised.
		processes[index].waitTime = 0;
		puts("");
	}
}

void printProcesses(struct_process_info *processes, int arraySize, int timeQuantum)
{
	int i;
	printf("Details of each process:\n");
	for (i = 0; i < arraySize; i++)
		printf("Process[%d]: Arrive Time: %d, Burst time: %d, Time quantum: %d\n", processes[i].processId, processes[i].arriveTime,
		       processes[i].burstTime, timeQuantum);  //print contents of each process.
	puts("");
}

/* ************************ End methods for handling process data **************************** */





/* ************************ Methods and functions for using Round robin **************************** */
/*
 * @brief - sortByArrivalTimes - rearranges the order of the processes from lowest arrive time to highest arrive time
 *
 * Inputs: *processes - pointer to an array of processes - in which the function accesses the arrive times of each process in the array
 			arraySize - size of the array - as the array size cannot be passed down from a pointer
 *
 */
int sortByArrivalTimes(struct_process_info *processes, int arraySize) //function only receives pointer value, thats why
//the array size must be passed seperately.
{
	struct_process_info temp[arraySize], swap;
	int index1, index2;

	for (index1 = 0; index1 < arraySize - 1; index1++)
	{
		for (index2 = 0; index2 < arraySize - index1 - 1; index2++)
		{
			if (processes[index2].arriveTime > processes[index2 + 1].arriveTime)
			{
				swap = processes[index2];
				processes[index2] = processes[index2 + 1]; //bubble sorts the processes according to arrival times in ascending order
				processes[index2 + 1] = swap;
			}
		}
	}
	processes = temp;
}

void roundRobin(struct_process_info *processes, int arraySize, int timeQuantum)
{
	int timeCounter = processes[0].arriveTime; //start the time when the first process arrives
	int flag = 0;
	int index;
	int tq = 0;
	int timeLeft = 0;

	Queue *processQueue = ConstructQueue(arraySize);
	NODE *pN;
	NODE *processNode;

	while (flag < arraySize) //run loop until all the processes have been completed
	{
		for (index = 0; index < arraySize; index++) //loop
		{
			if (processes[index].arriveTime == timeCounter && processes[index].remainingTime != 0) //if the process has arrived and the remaining time
			{	//is not 0
				pN = (NODE*) malloc(sizeof (NODE));
				pN->data = &processes[index];
				Enqueue(processQueue, pN); //store the process in the queue
			}
		}

		if (isEmpty(processQueue) == 0)	//if the queue is not empty
		{
			processNode = front(processQueue); //get the first process in the queue
			if (processNode->data->remainingTime <= timeQuantum && timeLeft == 0) //if remaining time is less than or equal to
				//time quantum and is currently not running
				timeLeft = processNode->data->remainingTime;
			else if (processNode->data->remainingTime > timeQuantum && timeLeft == 0) //if remaining time is greater than time quantum
				timeLeft = timeQuantum;

			timeLeft--;
			timeCounter++;
			processNode->data->remainingTime--;

			if (processNode->data->remainingTime == 0) //if the remaining time is 0, process is completed.
			{
				processNode->data->turnAroundTime = timeCounter - processNode->data->arriveTime; //since the process is done, calculate the turnaround time
				//line below is for debugging purposes
				//printf("Process[%d]: time - %d, remaining time - %d\n", processNode->data->processId, timeCounter, processNode->data->remainingTime);
				processNode->data->waitTime = processNode->data->turnAroundTime  - processNode->data->burstTime; //since the process is done, calculate the wait time
				flag++;
				processNode = Dequeue(processQueue); //remove the process from the queue
				free(processNode); //delete the process node
			}
			else if (timeLeft == 0) //if the process finished running but not fully completed
			{
				//line below is for debugging purposes
				//printf("Process[%d]: time - %d, remaining time - %d\n", processNode->data->processId, timeCounter, processNode->data->remainingTime);
				processNode = Dequeue(processQueue); //remove the process node from the queue
				Enqueue(processQueue, processNode); //requeue the process node at the end of the queue
			}
		}
		else
			timeCounter++; //increment time
	}
	DestructQueue(processQueue); //free the memory used by the Queue struct
}

/*
 * @brief - averageWaitTime - adds all the wait times from the processes and calculates the average wait time
 *
 * Inputs: *processes - pointer to an array of processes - in which the function accesses the wait times of each process in the array
 			arraySize - size of the array - as the array size cannot be passed down from a pointer
 *
 */
double averageWaitTime(struct_process_info *processes, int arraySize)
{
	int totalWaitTime = 0, elementIndex;
	for (elementIndex = 0; elementIndex < arraySize; elementIndex++)
		totalWaitTime += processes[elementIndex].waitTime;
	return (double)totalWaitTime / arraySize;
}

/*
 * @brief - averageTurnAroundTime - adds all the turnaround times from the processes and calculates the average turnaround time
 *
 * Inputs: *processes - pointer to an array of processes - in which the function accesses the turnaround times of each process in the array
 			arraySize - size of the array - as the array size cannot be passed down from a pointer
 *
 */
double averageTurnAroundTime(struct_process_info *processes, int arraySize)
{
	int totalTurnAroundTime = 0, elementIndex;
	for (elementIndex = 0; elementIndex < arraySize; elementIndex++)
		totalTurnAroundTime += processes[elementIndex].turnAroundTime;
	return (double)totalTurnAroundTime / arraySize;
}

/* ************************ End of Methods and functions for using Round robin **************************** */




/* ************************* Methods and functions for input checking  ***************************** */
/*
 * @brief - Check if the number only contains digits and is not a negative number
 *
 * Inputs: *number - pointer to a number which is then checked by the function to see if it is only a positive number
 *
 */
int getNumber(int *number)
{
	int temp, peek;
	while (scanf("%d", &temp) != 1 || temp < 0 || (peek = getchar()) != EOF && !isspace(peek)) //check the scanf read one item
		//stops reading at whitespace
	{
		printf("Invalid input. Try again\n");
		cleanInput();
		printf("Please enter again: ");
	}
	*number = temp;
	return 0;
}

/*
 * @brief - Check if the number only contains digits and is not a negative number
 *
 * Inputs: char number[] - pointer to a number which is then checked by the function to see if it is only a positive number
 *
 */
int isPositiveNumber(char number[])
{
	int index = 0;
	//checking for negative numbers
	if (number[0] == '-') //if it contains a negative symbol,
		return 1;
	while (number[index] != 0)
	{
		if (!isdigit(number[index]))
			return 1;
		index++;
	}
	return 0;
}

/*
 * @brief clears the input buffer
*/
void cleanInput(void)
{
	int ch;
	while (getchar() != '\n' && ch != EOF); //clear the stdin
	return;
}
/* ************************* End of methods and functions for input checking ***************************** */





/* ************************* Methods and functions for Queue-Linked List implementation ***************************** */
Queue *ConstructQueue(int limit)
{
	Queue *queue = (Queue*) malloc(sizeof (Queue));
	if (queue == NULL)
		return NULL;
	if (limit <= 0) //limit cannot be less than 1, therefore set the limit to the maximum value.
		limit = 65535;

	queue->limit = limit;
	queue->size = 0;
	queue->head = NULL;
	queue->tail = NULL;

	return queue;
}


void DestructQueue(Queue *queue)
{
	NODE *pN;
	while (!isEmpty(queue)) { //if not empty, delete all notes and free the memory used by them
		pN = Dequeue(queue);
		free(pN);
	}
	free(queue); //free the memory used by the queue strucrt
}

int Enqueue(Queue *pQueue, NODE *item)
{
	if ((pQueue == NULL) || (item == NULL)) //if bad parameters given
		return 0;
	if (pQueue->size >= pQueue->limit) //if size is greater than or equal to the limit, return as there is no more space
		return 0;
	/*the queue is empty*/
	item->prev = NULL;
	if (pQueue->size == 0) //if queue is empty, sent the node as the front and rear of the queue
	{
		pQueue->head = item;
		pQueue->tail = item;

	}
	else {
		/*adding item to the end of the queue*/
		pQueue->tail->prev = item;
		pQueue->tail = item;
	}
	pQueue->size++;
	return 1;
}

NODE * Dequeue(Queue *pQueue)
{
	/*the queue is empty or bad param*/
	NODE *item;
	if (isEmpty(pQueue))
		return NULL; //if empty return NULL
	item = pQueue->head;
	pQueue->head = (pQueue->head)->prev; //remove front node and set the next node as the new front
	pQueue->size--; //reduce the size by one
	return item;
}

int isEmpty(Queue* pQueue)
{
	if (pQueue == NULL) //if queue is null then parameter is bad
		return 0;
	if (pQueue->size == 0) //if size is 0 then it is empty
		return 1;
	else
		return 0;
}

NODE * front(Queue *pQueue)
{
	NODE *item;
	if (isEmpty(pQueue)) //if the queue is empty or queue is bad.
		return NULL;
	item = pQueue->head; //return the node from the front of the queue
	return item;
}
/*************************** End of methods and functions for Queue-Linked List Implementation ****************/
