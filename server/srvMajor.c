/*
University of North Texas Spring '17
CSCE3600 Systems Programming Major Assignment
Written by Andrew Buikema, Matt Salway, Guthrie Schoolar and Cassidy Susa
Purpose: Create a messaging system using sockets so that up to 4 clients can chat
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#define NUM_THREADS 4
    

//global holding socketID's
int socketID[4];
char globalBuffer [2048];
char globalFileBuffer [4098], sendFile[4098];
pthread_mutex_t global_lock; // mutex lock
FILE *fp;

//function to write line by line what the file from server says
void fileWrite (char *fileName, char *text) {
    FILE *fp;
    fp = fopen(fileName, "w"); //open file for writing
    fputs(text, fp);
    fclose(fp); //always close file after writing!
    return;
}//fileWrite

void *msgServer(void *threadid)
{
    long tid;
    tid = (long) threadid;
    int sid, bufferSize, i;
    char buffer[2048], *name;
    char *command, *content;
    
    sid = socketID[tid];
        //connect to client if one is present
        //accept client connection
        //handshake to find client's screen name
//    bufferSize = recv(sid, name, sizeof(name), 0);
    printf("User connected!\n");
    while (1) {
        //receive command
        bufferSize = 0;
        memset(buffer, '0', sizeof(char) * 2048);
        bufferSize = recv(sid, buffer, sizeof(buffer), 0); //get all content
        if (bufferSize  <= 0) {
	    printf("\nError: Receive failed\n");
            socketID[tid] = -1; //set array to NULL
            break;
        }//if
        buffer[bufferSize - 1] = '\0';
        command = strtok(buffer, " .\n\0");
        if(strcmp(command, "message") == 0) { // this checks the command, right?
            content = strtok(NULL, "\\");
            name = strtok(NULL, "\n\0");
            //put buffer into global buffer
            // lock the mutex
            pthread_mutex_lock(&global_lock);
            for (i = 0; i < 4; i++) {
                if (socketID[i] == -1)
                    continue;
                memset(globalBuffer, '0', sizeof(globalBuffer));
                strcpy(globalBuffer, name);
                strcat(globalBuffer, ": ");
                strcat(globalBuffer, content);
                send(socketID[i], globalBuffer, sizeof(globalBuffer), 0);    
            }//for
            // unlock the mutex
            pthread_mutex_unlock(&global_lock);
        }//if
        
        if (strcmp(command, "put") == 0) // check for put
        {
            content = strtok(NULL, "\\"); //content is now the file name
            name = strtok(NULL, "\n\0");
            
            //put buffer into global buffer
            // lock the mutex
            
            pthread_mutex_lock(&global_lock);
            //expect file contents
            bufferSize = recv(sid, globalFileBuffer, sizeof(globalFileBuffer), 0);
            puts(globalFileBuffer);
            strcat(sendFile, "\\");
            strcat(sendFile, content);
            strcat(sendFile, "*");
            strcat(sendFile, globalFileBuffer);
            for (i = 0; i < 4; i++) {
                if (socketID[i] == -1)
                    continue;
                send(socketID[i], sendFile, sizeof(sendFile), 0);
            }//for
            // unlock the mutex
            pthread_mutex_unlock(&global_lock);
        }// else if
    }//while
    
    pthread_exit(NULL); //exit the pthread at end of function execution
}

int main() {
    long tid; //thread ID
    int rc; //error handling int
    pthread_t threads[NUM_THREADS]; //array that holds all the pthreads
    char buffer[2048];
    int bufferSize;
    
    /*socket stuff - Guthrie*/
    struct addrinfo hints, *p, *sinfo;
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    
    //put all the stuff into sinfo
    if (getaddrinfo(NULL, "60002", &hints, &sinfo) != 0) {
        printf("\nError: getaddrinfo\n");
        return 1;
    }//if
    
    //loop through results that sinfo got from getaddrinfo
    for (p = sinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            printf("\nError: socket\n");
            continue;
        }//if
        
        int i = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) == -1) {
            printf("\nError: setsockopt\n");
            return 1;
        }//if
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd); //don't need anymore because it broken
            printf("\nError: Bind failed\n");
            continue;
        }//if
        
        break;
    }//for
    
    freeaddrinfo(sinfo); // all done with this guy
    
    if (p == NULL) {
        printf("\nError: Bind failed\n");
        return 1;
    }//if
    
    if (listen(sockfd, 4) == -1) {
        printf("\nError: Listen failed\n");
        return 1;
    }//if
    
    tid = 0; //pthread ids
    int i = 0;
    //initialize socketID array to NULL
    for (i = 0; i < 4; i++) {
        socketID[i] = -1;
    }//for
    while(1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsockfd == -1) {
            printf("\nError: Accepting failed");
            continue;
        }//if
        
        printf("\nConnection Successful!\n");
        rc = pthread_create(&threads[tid], NULL, msgServer, (void *) tid);
        socketID[tid] = newsockfd;
        tid++;
    }//while
    
    pthread_exit(NULL); //exit with pthreads
    return 0; //close main just in case
}
