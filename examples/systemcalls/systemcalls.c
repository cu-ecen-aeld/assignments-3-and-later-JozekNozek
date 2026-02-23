#include "systemcalls.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

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
    int systemRetval = system(cmd);
    // printf("\r\ndo_system(%s) systemRetval: %d\r\n", cmd, systemRetval);
    return !(systemRetval == -1);
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

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    // printf("\r\ndo_exec ARGUMENTS PASSED: |");
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        // printf(" [%d]: %s |", i , command[i]);
    }
    // printf("\r\n\r\n");
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    // printf("\r\nPARENT PROCESS PID: %d\r\n", getpid());
    fflush(stdout);
    int pid = fork();
    // printf("\r\n[do_exec] execv(");
    // for(int iter = 0; iter < count + 1; iter++)
    // {
    //     if( !iter ) { printf("%s", command[iter]); }
    //     else if( command[iter] ) { printf(", %s", command[iter]); }
    // }
    // printf("); | CHILD PID: %u\r\n\r\n", pid);
    bool retval = false;
    do
    {
        if( pid == -1 ) break;
        if( pid == 0 ) 
        {
            // printf("\r\nLets run execv with -> %s\r\n", command[0]);
            /* int execvRetval = */execv(command[0], command);
            // printf("\r\ndo_exec(%s, ...) execv returns: %d\r\n", command[0], execvRetval);
            // if( execvRetval == -1)
            if( 1 )
            {
                // printf("\r\n[CHILD] do_exec(%s, ...) execv FAIL\r\n", command[0]);
                // break;
                exit(127);
            }
        }
        else
        {
            int cmdStatus = 0;
            int waitpidRetval = waitpid(pid, &cmdStatus, 0);
            // printf("\r\n[PARENT] do_exec(%s, ...) waitpid(%d, status = %d, 0) returns: %d\r\n", command[0], pid, cmdStatus, waitpidRetval);
            if( waitpidRetval == -1 )
            {
                // printf("\r\n[PARENT] do_exec(%s, ...) waitpid(%d, status = %d, 0) FAIL\r\n", command[0], pid, cmdStatus);
                break;
            }
            if( !WIFEXITED(cmdStatus) )
            {
                // printf("\r\n[PARENT] do_exec(%s, ...) cmdStatus: %d -> FAIL\r\n", command[0], cmdStatus);
                break;
            }
            int wexitstatus = WEXITSTATUS(cmdStatus);
            // printf("\r\n[PARENT] do_exec(%s, ...) wexitstatus: %d\r\n", command[0], wexitstatus);
            if( wexitstatus != 0 ) break;
        }

        retval = true;

    } while(0);

    va_end(args);

    // printf("\r\n[do_exec] execv(");
    // for(int iter = 0; iter < count + 1; iter++)
    // {
    //     if( !iter ) { printf("%s", command[iter]); }
    //     else if( command[iter] ) { printf(", %s", command[iter]); }
    // }
    // printf("); | PID: %d, %s returning: %s\r\n\r\n", getpid(), pid == 0 ? "CHILD" : "PARENT", retval == true ? "TRUE" : "FALSE");

    return retval;
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
    char * command[count + 1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL; 

/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    fflush(stdout);
    int fd = open(outputfile, O_CREAT | O_RDWR | O_TRUNC, 0644);
    int pid = fork();
    // printf("\r\n[do_exec_redirect] execv(");
    // for(int iter = 0; iter < count + 1; iter++)
    // {
    //     if( !iter ) { printf("%s", command[iter]); }
    //     else if( command[iter] ) { printf(", %s", command[iter]); }
    // }
    // printf("); | PID: %u\r\n\r\n", pid);
    bool retval = false;
    do
    {
        if( pid == -1 ) break;
        if( pid == 0 ) 
        {
            if( dup2(fd, 1) == -1 )
            {
                // printf("\r\ndup2 FAIL\r\n");
                break;
            }

            // printf("\r\n[do_exec_redirect] execv(");
            // for(int iter = 0; iter < count + 1; iter++)
            // {
            //     if( !iter ) { printf("%s", command[iter]); }
            //     else if( command[iter] ) { printf(", %s", command[iter]); }
            // }
            // printf(");\r\n\r\n");
            // printf("LETS EXECUTE THIS SHIIIIET");
            /*int execvRetval = */execv(command[0], command);
            // printf("\r\ndo_exec_redirect(%s, ...) execv returns: %d\r\n", command[0], execvRetval);
            // if( execvRetval == -1)
            if( 1 )
            {
                // printf("\r\ndo_exec_redirect(%s, ...) execv FAIL\r\n", command[0]);
                // break;
                exit(127);
            }
        }
        else
        {
            // printf("\r\ndo_exec(...) PARENT PROCESS\r\n");
            int cmdStatus = 0;
            int waitpidRetval = waitpid(pid, &cmdStatus, 0);
            // printf("\r\ndo_exec_redirect(%s, ...) waitpid(%d, status = %d, 0) returns: %d\r\n", command[0], pid, cmdStatus, waitpidRetval);
            if( waitpidRetval == -1 )
            {
                // printf("\r\ndo_exec_redirect(%s, ...) waitpid(%d, status = %d, 0) FAIL\r\n", command[0], pid, cmdStatus);
                break;
            }
            if( !WIFEXITED(cmdStatus) )
            {
                // printf("\r\ndo_exec_redirect(%s, ...) cmdStatus: %d -> FAIL\r\n", command[0], cmdStatus);
                break;
            }
            int wexitstatus = WEXITSTATUS(cmdStatus);
            // printf("\r\ndo_exec_redirect(%s, ...) wexitstatus: %d\r\n", command[0], wexitstatus);
            if( wexitstatus != 0 ) break;
        }

        retval = true;

    } while(0);

    if( close(fd) == -1 ) printf("ERROR CLOSING FILE!");

    va_end(args);

    // printf("\r\ndo_exec_redirect(%s, ...) returning: %s\r\n", command[0], retval == true ? "TRUE" : "FALSE");

    return retval;
}
