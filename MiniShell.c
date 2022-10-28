#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

// MiniShell.c
// Partner: Victor Chhun
// Student ID#: 87506313

// Partner: Haver Ho
// Student ID#: 54379591
#define MaxLine 80
#define MaxArgs 80
#define MaxJob 5


// Struct for holding job information
struct Job{
	int job_id;
	int pid_id;
	char status[MaxLine];
	char cmd[MaxLine];
	char state[MaxLine];

};

// Global Variables

struct stat buf;
pid_t gpid; // to hold pid
struct Job jobArray[MaxJob] = {{-1}, {-1}, {-1}, {-1}, {-1}};
mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
// ------------------------------------------------------------- //
// Declare all functions

//	Signals	Handlers
void sigint();		// ctrl + c
void sigchld();		// reap process
void sigtstp();		// ctrl + z
void sigcont();		// continue process

//	 Built-ins
void pwd(char directory[80]);	// get current directory
void cd(char directory[80]); 	// go to directory
void fg(char* tag);	// brings process to foreground
void bg(char* tag);	// brings process to background

//	Miscellaneous
void foreGround(char* exe[MaxArgs], int jobCount);	// foreground process
void backGround(char* exe[MaxArgs], int jobCount);	// background process
void deleteJob();					// Delete job from jobArray
void killCMD(char* tag);				// user input kill command
int jobs();						// Prints jobArray
int addJob(int jobCount, int pid, char* status, char* cmd, char* state); // add to jobArray
void inputDirection(char* fname[MaxArgs]);			// Change Direction to input file
void outputDirection(char* fname[MaxArgs], char* type);		// Change Direction to output file
void biDirection(char* fname[MaxArgs]);				// Change Direction to both input/output
// ------------------------------------------------------------- //
// Signal Handlers

// Ctrl + c
void sigint(){
	//printf("---Sigint called, id = %d---\n", gpid);
	deleteJob();
	kill(gpid, SIGTERM);
}

// Reap Process
void sigchld(){
	int status;
	pid_t pid;
	//printf("---sigchld called, id = %d---\n", gpid);
	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){
		//printf("---in sigchld, pid = %d---\n", pid);
		if (WIFEXITED(status) || WIFSIGNALED(status) || WIFSTOPPED(status)){
			gpid = pid;
			deleteJob();
		}
	}
	
}

// Ctrl + zi
void sigtstp(){
//	printf("---sigtstp called, id = %d---\n", gpid);
	signal(SIGCHLD, SIG_IGN);
	int tempPID = gpid;
	for (int i = 0; i < MaxJob; i++){
		if (jobArray[i].pid_id == tempPID){// &&
		//	strcpy(jobArray[i].status, "Running") == 0){
			
			strcpy(jobArray[i].status, "Stopped");
			//printf("---Killing %d with SIGTSTP signal---\n", tempPID);
			kill(tempPID, SIGSTOP);
		}
	}
//	printf("---Exiting sigtstp---\n");
}

// Continue Process
void sigcont(){
	//printf("---sigcont called, id = %d ---\n", gpid);
	kill(gpid, SIGCONT);
}

// ------------------------------------------------------------- //

// Built-ins commands

// pwd command
void pwd(char directory[80]){
	printf("%s\n", getcwd(directory,80));
}

// cd command
void cd(char directory[80]){
	chdir(directory);
}

// fg command
void fg(char* tag){
	//printf("---You've called fg command, id = %d---\n", gpid);
	int status;
	int isJobID = 0;
	int val;
	if (strncmp(tag, "%", 1) == 0){
		tag++;
		val = atoi(tag);
		isJobID = 1;
	}
	else{
		val = atoi(tag);
	}

	for (int i = 0; i < MaxJob; i++){
		if ((jobArray[i].job_id == val && isJobID == 1)
			|| (jobArray[i].pid_id == val && isJobID == 0)){
				//printf("---Found correct pid_id in jobArray, id = %d---\n", gpid);
				strcpy(jobArray[i].status, "Running");
				strcpy(jobArray[i].state, "fg");
				signal(SIGINT, sigint);
				signal(SIGTSTP, sigtstp);
				
				gpid = jobArray[i].pid_id;
				kill(gpid, SIGCONT);
				//printf("---Waiting for signal---\n");
				pid_t wpid = waitpid(gpid, &status, WUNTRACED);
				if (status == 0){
					gpid = jobArray[i].pid_id;
					deleteJob();
				}
		}
	}
}

// bg commannd
void bg(char* tag){
	//printf("---You've called bg command, id = %d---\n", gpid);
	int status;
	int isJobID = 0;
	int val;
	if(strncmp(tag, "%", 1) == 0){
		tag++;
		val = atoi(tag);
		isJobID = 1;
	}
	else{
		val = atoi(tag);
	}
	signal(SIGCHLD, sigchld);
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	for (int i = 0; i < MaxJob; i++){
		if ((jobArray[i].job_id == val && isJobID == 1)
			|| (jobArray[i].pid_id == val && isJobID == 0)){
				//printf("---Found correct pid_id in jobArray, id = %d---\n", gpid);
				strcpy(jobArray[i].status, "Running");
				strcpy(jobArray[i].state, "bg");
				gpid = jobArray[i].pid_id;
				kill(gpid, SIGCONT);
		}
	}
}

