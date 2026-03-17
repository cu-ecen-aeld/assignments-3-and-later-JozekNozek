#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>

bool signalReceived = false;

static void singalHandler(int signal_number)
{
    if( (signal_number == SIGINT) || (signal_number == SIGTERM) ) signalReceived = true;
}

static char* read_file_bytes(const char* filename, size_t* size_out)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("fopen failed");
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("fseek failed");
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        perror("ftell failed");
        fclose(file);
        return NULL;
    }

    rewind(file);

    char* buffer = malloc(size);
    if (!buffer) {
        perror("malloc failed");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, size, file);
    if (bytes_read != (size_t)size) {
        perror("fread incomplete");
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);
    if( size_out ) *size_out = size;
    return buffer;
}

int main(int argc, char *argv[])
{
    int currPID = 0;
    printf("\r\n\r\nLets run assignment 5\r\n\r\n");

    bool daemon_mode = false;
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        printf("\r\nDAEMON MODE ON\r\n");
        daemon_mode = true;
    }

    int retval = 0;

    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(struct sigaction));
    sig_action.sa_handler = singalHandler;

    if( sigaction(SIGTERM, &sig_action, NULL) != 0 )
    {
        printf("[ERROR %d (%s)] Registering handler for SIGTERM FAIL!\r\n", errno, strerror(errno));
        return -1;
    }

    if( sigaction(SIGINT, &sig_action, NULL) != 0 )
    {
        printf("[ERROR %d (%s)] Registering handler for SIGINT FAIL!\r\n", errno, strerror(errno));
        return -1;
    }

    struct addrinfo *sockaddrinfo;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    openlog(NULL, LOG_CONS, LOG_USER);

    int socketfd = socket(PF_INET, SOCK_STREAM, 0);
    if( socketfd == -1 )
    {
        // some error logging
        printf("[ERROR %d (%s)] socket syscall returned -1\r\n", errno, strerror(errno));
        return -1;
    }

    int yes = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt");
        printf("[ERROR %d (%s)] setsockopt returned -1\r\n", errno, strerror(errno));
    }

    int addrinforeturncode = getaddrinfo(NULL, "9000", &hints, &sockaddrinfo);
    if( addrinforeturncode != 0 )
    {
        // some error logging
        printf("[ERROR %d (%s)] getaddrinfo returned -1\r\n", errno, strerror(errno));
        return -1;
    }

    do
    {
        int bindreturncode = bind(socketfd, sockaddrinfo->ai_addr, sockaddrinfo->ai_addrlen);
        if( bindreturncode == -1 )
        {
            // some error logging
            printf("[ERROR%d (%s)] bind syscall returned -1\r\n", errno, strerror(errno));
            retval = -1;
            break;
        }

        if( daemon_mode )
        {
            int pid = fork();
            currPID = pid;
            if( pid == -1 )
            {
                printf("[ERROR%d (%s)] fork fail\r\n", errno, strerror(errno));
                break;
            }

            if( pid != 0 ) { exit(0); }
            // else 
            // {
            //     setsid();
            //     chdir("/");
            //     umask(0);
            //     close(STDIN_FILENO);
            //     close(STDOUT_FILENO);
            //     close(STDERR_FILENO);
            // }
        }
        

        int listenreturncode = listen(socketfd, 5);     // Could try with 0 maybe instead of 5?
        if( listenreturncode == -1 )
        {
            // some error logging
            printf("[ERROR%d (%s)] listen syscall returned -1\r\n", errno, strerror(errno));
            retval = -1;
            break;
        }

        while( !signalReceived )
        {
            struct sockaddr recvConnAdr;
            socklen_t len = sizeof(struct sockaddr);
            int acceptfd = accept(socketfd, &recvConnAdr, &len);
            if( acceptfd == -1 )
            {
                // some error logging
                printf("[ERROR %d (%s)] accept returned -1\r\n", errno, strerror(errno));
                retval = -1;
                break;
            }
            
            struct sockaddr_in *connAdr = (struct sockaddr_in *)&recvConnAdr;
            char IP[16] = {0};
            inet_ntop(AF_INET, &connAdr->sin_addr, IP, sizeof(IP));
            printf("\r\nACCEPTED CONNECTION FROM ADDR: %s\r\n", IP);
            syslog(LOG_DEBUG, "Accepted connection from %s", IP);

            int bufferSize = 1024 * 1024;
            char *recvBuff = malloc(bufferSize);
            if( !recvBuff )
            {
                printf("[ERROR %d (%s)] malloc returned NULL\r\n", errno, strerror(errno));
                retval = -1;
                break;
            }
            memset(recvBuff, 0, bufferSize);
            int bytesRead = 0;
            bool packageReceived = false;

            while( !signalReceived && !packageReceived )
            {
                FILE *fptr = fopen("/var/tmp/aesdsocketdata.txt", "a+");
                if( !fptr )
                {
                    printf("[ERROR%d (%s)] NULL file pointer to /var/tmp/aesdsocketdata.txt, option a+\r\n", errno, strerror(errno));
                    retval = -1;
                    break;
                }
                
                int recvBytes = recv(acceptfd, recvBuff + bytesRead, bufferSize - bytesRead, 0);
                if( recvBytes < 1 )
                {
                    printf("[ERROR %d (%s)] recv returned: %d\r\n", errno, strerror(errno), recvBytes);
                    fclose(fptr);
                    break;
                }

                for(int i = 0; i < recvBytes; i++)
                {
                    if( *(recvBuff + bytesRead + i) == '\n' )
                    {
                        recvBytes = i + 1;
                        packageReceived = true;
                        break;
                    }
                }

                fwrite(recvBuff + bytesRead, 1, recvBytes, fptr);
                bytesRead += recvBytes;
                // printf("\r\nOy Oy socket received this string[%d]: %s\r\n", recvBytes, recvBuff);

                fclose(fptr);

                if( bytesRead >= bufferSize ) break;
            }

            size_t size;
            char *data = read_file_bytes("/var/tmp/aesdsocketdata.txt", &size);
            if( data )
            {
                printf("Read %zu bytes from /var/tmp/aesdsocketdata.txt\n", size);
                send(acceptfd, data, size, 0);
                free(data);
            }

            close(acceptfd);
            syslog(LOG_DEBUG, "Closed connection from %s", IP);    
            
            free(recvBuff);

            if( !data )
            {
                retval = -1;
                break;                
            }
        }

        syslog(LOG_DEBUG, "Caught signal, exiting");
        printf("\r\nCaught signal, exiting\r\n");

        if( retval == -1 ) break;

    } while(0);

    /* LAST FUNCTION CALLS: free allocated memory to avoid memory leaks */
    remove("/var/tmp/aesdsocketdata.txt");
    freeaddrinfo(sockaddrinfo);
    close(socketfd);
    closelog();

    printf("\r\n\r\n%s process finished, returning: %d\r\n\r\n", currPID == 0 ? "CHILD" : "PARENT", retval);

    return retval;
}