/* 
 * Author: Thomas Buteau
 * Program: Smallsh
 * Class: CS344-400
 * Date: 8-7-17
 * Description: smallsh is a shell for UNIX that has built in functions: exit, cd, and status. Exit terminates
 * all child processes and closes the program. cd will change the working directory. status displays the exit
 * result of the last foreground process. smallsh will parse user input and call either one of the predefined
 * functions or call UNIX commands. It can handle arguments separated by spaces for these functions. Additionally,
 * stding and stdout can be redirected by using the '<' and '>' characters. Likewise, the '&' character at the end
 * of the user input will result in the process being run in the background.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

int foreGround = 0; //Global variables cause great shame upon my house but I can't figure out how to
		   //handle the ctrl+Z requirement for this assignment otherwise

/*
 * The foregroundOnly function is called when a SIGTSTP signal is caught. It either enables or
 * disables the '&' input causing a process to be run in the background and displays a message
 * reflecting this.
 */
void foregroundOnly(int sig) {
	if(foreGround == 0) {
		foreGround = 1;
		char* disable = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, disable, 49);
	}
	else {
		foreGround = 0;
		char* enable = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, enable, 29);
	}
}

/*
 * The exit function kills any child processes or jobs and then terminates the program
 */
void exitSH() {
	kill(0, SIGTERM); //sends kill signal to all processes with the same process group ID
	exit(0);
}

/*
 * The cd function changes the working directory. If the argument provided is NULL then
 * it changes to the directory specified in the HOME environment variable. Otherwise, 
 * it will change to the path of the argument. Note that the path provided can be 
 * relative or absolute.
 */
void cd(char* arg) {
	if(arg == NULL) {
		chdir(getenv("HOME"));
	}
	else if(1) {
		chdir(arg);
	}
}

/*
 * The status function prints out the exit status or terminating signal of the last 
 * foreground process.
 */
void status(int childExitMethod) {
	if(WIFEXITED(childExitMethod) != 0) {
		int exitStatus = WEXITSTATUS(childExitMethod);
		printf("exit value %d\n", exitStatus);
		fflush(stdout);
	}
	else if(WIFSIGNALED(childExitMethod) != 0) {
		int termSignal = WTERMSIG(childExitMethod);
		printf("terminated by signal %d\n", termSignal);
		fflush(stdout);
	}
}


/*
 * The parseInput function takes the input string and separates it by " " and puts
 * each word into the parsedArray array.
 */
int parseInput(char *input, char** parsedArray, int wordCount, int* background) {
	char* tempInp = malloc(sizeof(char)*2048);
	memset(tempInp, '\0', sizeof(tempInp));
	int location = -1;
	int i;
	for(i=0;i<2047;i++) {
		//check for $$ in code and set location to that spot in the array
		if(input[i] == '$' && input[i+1] == '$') {
			location = i;
			break;
		}
	}
	
	//$$ was found, make new string with user input but have PID where $$ was
	if(location != -1) {		
		tempInp = strtok(input, "$\n");
		char* pidTemp = malloc(sizeof(char)*2048);
		memset(pidTemp, '\0', sizeof(pidTemp));
		snprintf(pidTemp,2048,"%d", (int)getpid());
		strcat(tempInp, pidTemp);
		free(pidTemp);
	}
	
	//$$ was not found, parse words normally
	if(location == -1) {
		memset(parsedArray, '\0', sizeof(parsedArray));
		wordCount = 0;
		parsedArray[wordCount] = strtok(input, " \n");
	
		while(parsedArray[wordCount] != NULL) {
			wordCount++;
			parsedArray[wordCount] = strtok(NULL, " \n");
		}
		if(wordCount > 0) {
			if(strcmp(parsedArray[wordCount -1], "&") == 0 && foreGround == 0) {
				*background = 1;
			}		
		}
		if(parsedArray[0] == NULL) parsedArray[0] = "#";
		return (wordCount-1);
	}
	
	//$$ was found and replaced with PID, parse words normally
	if(location != -1) {
                memset(parsedArray, '\0', sizeof(parsedArray));
                wordCount = 0;
                parsedArray[wordCount] = strtok(tempInp, " \n");
                while(parsedArray[wordCount] != NULL) {
                        wordCount++;
                        parsedArray[wordCount] = strtok(NULL, " \n");
                }
		if(wordCount > 0) {
			if(strcmp(parsedArray[wordCount -1], "&") == 0) {
				*background = 1;
			}
		} 
                if(parsedArray[0] == NULL) parsedArray[0] = "#";	
                return (wordCount-1);
        }

}