// ------------------------------------------------------------- //

// Miscellaneous

// Foreground process
void foreGround(char* exe[MaxArgs], int jobCount){
	//printf("---ForeGround was issued, id = %d---\n", gpid);
	pid_t pid;
	signal(SIGCHLD, sigchld);
	// Child Process
	if ((pid = fork()) == 0){
		signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		if (execv(exe[0], exe) < 0)
			execvp(exe[0], exe);
		exit(0);
	}
	// Parent Process
	else{
		int status;
		signal(SIGTSTP, sigtstp);
		signal(SIGINT,  sigint);
		gpid = pid;
		addJob(jobCount, gpid, "Running", exe[0], "fg");
		waitpid(gpid, &status, WUNTRACED);
		signal(SIGCHLD, sigchld);
		//printf("---Status for foreground/wuntraced = %d---\n", status);
		if (status == 0){
			gpid = pid;
			deleteJob();
		}
	}
}

// Background process
void backGround(char* exe[MaxArgs], int jobCount){
	//printf("---BackGround was issued, id = %d---\n", gpid);
	pid_t pid;
	signal(SIGINT, SIG_IGN);
	char amper[MaxLine] = " &";
	char temp[MaxLine];
	strcpy(temp, exe[0]);
	strcat(temp, amper);
		
	// Child Process
	if  ((pid = fork()) == 0){
		//setpgid(0, 0);
		signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		if (execv(exe[0], exe) < 0)
			execvp(exe[0], exe);
		exit(0);
	}
	// Parent Process
	else{
		signal(SIGCHLD, sigchld);
		gpid = pid;
		addJob(jobCount, gpid, "Running", temp, "bg");
	}
}

// Issued kill command
void killCMD(char* tag){
	//printf("---Kill command was issued, id = %d---\n", gpid);
	int status;
	int isJobID = 0;
	int val;
	if (strncmp(tag, "%", 1) == 0){
		tag++;
		val = atoi(tag);
		isJobID = 1;
	}
	else{
		val = atoi(tag);
	}

	for (int i = 0; i < MaxJob; i++){
		if ((jobArray[i].job_id == val && isJobID == 1)
			|| (jobArray[i].pid_id == val && isJobID == 0)){
			//	printf("---Found correct pid_id in jobArray, id = %d---\n", gpid);
				signal(SIGCHLD, sigchld);
				kill(gpid, SIGKILL);
				deleteJob();
		}
	}

}

// Delete job from jobArray
void deleteJob(){
	for (int i = 0; i < MaxJob; i++){
		if (jobArray[i].pid_id == gpid){
		//	printf("---Deleting pid_id: %d---\n", gpid);
			jobArray[i].job_id = -1;
		}
	}
}

// Print all current/active jobs
int jobs(){
	for (int i = 0; i < MaxJob; i++){
		if (jobArray[i].job_id != -1){
			printf("[%d] (%d) %s %s\n",
					jobArray[i].job_id,
					jobArray[i].pid_id,
					jobArray[i].status,
					jobArray[i].cmd);
		}
	}
}

// add job to jobArray
int addJob(int jobCount, int pid, char* status, char* cmd, char* state){

	for (int i =0; i < MaxJob; i++){
		if (jobArray[i].job_id == -1){	// Finds the first empty slot
		//	printf("---Adding %d to jobArray---\n", pid);
			jobArray[i].job_id = jobCount;
			jobArray[i].pid_id = pid;
			strcpy(jobArray[i].status, status);
			strcpy(jobArray[i].cmd, cmd);
			strcpy(jobArray[i].state, state);
			break;
		}
	}
}

// Redirection
// input
void inputDirection(char* fname[MaxArgs]){
	printf("---Changing input!---\n");
	pid_t pid;
	int status;
	char* argv[] = {fname[0], NULL};
	
	// Child Process
	if ((pid = fork() == 0)){

		int inFileID = open(fname[2], O_RDONLY, mode);
		dup2(inFileID, STDIN_FILENO);
		execv(argv[0], argv);
		execvp(argv[0], argv);
	}
	else{
		gpid = pid;
		waitpid(pid, &status, WUNTRACED);
	}

}

// output
void outputDirection(char* fname[MaxArgs], char* type){
	printf("---Changing output direction with type %s---\n", type);
	pid_t pid;
	int status;
	char* argv[] = {fname[0], NULL};
	// Child Process
	if ((pid = fork() == 0)){
		int outFileID;
		if (strcmp(fname[1], ">") == 0){
			outFileID = open(fname[2], O_CREAT | O_WRONLY | O_TRUNC, mode);
		//	dup2 (outFileID, STDOUT_FILENO);
		}
		else{
			outFileID = open(fname[2], O_CREAT | O_APPEND | O_WRONLY, mode);
		//	dup2 (outFileID, STDOUT_FILENO);
		}
		dup2 (outFileID, STDOUT_FILENO);
		execv(argv[0], argv);
		execvp(argv[0], argv);
		exit(0);
	}
	else{
		gpid = pid;
		waitpid(pid, &status, WUNTRACED);
	}


}

