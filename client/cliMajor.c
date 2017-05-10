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

//found we needed this defined for the connect to not spout an error - Guthire
#define h_addr h_addr_list[0] /* for backward compatibility */

//function to write line by line what the file from server says
void fileWrite (char *fileName, char *text) {
    FILE *fp;
    fp = fopen(fileName, "w"); //open file for writing
    fputs(text, fp);
    fclose(fp); //always close file after writing!
    puts("file received!");
    return;
}//fileWrite

void *getMessages(void *socketID) {
    int sid, bufferSize, i = 0;
    char buffer[2048], name[2048];
    char *fileName, *content;
    sid = (long)socketID;
    memset(name, '0', sizeof(char) * 2048);
    gethostname(name, sizeof(name));
    while (1) {
        memset(buffer, '0', sizeof(buffer));
        bufferSize = recv(sid, buffer, sizeof(buffer), 0);
        if (bufferSize <= 0) {
            pthread_exit(NULL);
        }//if
        
        else if (buffer[0] == '\\') {
            fileName = strtok(buffer, "*");
            content = strtok(NULL, "\0");
            fileName++;
            
            //write to file
            fileWrite(fileName, content);
        }//else if
        else {
            buffer[bufferSize] = '\0';
            puts(buffer);
        }//else
    }//while
}//getMessages

int main() {
    //socket stuff
    FILE *fp;
    long tid = 0;
    int sockfd, n, rc;
    pthread_t thread;
    char *portno;
    struct sockaddr_in serv_addr;
    
    struct addrinfo hints, *sinfo, *p;
    
    struct hostent *server;
    
    //client input variables
    char input[1024], content[1024], name[2048], fileRead[1024], fileBuffer[4098]; //input from client
    char *command, *fileName; //parsed from input
    
    // we might need multiple for each thread considering each socket
    // can only have one connecion; so check on this part
    portno = "60002"; // connecting to port 60001 bc I like it - Cassidy
    // socket point creation
    memset(&hints, 0, sizeof(hints)); //initialize hints
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo("cse06.cse.unt.edu", portno, &hints, &sinfo) != 0) {
        printf("\nError: getaddrinfo failed\n");
        return 1;
    }//if
    
    //loop through sinfo because it has a bunch of possible connections
    for (p = sinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            printf("\nError: Socket creation failed\n");
            continue;
        }//if
        
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            printf("\nError: connect failed\n");
            continue;
        }//if
        
        break;
    }//for
    
    if (p == NULL) {
        printf("\nError: Connection failed\n");
        return 1;
    }//if
     
    printf("\nConnection Successful!\n");
    //prior to infinite loop, create pthread for receiving messages
    rc = pthread_create(&thread, NULL, getMessages, (void *) (long)sockfd);
    //Client goes into loop after connecting to server
    while(1) {
        fgets(input, 1024, stdin); //get input from stdin
        strcpy(content, input);
        //check if input is empty
        if (strcmp(input, "\n\0") == 0) {
            printf("\nError: Incorrect Usage\n");
            continue;
        }//if
        
        //set command equal to input up until space, period, newline, or null
        command = strtok(input, " .\n\0");
        
        //test command options (can't do switch because it's a string!)
        if (strcmp(command, "quit") == 0) {
            printf("\nQuit: Closing client\n");
            return 0;
        }//if
        
        else if (strncmp(command, "message", 7) == 0) {
            gethostname(name, sizeof(name));
            content[strlen(content) - 1] = '\\';
            strcat(content, name);
            send(sockfd, content, sizeof(content), 0);
            //get message content from input buffer
            //content = strtok(NULL, "\n\0"); //parses content from end of "command"
            //send(sockfd, content, sizeof(content), 0); //send the message
            continue;
        }//elseif
        
        else if (strcmp(command, "put") == 0) {
            gethostname(name, sizeof(name));
            content[strlen(content) - 1] = '\\';
            strcat(content, name);
            send(sockfd, content, sizeof(content), 0);
            
            // open the file
            fileName = strtok(content, " ");
            fileName = strtok(NULL, "\\");
            fp = fopen(fileName, "r");
            // start putting the file contents into the buffer
            while (fgets(fileRead, sizeof(fileRead), fp)) {
                strcat(fileBuffer, fileRead);
            }//while
            send(sockfd, fileBuffer, sizeof(fileBuffer), 0); //send it
            continue;
        }//elseif
        
        else if (strcmp(command, "help") == 0) {
            printf("\nUsage:\n1. message [content]\n2. put [file name]\n3. help\n4. quit\n");
            continue;
        }//elseif
        
        else {
            printf("\nError: Incorrect command\nType \"help\" for Usage\n");
            continue;
        }//else
        
    }//while    
    return 0;
}