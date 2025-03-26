#include "systemcalls.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

extern char **environ;
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int ret =0;	
    ret = system(cmd);
    if (ret == -1)
	    return false;
    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/


void print_env(char* name, char **env)
{
   char cmpVal[6];
   for(int i=0; env[i]!=NULL; i++)
    {
	strncpy(cmpVal,env[i],6);
	cmpVal[5]=0;
	if (strcmp(cmpVal,"PATH=") == 0) {
		printf("Found PATH - reset it\n");
		env[i][5] = 0;
	}
    
        //printf("%s: %s\n",name, env[i]);

    }
}
bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    int status = 0;
    char fullcmd[100];
    strcpy(fullcmd, command[0]);
    //print_env("env", environ);

    int pid = fork();
    if (0 == pid) {
	command[0] = basename(command[0]);
//	printf("full:%s, command:%s %s %s \n",fullcmd, command[0], command[1], command[2]);
	if (-1 == execv(fullcmd, &command[0]) ) {
		perror("execv"); return false;
	}
    }		
    else // This is the parent
    { 
	if (-1 == pid) {
	 	perror("fork"); return false;
	}
    	 //Pid>0 
	//printf("== parent before wait ==\n");
  	//printf("cmd:%s, arg:%s, %s, status:%x\n", command[0], command[1],command[2], status);
	//int ret = wait(&status);
	int ret = waitpid(pid, &status, 0);
	if (-1 == ret) {
		perror("wait"); return false;
	}

	//printf("== parent After wait :ret:%d ==\n",ret);
  	//printf("cmd:%s, status:%x\n", command[0],status);
	if WIFEXITED(status) {
	//	printf("WEXITSTATUS(status) :%d\n",WEXITSTATUS(status) );
		if (WEXITSTATUS(status) == 0 && status == 0)
			return true;
		else
			return false;
	}
    }
    va_end(args);
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    int status = 0;
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (fd < 0) { perror("open"); abort(); }
    
    char fullcmd[100];
    strcpy(fullcmd, command[0]);
  
   int pid = fork();
   if (-1 == pid) {
	perror("fork"); return false;
   }
   else if (0 == pid) {


	 if (dup2(fd, 1) < 0) { perror("dup2"); return false; }
	 close(fd);

	command[0] = basename(command[0]);
	//printf("full:%s, command:%s\n",fullcmd, command[0]);

	if (-1 == execv(fullcmd, command) ) {
		perror("execv"); return false;
	}
    }		
    else // This is the parent
    { 
    	 //Pid>0 
	int ret = waitpid(pid, &status, 0);

	//int ret = wait(&status);

	close(fd);
	if (-1 == ret) {
		perror("wait"); return false;
	}
	if WIFEXITED(status) {
		//printf("WEXITSTATUS(status) :%d\n",WEXITSTATUS(status) );
		if (WEXITSTATUS(status) == 0 && status == 0)
			return true;
		else
			return false;
	}
    }
    
    va_end(args);

    return true;
}
