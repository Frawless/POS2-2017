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
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>

#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>

#define _POSIX_C_SOURCE 200809L
#if _POSIX_VERSION >= 200112L
	#include <pthread.h>
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
bool redInput = false;
bool redOutput = false;
// Save PID
pid_t foreID;
pid_t backID;
// BUffer with input command
char inputCmd[BUFFER_SIZE];

// Input/Ouput file from redirect options
char *inputFile;
char *outputFile;

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


void handler()
{
	if(foreID > 0){
		printf("Killing process [%d]\n",foreID);
		kill(foreID,SIGINT);
		foreID = 0;
	}
}

void childHandler()
{
	int status;
	pid_t child = waitpid(-1, &status, WNOHANG);
	
	if(child < 0)
		return;
	
	// http://pubs.opengroup.org/onlinepubs/9699919799/functions/wait.html
	if(WIFEXITED(status))
		printf("Proces [%d] terminated normally with exit code %d\n",child, WEXITSTATUS(status));
	else if(WIFSIGNALED(status))
		printf("Proces [%d] terminated with exit code %d\n",child, WTERMSIG(status));
	else if(WIFSTOPPED(status))
		printf("Proces [%d] was stopped with exit code %d\n",child, WIFSTOPPED(status));
	else
		printf("There is no proces [%d] running\n",child);
	
	printf(PROMPT);
	fflush(stdout);
	
	backID = 0;
}

void createRedirects(char *token)
{
	if(redInput && inputFile == '\0')
		inputFile = token;

	if(redOutput && outputFile == '\0')
		outputFile = token;
}

void cleanAfterExecute()
{
	inputFile = '\0';
	outputFile = '\0';
	background = false;
	redInput = false;
	redOutput = false;
}


void parseInputCmd(char **argv)
{
	char *token;
	token = strtok(inputCmd," ");
	// Ctrl+D react
	if(token == '\0'){
		printf("\n");
		exit(EXIT_SUCCESS);
	}
	// Creating arguments
	while(token != NULL)
	{	
		// Create redirect in and out
		createRedirects(token);
		
		if(strcmp(token,REDIRECT_LEFT) == 0)
			redInput = true;
		if(strcmp(token,REDIRECT_RIGHT) == 0)
			redOutput = true;
		if(strcmp(token,BACKGROUND) == 0)
			background = true;
		// Parsing cmd as program with parameters
		if(!(redInput || redOutput || background) && strcmp(token,"\n") != 0)
			*argv++ = token;
		
	    token = strtok(NULL, " ");
	}
	// End of line
	*argv = '\0';
}

// http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
void run(char** argv)
{
	int output, input;
	pid_t  pid;

	if ((pid = fork()) < 0) {     /* fork a child process           */
			fprintf(stderr,"ERROR: forking child process failed\n");
			fflush(stderr);
			exit(1);
	}
	else if (pid == 0) {          /* for the child process:         */
		// Redirect >
		if(outputFile != '\0')
		{
			output = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			close(STDOUT_FILENO);
			dup(output);
		}
		// Redirect <
		if(inputFile != '\0')
		{
			input = open(inputFile, O_RDONLY);
			close(STDIN_FILENO);
			dup(input);
		}
		// Background save pid
		if(background)
		{
			setpgid(pid,pid);
		}
		// Execute
		if (execvp(*argv, argv) < 0) {
			   printf("No such program with name \'%s\' in Unix system!\n",*argv);
			   fflush(stdout);
			   exit(1);
		}
		// Clearing
		if(outputFile != '\0')
			close(output);
		if(inputFile != '\0')
			close(input);		
		
		exit(EXIT_SUCCESS);
		
	}
	else {                              
		if(!background)
		{
			foreID = pid;	// save PID of foreground process
			int status;
			while (wait(&status) != pid);
		}
		else
		{
			backID = pid;	// save PID of background process
			printf("Started \'%s\': [%d]\n",argv[0],pid);
			fflush(stdout);
		}

	}	
}


void *executeCommand()
{
	char *argv[512];
	while(!exitCommand)
	{
		pthread_mutex_lock(&mutex);
		// Command execute
		if(!executing){
			pthread_cond_wait(&cond,&mutex);	
		}
		// Parsing exit cmd
		if(strcmp((const char*)&inputCmd,"exit\n") == 0)
			exitCommand = true;
		else{
			// Parse cmd
			trimwhitespace(inputCmd);
			parseInputCmd(argv);
			run(argv);
			cleanAfterExecute();
		}
		executing = false;
		// Signal for read next input
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
		}
		// Too long command prase
		else if(tooLongCommand)
		{
			memset(inputCmd,'\0', BUFFER_SIZE);
			tooLongCommand = false;
			continue;
		}
		else
		{
			// Signal for executing
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
int main(void) {

	// Threads
	pthread_t readThread;
	pthread_t executeThread;
	// Attributes
	pthread_attr_t attr;
	
	// Results of create and join
	void *resultRead;
	void *resultExec; 
	
	struct sigaction sig_child, sig_int;
	// CHild handler
	sig_child.sa_handler = childHandler;
	sig_child.sa_flags = 0;
	sigemptyset(&sig_child.sa_mask);
	sigaction(SIGCHLD,&sig_child,NULL);
	// Ctrl+c handler for foreground process
	sig_int.sa_handler = handler;
	sig_int.sa_flags = 0;
	sigemptyset(&sig_int.sa_mask);
	sigaction(SIGINT,&sig_int,NULL);
	
	// Thread init
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	
	// Create threads
	pthread_create(&readThread, &attr, runShell, NULL);
	pthread_create(&executeThread, &attr, executeCommand, NULL);
	
	// Join threads
	pthread_join(readThread, &resultRead);
	pthread_join(executeThread, &resultExec);
	// Cleaning
	pthread_attr_destroy(&attr);
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	
	
	return (EXIT_SUCCESS);
}

