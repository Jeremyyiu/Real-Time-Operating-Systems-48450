/*! @file
 *
 *  @brief This is a page replacement program for virtual memory mangement using FIFO algorithm.
 *
 *  This contains the structure and "methods" for operating a page replacement program for virtual memory mangement using FIFO algorithm.
 *
 *  To use this program, make sure you have src.txt and data.txt in your folder
 *  To compile this file - write in the terminal : gcc -o Prg_2 Prg_2.c -lpthread -lrt
 *  then write in the terminal: ./Prg_2 4 output.txt
 *
 *  @author Jeremy Yiu
 *  @date 2017-05-27
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

/* *************************************************************************** */
#define REFERENCESTRINGSIZE 24
const char *REFERENCE_STRING = "7 0 1 2 0 3 0 4 2 3 0 3 0 3 2 1 2 0 1 7 0 1 7 5";
/* *************************************************************************** */
struct LinkedList
{
	char data;
	struct LinkedList *next;
};

typedef struct LinkedList *node;

typedef struct {
    sem_t *sem_pageReplacement;
    sem_t *sem_signalHandler;
    int *count;
    int *arr;
    int frameSize;
    int *faults;
} struct_thread1_info;


typedef struct {
    sem_t *sem_signalHandler;
    sem_t *sem_pageReplacement;
    int *faults;
} struct_thread2_info;

//thread 1 functions and methods
void createFrameList(int frameSize);
void signal_handler(int sig);
int frameSearch(int number);
void fifo(int count, int *arr, int *faults);
node nodeCreate(void);
void printFrame(int arrElement);

void readRefString(int *count, int *arr);
int isNumber(char number[]);
void instructions(void);

//thread 2 methods
void sighandler(int sig);

//Semaphore Handling
int initialiseSemaphores(sem_t *sem_pageReplacement, sem_t *sem_signalHandler);

//pthread routines
void *thread1_routine(struct_thread1_info * data);
void *thread2_routine(struct_thread2_info * data);

node start;
volatile sig_atomic_t flag = 0; //an integer type which can be accessed as an
//atomic entity even in the presence of asynchronous interrupts made by signals
//value of the variable may change at any time - without any action being 
//taken by the code the compiler finds nearby

/* *************************** User Instructions *******************************/
void instructions(void)
{
	
 	printf("\n           ****************************************************************************\n");
 	printf("           *************************** Program Instructions ***************************\n");
 	printf("           ****************** RTOS Assignment 3b by Jeremy Yiu 11656206 ***************\n");
 	printf("           *********** To compile this file - write in the terminal :- ****************\n");
    printf("           ***************** gcc -o Prg_2 Prg_2.c -lpthread -lrt **********************\n");
    printf("           ************* then write in the terminal:  ./Prg_2 4 ***********************\n");
 	printf("           ****************************************************************************\n\n");
 	
}
/* ************************* End of User Instructions ***************************** */


void sighandler(int sig){
  	flag = 1; // set flag so that the program knows when to stop the while loop - which wait for the ctrl+c signal
}

node nodeCreate(void)
{
	node newNode;
	newNode = malloc(sizeof(node)); //allocate memory
	newNode->data = -1;     // -1 means that the data is currently empty
	newNode->next = NULL;    //indicates the next node as NULL as default
	return newNode;
}

void createFrameList(int frameSize)
{
	int index;
	node current, new_node;
	for(index = 0; index < frameSize; index++) //createFrame
	{
		new_node = nodeCreate();
		if(start == NULL)  //if start is empty then set the start as the new node and current to new node
			start = current = new_node;
		else
			current = current->next = new_node; //Set the pointer after the current as the new node and set the current to that. 
	}
	current->next = start; //when current is at the end, point to the start. */
}


int frameSearch(int number)
{
	node temp_node; //check if the number is within the frame
	temp_node = start; //starts checking from the start of the frame
	do
	{
		if(temp_node->data == number) //if the number in the parameter is the same as the number in the frame then return true
			return(1);
		temp_node = temp_node->next; //otherwise check the next frame
	}while(temp_node!=start); //check until the frame loops back to the start again
	return(0);
}

void printFrame(int arrElement)
{
	node temp;
	temp = start;
	printf("\n      %d       |   ",arrElement); //print an element of the reference string
	printf("|");
	do
	{
		if(temp->data == -1) //if the frame number is null
			printf(" - |");
		else
			printf(" %d |",temp->data); //otherwise print the frame number
		temp=temp->next;
	}while(temp!=start); //run until the temp node loops and meets the start node 
}

