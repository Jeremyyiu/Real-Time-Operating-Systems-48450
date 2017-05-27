/*! @file
 *
 *  @brief This is a real time multi-threaded file reading/writing program that uses pipes and semaphores
 *
 *  This contains the structure and "methods" for operating a multi-threaded file/reading program
 *
 *  @author Jeremy Yiu
 *  @date 2017-04-30
 *
 */

/* To use this program, make sure you have src.txt and data.txt in your folder
   To compile this file - write in the terminal : gcc -o ass2 ass2.c -lpthread 
   then write in the terminal: ./ass2
*/ 
 
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include <pthread.h>  /* required for pthreads */
#include <semaphore.h> /* required for semaphores */

#define BUFFER_SIZE 200

#define READ_END 0 /* Read-end of the pipe */
#define WRITE_END 1 /* Write-end of the pipe */

int fd[2]; /*file descriptors to be used for the pipe*/
pthread_t threadA, threadB, threadC;    /* pthread defintions */
sem_t sem_read, sem_write, sem_justify; /* semaphore definitions */

/* ********************* Structs for each thread *********************  */

typedef struct {
    sem_t *sem_write_pipe;
    sem_t *sem_read_pipe;
    FILE *fp;
} struct_threadA_info;

typedef struct {
    sem_t *sem_read_pipe;
    sem_t *sem_shared_buffer;
    char * buffer1;
} struct_threadB_info;

typedef struct {
    sem_t *sem_shared_buffer;
    sem_t *sem_write_pipe;
    FILE *fp1;
    char * buffer1;
} struct_threadC_info;


/* ********************* Function Prototypes *********************  */
int createPipe(int * fd);
int initialiseData(void);
int introMessage(void);

int readLineFromFile(FILE *f, char *buffer);
int writeToPipe(int fd, char *buffer);

int readFromPipe(int fd, char *buf1);
int detectHeaderLine(char *buf, int *fileHeaderCheck);
int writeToFile(FILE *f, char *buffer);
void terminateAllThreads(void);
int assignment2(void);

void *threadA_routine(struct_threadA_info * data);
void *threadB_routine(struct_threadB_info * data);
void *threadC_routine(struct_threadC_info * data);

/* ***************************************************************  */

void *threadA_routine(struct_threadA_info *data)
{
	char buffer[BUFFER_SIZE];
	for(;;)
	{	
	   sem_wait(data->sem_write_pipe);	/*    wait until write pipe is available*/
		if(readLineFromFile(data->fp, buffer) == -1) /* if any end of file reached or any errors occurs
		terminate all threads */
		{
			terminateAllThreads();
		}
		createPipe(fd); /* create pipe */
		writeToPipe(fd[WRITE_END], buffer);		
		printf("Writing to Pipe: %s", buffer);
		sem_post(data->sem_read_pipe); /* relinquish access to read pipe */
	}
	return 0;	
}

void *threadB_routine(struct_threadB_info * data)
{
	for(;;)
	{
		sem_wait(data->sem_read_pipe); /* wait until read pipe is available */
		readFromPipe(fd[READ_END], data->buffer1); /*read from the pipe and place contents into a buffer */
		sem_post(data->sem_shared_buffer);	/* relinquish access to shared buffer */
	}
	return 0;
}

void *threadC_routine(struct_threadC_info *data)
{
	int fileHeaderCheck = 1;
	for(;;)
	{
		sem_wait(data->sem_shared_buffer); /* wait until access to shared buffer is available */
		if(detectHeaderLine(data->buffer1, &fileHeaderCheck) == -1)
		/* check line from buffer and discard any line from the file header region */
		{
			puts("File header detected - line discarded");
		   fileHeaderCheck = 0;
		}
		else {
			if(fileHeaderCheck == 0) /*write line to file if it is in the content region */
			{
				writeToFile(data->fp1, data->buffer1);
			}
			else
				puts("File header region detected - line discarded");
		}		
		
		puts("----------------------------------------------------------------");
		sem_post(data->sem_write_pipe); /* relinquish access to write pipe */
	}	
	return 0;
}