// bidirectional
void biDirection(char* fname[MaxArgs]){
	printf("---CHANGING BIDIRECTIONAL---\n");
	pid_t pid;
	int inFileID, outFileID, status;
	char* argv[] = {fname[0], NULL};
	if ((pid = fork()) == 0){
		inFileID = open(fname[2],  O_RDONLY, mode);
		dup2(inFileID, STDIN_FILENO);
		if (strcmp(fname[3], ">") == 0){
			outFileID = open(fname[4], O_CREAT | O_WRONLY | O_TRUNC, mode);
		}
		else{
			outFileID = open(fname[4], O_CREAT | O_APPEND | O_WRONLY, mode);
		}
		dup2 (outFileID, STDOUT_FILENO);

		execv(argv[0], argv);
		execvp(argv[0], argv);
		exit(0);
	}
	else{
		gpid = pid;
		waitpid(pid, &status, WUNTRACED); 
	}

}
int isArrayOpen(){
	for (int i = 0; i < MaxJob; i++){
		if (jobArray[i].job_id == -1){
			return 1;
		}

	}
	return 0;
}

// Main Function
int main(){
	char cmdLine[MaxLine];
	char cmdCpy[MaxLine];
	const char* delim = " \n\t";
	int jobCount = 0;
	
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	do{
		
		signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGCHLD, sigchld);
		//if (isArrayFull()){	
			printf("prompt > ");
			fgets(cmdLine, MaxLine, stdin);
		
			strcpy(cmdCpy, cmdLine);
			if (strncmp(cmdLine, "quit", 4) == 0){
				exit(0);
			}
			char* token =  strtok(cmdCpy, delim);
			if (token != NULL){
			// Calls pwd
			if (strncmp(cmdLine, "pwd", 3) == 0){
				pwd(cmdLine);
			}

			// Calls cd
			else if (strncmp(cmdLine, "cd", 2) == 0){
				token =  strtok(NULL, delim);
				chdir(token);
				pwd(getcwd(cmdLine,80));
			}

			// Calls job
			else if (strcmp(cmdLine, "jobs\n") == 0){
				jobs();
			}
			
			// Calls fg
			else if (strncmp(token, "fg", 2) == 0){
				token = strtok(NULL, delim); // tag
				fg(token);
			}

			// Calls bg
			else if (strncmp(token, "bg", 2) == 0){
				token = strtok(NULL, delim); // tag
				bg(token);
			}

			else {
			char temp[MaxLine];
					strcpy(temp, cmdLine);			
					char* argv[MaxArgs];
					int i = 0;
					// Tokenizing all args
					token = strtok(temp, delim);
				
					while (token != NULL){
						argv[i] = token;
						token = strtok(NULL, delim);
						i++;
					}			
					argv[i] = NULL;
					// Foreground process
					 if (argv[0] != NULL && argv[1] == NULL || (i == 1 && argv[0] != NULL)){
						signal(SIGINT, sigint);
						signal(SIGTSTP, sigtstp);
						 if (!stat(argv[0], &buf)){
							if (isArrayOpen()){	
								jobCount++;
								foreGround(argv, jobCount);
							}
						}
						else{
							foreGround(argv, jobCount);
						 }
						}
				
					// Background Process
					else if (argv[0] != NULL && strcmp(argv[1], "&") == 0){
						signal(SIGCHLD, sigchld);
						if (isArrayOpen()){
							jobCount++;
							backGround(argv, jobCount);
						}
					}
					
					// Kill cmd
					else if (argv[0] != NULL && strcmp(argv[0], "kill") == 0){
							killCMD(argv[1]);
					}
					else if (argv[0] != NULL){
						signal(SIGINT, sigint);
						signal(SIGTSTP, sigtstp);
					
						if (i > 3 && argv[0] != NULL && strcmp(argv[1], "<") == 0 && argv[2] != NULL &&
								 (strcmp(argv[3], ">") == 0 || strcmp(argv[3], ">>") == 0) && argv[4] != NULL){
							biDirection(argv);
						 }

						else if (i > 1 && argv[0] != NULL && strcmp(argv[1], "<") == 0){
							inputDirection(argv);
						}
				
						//	output
						else if (i > 1 && argv[0] != NULL && (strcmp(argv[1], ">")  == 0) || strcmp(argv[1], ">>") == 0){
							outputDirection(argv, argv[1]);
						}
					}
				}
			}
	//	}
	} while (strcmp(cmdLine, "quit\n") != 0 || strncmp(cmdLine, "quit", 4) != 0 || feof(stdin));
	// Will only quit when user input quit
	return 0;
}