/*
 * The redirection function checks the parsed user input for the '<', '>', '&' characters.
 * If '<' is found this function sets stdin to the item in the array after '<'. Likewise if
 * '>' is found this function sets stdout to the item in the array after '>'. Finally, if 
 * '&' is found this function sets stdin/stdout to /dev/null if they have not already been
 * redirected.
 */
void redirection(char** parsedArray, int wordCount) {
	int i;
	char* pathin;
	char* pathout;
	pathin = malloc(sizeof(char)*16);
	pathout = malloc(sizeof(char)*16);
	int foundin = 0;
	int inOpen;
	int foundout = 0;
	int outOpen;
	
	for(i=0;i<wordCount;i++) {
		//looks for stdin redirect character
		if(strcmp(parsedArray[i], "<") == 0 && foundin == 0){
			parsedArray[i] = "\0";
			strcpy(pathin, parsedArray[i+1]);
			parsedArray[i+1] = "\0";
			foundin = 1; //will prevent stdin from being redirected to /dev/null for background process
			inOpen = open(pathin, O_RDONLY);
			if(inOpen == -1) {
				printf("File open for reading failed.\n");
			}
			dup2(inOpen, 0);
		}
		//looks for stdout redirect character
		else if(strcmp(parsedArray[i], ">") == 0 && foundout == 0) {
			parsedArray[i] = "\0";
			strncpy(pathout, parsedArray[i+1], sizeof(parsedArray[i+1]));
			parsedArray[i+1] = "\0";
			foundout = 1; //will prevent stdout from being redirected to /dev/null for background process
			outOpen = open(pathout, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if(outOpen == -1) {
				printf("File open for writing failed.\n");
			}
			dup2(outOpen, 1);
		}
		if(foundin == 1 && foundout == 1) break;
	}
	//looks for background process character
	if(strcmp(parsedArray[wordCount], "&") == 0) {
		strcpy(parsedArray[wordCount], '\0');
		if(foundin == 0 && foreGround == 0)  {
			inOpen = open("/dev/null", O_RDONLY);
			dup2(inOpen, 0);
		}
		if(foundout == 0 && foreGround == 0) {
			outOpen = open("/dev/null", O_WRONLY);
			dup2(outOpen, 1);
		}
	}
	
	free(pathin);
	free(pathout);
}

/*
 * The forkFunc function forks the current process
 */
pid_t forkFunc(char** parsedArray, int wordCount, pid_t childPID, int *childExitStatus, int* background) {
        childPID = fork();
        switch(childPID) {
		//error with fork
                case -1: {perror("Fork error\n"); exit(1);break;}
		//child process runs this
                case 0: {
			redirection(parsedArray, wordCount);
                        if(execvp(parsedArray[0], parsedArray) == -1) {
				printf("execvp failed \n");
				fflush(stdout);
			}
                        break;
                }
		//parent process runs this
                default: {
			if(*background != 1) { //wait for foreground process to end before continuing
				childPID = waitpid(childPID, &(*childExitStatus), 0);
			}
			else printf("background pid is %d\n", childPID);
        	}
	}
	return childPID;
}

/*
 * The welfareCheck function will display the pid and exit status of the most recently reaped
 * child.
 */
void welfareCheck(pid_t childPID, int childExitMethod) {
	if(WIFEXITED(childExitMethod) != 0) {
                int exitStatus = WEXITSTATUS(childExitMethod);
                printf("background pid %d is done: exit value %d\n", childPID, exitStatus);
                fflush(stdout);
        }
        else if(WIFSIGNALED(childExitMethod) != 0) {
                int termSignal = WTERMSIG(childExitMethod);
                printf("background pid %d is done: terminated by signal %d\n", childPID, termSignal);
                fflush(stdout);
        }
	
}

/*
 * The catchSIGCHILD function is called when the SIGCHILD signal is received. It calls waitpid and
 * sends the results to welfareCheck.
 */
void catchSIGCHLD(int sig) {
	int childExitMethod;
	pid_t child;

	child = waitpid(-1, &childExitMethod, 0);
	welfareCheck(child, childExitMethod);	
}

int main(){
/*
 * The two commented out code blocks below are supposed to handle SIGTSTP (ctrl+z) and SIGCHLD signals.
 * The SIGTSTP code block upon receiving the SIGTSTP signal would call the foregroundOnly function which
 * changes the foreGround global variable and either enables or disables the use of background processes
 * and also displays text to the user stating the current mode.
 *
 * The SIGCHLD code block upon receiving the SIGCHLD signal calls the catchSIGCHLD function which waits
 * for the child process to finish exiting and sends the relevant exit information to welfareCheck and that
 * function displays the PID and exit condition to the user.
 *
 * The reason these sections of code are commented out is because upon receiving their signals they do perform
 * the intended action but after that the program gets stuck in a loop displaying the user input prompt ": "
 * repeatedly. Unfortunately I ran out of time to debug this.
 */

	
	//struct sigaction SIGTSTP_action = {0};
	//SIGTSTP_action.sa_handler = foregroundOnly;
	//sigfillset(&SIGTSTP_action.sa_mask);
	//SIGTSTP_action.sa_flags = 0;
	//sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	
	//struct sigaction SIGCHLD_action = {0}, act = {0};
	//SIGCHLD_action.sa_handler = catchSIGCHLD;
	//sigfillset(&SIGCHLD_action.sa_mask);
	//SIGCHLD_action.sa_flags = 0;
	//sigaction(SIGCHLD, &SIGCHLD_action, NULL);
	

	//causes the process to ignore ctrl+c inputs
	struct sigaction act = {0};
	act.sa_handler = SIG_IGN;
	sigfillset(&act.sa_mask);
	act.sa_flags = 0;
	
	sigaction(SIGINT, &act, NULL);

	char** parsedInput;//[519][16]; //support maximum of 1 command, 512 arguments, input redirect (< plus path),
	parsedInput = malloc(sizeof(char*) * 512);			   //output redirect (> plus path), background character (&)
	int parsedWordCount;
	size_t inputSize = 2048;
	char *unparsedInput; //support inputs with a maximum length of 2048 characters
	unparsedInput = (char*) malloc(inputSize);
	int childExitMethod = -5;
	pid_t childPID = -5;
	int* background =(int*)malloc(sizeof(int));		
	
	while(1) {
		childPID = 0;
		*background = 0;
		printf(": ");
		fflush(stdout);
		memset(unparsedInput, '\0', inputSize);
		getline(&unparsedInput, &inputSize, stdin);
		size_t size = strlen(unparsedInput)-1;
		if(unparsedInput[size] == '\n') unparsedInput[size] = '\0';
		parsedWordCount = parseInput(unparsedInput, parsedInput, parsedWordCount, background );	
		
		char* comment;
		comment = strchr(parsedInput[0], '#');
		if(comment != NULL) {
		//intentionally left blank
		}
				
		else if(strcmp(parsedInput[0], "exit") == 0) {
			exitSH();
		}
		
		else if(strcmp(parsedInput[0], "cd") == 0) {
			cd(parsedInput[1]);
		}
		
		else if(strcmp(parsedInput[0], "status") == 0) {
			status(childExitMethod);
		}

		else { //user input not a predefined function, blank input, or a comment
			childPID = forkFunc(parsedInput, parsedWordCount, childPID, &childExitMethod, background);
		}

/*
 * The following 4 lines of code are a stop-gap measure because the SIGCHLD catch causes an unrecoverable loop of the user input
 * prompt ": ". If that was resolved comment out the 4 lines below to correctly handle child processes ending.
 */
		childPID = waitpid(-1, &childExitMethod, WNOHANG); //remove if SIGCHLD can be handled
		if(childPID > 0) {				   //remove if SIGCHLD can be handled
			welfareCheck(childPID, childExitMethod);   //remove if SIGCHLD can be handled
		}						   //remove if SIGCHLD can be handled
		
	}	

	return 0;
}
