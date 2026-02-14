#include <stdio.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[])
{
    openlog(NULL, LOG_CONS, LOG_USER);
    if( (argc < 3) || !argv[1] || !argv[2] )
    {
        syslog(LOG_ERR, "Invalid arguments!");   // Some logging
        return 1;
    }

    const char *writeStr = argv[2];
    const char *fileName = argv[1];
    FILE* fptr = fopen(fileName, "w+");
    if( !fptr ) 
    {
        char errorMsg[150] = {0};
        strcpy(errorMsg, "Could not open file: ");
        strcat(errorMsg, fileName);
        syslog(LOG_ERR, errorMsg);
        return 1;   // Some logging
    }

    char debugMsg[150] = {0};
    strcpy(debugMsg, "Writing string: ");
    strcat(debugMsg, writeStr);
    strcat(debugMsg, " to file: ");
    strcat(debugMsg, fileName);
    syslog(LOG_DEBUG, debugMsg);
    fprintf(fptr, writeStr);
    fclose(fptr);
    closelog();

    return 0;
}
