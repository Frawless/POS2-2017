/************************************************
*	 Projekt: 	Projekt do předmětu POS		    * 
* 				Shell						    *
*	Autoři:	Bc. Jakub Stejskal <xstejs24>	    *
*	Nazev souboru: proj02.c						*
*			Datum:  25. 4. 2017					*
*			Verze:	1.0		                    *
************************************************/

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <fcntl.h>

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#if _POSIX_VERSION >= 200112L
	#include <pthread.h>
#include <netinet/ip.h>
#else
	#error "POSIX threads are not available"
#endif

#define BUFFER_SIZE 513
#define LINE_SIZE 512
#define PROMPT "prompt>"
#define BACKGROUND "&"
#define REDIRECT_LEFT "<"
#define REDIRECT_RIGHT ">"

bool exitCommand = false;
bool executing = false;
bool background = false;
bool redirect_left = false;
bool redirect_right = false;
// BUffer with input command
char inputCmd[BUFFER_SIZE];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;	// Mutex create
pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;	// cond create

// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
// Source: http://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
char *trimwhitespace(char *str)
{
	char *end;

	// Trim leading space
	while(isspace((unsigned char)*str)) str++;

	if(*str == 0)  // All spaces?
	  return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return str;
}

void parseInputCmd(char **argv)
{
	char *token;
	
	token = strtok(inputCmd," ");
	
	while(token != NULL)
	{
		if(token == " ")
		
		if(strcmp(token,REDIRECT_LEFT) == 0)
			redirect_left = true;
		if(strcmp(token,REDIRECT_RIGHT) == 0)
			redirect_right = true;
		if(strcmp(token,BACKGROUND) == 0)
			background = true;
		
		*argv++ = token;
		
		printf( "Token: %s\n", token );
	    token = strtok(NULL, " ");
	}
		
	*argv = '\0';
}


void *executeCommand()
{
	char *argv[64];
	while(!exitCommand)
	{
		pthread_mutex_lock(&mutex);
		// Command execute
		
		if(!executing){
			pthread_cond_wait(&cond,&mutex);	
		}
	
		if(strcmp((const char*)&inputCmd,"exit\n") == 0)
			exitCommand = true;
		else{
			// Parse cmd
			printf(PROMPT);
//			printf("Haha!\n");
			trimwhitespace(inputCmd);
			parseInputCmd(argv);
			printf("Test: %s\n",argv[0]);
			fflush(stdout);
		}
	
		executing = false;
		
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
	}
	
	return NULL;
}




void *runShell()
{
	bool tooLongCommand = false;
	int inputSize = 0;
	
	while(!exitCommand)
	{
		// Lock Mutex
		pthread_mutex_lock(&mutex);
		memset(inputCmd,'\0', BUFFER_SIZE);
		
		if(!tooLongCommand)
		{
			// prompt print
			printf(PROMPT);
			fflush(stdout);			
		}
		
		inputSize = read(STDIN_FILENO, &inputCmd, BUFFER_SIZE);
		
		// read from stdin
		if(inputSize > LINE_SIZE)
		{
			tooLongCommand = true;
			printf(PROMPT);
			printf("Input is too long!\n");
			fflush(stdout);
			continue;
//			exit(0);
		}
		else if(tooLongCommand)
		{
			memset(inputCmd,'\0', BUFFER_SIZE);
			tooLongCommand = false;
			continue;
		}
		else
		{
			executing = true;
			pthread_cond_signal(&cond);
			
			if(executing)
				pthread_cond_wait(&cond,&mutex);	
		}
		pthread_mutex_unlock(&mutex);
	}
	
	return NULL;
}


/*
 * 
 */
int main(int argc, char** argv) {

	// Threads
	pthread_t readThread;
	pthread_t executeThread;
	// Attributes
	pthread_attr_t attr;
	
	// Results of create and join
	void *resultRead;
	void *resultExec; 
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	
	// Create threads
	pthread_create(&readThread, &attr, runShell, NULL);
	pthread_create(&executeThread, &attr, executeCommand, NULL);
	
	// Join threads
	pthread_join(readThread, &resultRead);
	pthread_join(executeThread, &resultExec);
	
	pthread_attr_destroy(&attr);
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	
	
	return (EXIT_SUCCESS);
}