void fifo(int count, int *arr, int *faults)
{
	int index;
	node temp, last, curr;
	temp = last = curr = start;

	printf("--------------------------------------");
	printf("\n  Ref String  |    Page frames\n");
	printf("--------------------------------------");
	for(index = 0;index < count; index++) //check each number of the string
	{
		if(!frameSearch(arr[index])) //means that the number of the string cannot be found in the frame
		{
			*faults += 1; //increment the number of faults by 1. 
			if(curr->data == -1) //if the element in the frame is empty or NULL
			{
				curr->data = arr[index]; //place the number into the frame
				curr = curr->next; //move to the next frame that can be replaced
			}
			else
			{
				last->data = arr[index]; //replace the oldest frame with a new one
				last = last->next; //move to the new oldest frame
			}
			printFrame(arr[index]); //print frame contents
			printf("    Page fault Number: %d", *faults);	//when there's a page fault, display the page fault number as well
		}
		else
			printFrame(arr[index]); //print frame contents
	}
}

void readRefString(int *count, int *arr)
{
	int index, number = 0, cnt = 0, refStringPos;

	for (index = 0; index  < REFERENCESTRINGSIZE; index++) 
	{
	  cnt = sscanf(REFERENCE_STRING, "%d %n", &arr[index], &refStringPos); //get each number in the reference string and get the position in the string
	  if (cnt != 1) break;
	  else
	  	REFERENCE_STRING += refStringPos; //the string to the new position so that the program knows where to look next loop
	  number++;
	}
	*count = number; //set the count to the size of the reference string
}

int isNumber(char number[]) //checks if the number is a positive integer
{
	int numIndex = 0;
    //checking for negative numbers
    if (number[0] == '-')
        return 1;
    while(number[numIndex] != 0) //if greater or equal than one digit
    {
        if (!isdigit(number[numIndex])) //check if it is number
            return 1; //if not return false
      numIndex++;
    }
    return 0;
}


void *thread1_routine(struct_thread1_info * data)
{
	sem_wait(data->sem_pageReplacement); //wait for page replacement sem

	readRefString(data->count, data->arr); //read the reference string
	createFrameList(data->frameSize); //create the frame with NULL values
	fifo(*data->count, data->arr, data->faults); //run the FIFO
	puts("");

	sem_post(data->sem_signalHandler); /* relinquish access to signalhandler sem */
}

void *thread2_routine(struct_thread2_info * data)
{
	sem_wait(data->sem_signalHandler); //waits for access to signal handler
	printf("\n\nAwaiting ctrl+c signal to print total number of page faults...\n");

	//waits until ctrl+c is pressed before executing the function
	/* ***************************************** */
	if(signal(SIGINT, sighandler) == SIG_ERR) //waits until ctrl+c signal is received - if SIG_ERR then print error
		printf("\n cant catch SIGINT \n");
	sleep(1); //to put less strain on the program - sleep for one second
	while(flag != 1);

	/* ***************************************** */
	printf("\nSignal Received \n");
	printf("\n------------------------------------------------------------\n");
	printf("             Total Number of Page faults: %d",*data->faults);
	printf("\n------------------------------------------------------------\n");
	sem_post(data->sem_pageReplacement); /* relinquish access to page replacement sem */

}

int initialiseSemaphores(sem_t *sem_pageReplacement, sem_t *sem_signalHandler)
{
	if(sem_init(sem_pageReplacement,0, 1) == -1 || sem_init(sem_signalHandler, 0, 0) == -1)
		{
			printf("sem_init failed: %s\n", strerror(errno));
			return (-1);
		}
	return 0;
}

int main(int argc, char* argv [])
{
	int count = 0, frameSize;
	int faults = 0;		
	int arr[REFERENCESTRINGSIZE];

	sem_t sem_pageReplacement, sem_signalHandler; /* semaphore definitions */
	pthread_t thread1, thread2;    /* pthread defintions */

	instructions();

	if(argc != 2)
	{
		printf("Incorrect number of arguments placed in command line.\n");
		printf("Please place only one integer to the command line when executing.\n");
		return -1;
	}
	if(isNumber(argv[1]) != 0)
	{
		printf("Incorrect input, try again\n");
		return(-1); 	
	}
	frameSize = atoi(argv[1]); //sets frame size to number given in command line
	

	initialiseSemaphores(&sem_pageReplacement, &sem_signalHandler); //initailise semaphores so that they can used in the threads

	/* put values into structs so that they can be passed to the threads */
	struct_thread1_info a = {&sem_pageReplacement, &sem_signalHandler, &count, arr, frameSize, &faults};  
    struct_thread2_info b = {&sem_signalHandler, &sem_pageReplacement, &faults};

    //creates the pthreads - if not 0 then print error and exit program
	if(pthread_create(&thread1, NULL, (void *)thread1_routine, &a) != 0 || 
		pthread_create(&thread2, NULL, (void *)thread2_routine, &b) != 0)	
	{
		perror("pthread_create");
		return (-1);
	}

    pthread_join(thread1, NULL); /* to identify if the thread-termination was completed */
    pthread_join(thread2, NULL); //add error checking

  return 0;
}