int main()
{
	int value;
	introMessage(); /* print student details and how to run the program */
	value = assignment2();
	return value; /* return whether the program is successful or not */
}


int assignment2(void)
{
	FILE *fp0;
	FILE *fp1;
	
	char buf1[BUFFER_SIZE];
	
	if(initialiseData() == -1) /* initialise semaphores */
		return -1;

	fp0 = fopen("data.txt","r"); /* open data.txt to read */
	if(!fp0)
	{
		perror("Error opening file");
		return(-1);
	}
	
	fp1 = fopen("src.txt","w"); /* open src.txt to write */
	if(!fp1) {
		perror("Error opening file");
		return(-1);
	}
	
	/* put values into structs so that they can be passed to the threads */
	struct_threadA_info a = {&sem_write, &sem_read, fp0};  
   struct_threadB_info b = {&sem_read, &sem_justify, buf1};
   struct_threadC_info c = {&sem_justify, &sem_write, fp1, buf1};
	
	/* create new threads */
	if(pthread_create(&threadA, NULL, (void *)threadA_routine, &a) != 0 ||
		pthread_create(&threadB, NULL, (void *)threadB_routine, &b) != 0  ||
		pthread_create(&threadC, NULL, (void *) threadC_routine, &c) != 0)		
	{
		perror("pthread_create");
		return -1;
	}

	pthread_join(threadA, NULL); /* to identify if the thread-termination was completed */
	pthread_join(threadB, NULL);
	pthread_join(threadC, NULL);
	
	/* close both data.txt files and src.txt files */
	fclose(fp0);
	fclose(fp1);
	return 0;
}

int introMessage(void)
{	
	puts("----------------------------------------------------------------");
	puts("        48450 - RTOS Assignment 2 - Jeremy Yiu 11656206");
	puts("To use this program, make sure you have src.txt and data.txt in your folder");
	puts("----------------------------------------------------------------");
}

int initialiseData(void) /* initialise semaphores - print error if unsuccessful */
{
	if(sem_init(&sem_read, 0, 0) == -1 || 	
		sem_init(&sem_write, 0, 1) == -1 ||
		sem_init(&sem_justify, 0, 0) == -1)
		{
			printf("sem_init failed: %s\n", strerror(errno));
			return -1;
		}
	return 0;
}

int readLineFromFile(FILE *f, char *buffer)
{
	if(fgets(buffer, BUFFER_SIZE, f) != NULL) /*read a single line from the file until end of file */
		return 0;
	
	if(feof(f)) {  /*if end of file reached */
		puts("                       End of file reached");
		return (-1);
	}
	else {
		perror("Error: "); /*Check for other errors */
		return (-1);
	}	
	return(0);
}

int createPipe(int * fd)
{
	if(pipe(fd) < 0) {  /* create pipe, if unsuccessful - print error */
		perror("pipe error");
		return (-1);
	}
	return 0;
}

int writeToPipe(int fd_write, char *buffer)
{
	write(fd_write, buffer, BUFFER_SIZE);	 /* write contents of the buffer into pipe */
	close(fd_write); //close write section of the pipe
	return 0;
}

int readFromPipe(int fd_read, char * buf1)
{
	read(fd_read,buf1, BUFFER_SIZE); /* read from the pipe and place into the buffer */
	printf("Reading from pipe: %s", buf1);
	close(fd_read);		//close read section of the pipe
	return 0;
}


int writeToFile(FILE *f, char *buffer)
{
	fputs(buffer,f); /*put contents of the buffer into the file  */
	printf("Content written to file: %s", buffer);
	return 0;
}

void terminateAllThreads() //cancel the operations of the threads
{
	puts("----------------------------------------------------------------");
	puts("                            Program End");
	puts("----------------------------------------------------------------");
	pthread_cancel(threadA);  //clean-up handlers and destructors are called and then the thread is terminated
	pthread_cancel(threadB);
	pthread_cancel(threadC);
}

int detectHeaderLine(char *buf, int *fileHeaderCheck) //checks if string ha
{
	if(strstr(buf, "end_header")) //check if string has "end_header"
		return -1;
	return 0;
}