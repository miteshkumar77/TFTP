// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
  
#define PORT     8080
#define MAXLINE 1024
  
// Driver code
int main() {
    int sockfd;
    char buffer[MAXLINE];
    char* mode = "netascii";
    char* filename = "bad_fname";
    unsigned short* opcode_ptr = (unsigned short*)buffer;
    *opcode_ptr = htons(1);
    char* str_ptr = buffer + 2;
    strcpy(str_ptr, filename);
    str_ptr += strlen(filename) + 1;
    strcpy(str_ptr, mode);
    str_ptr += strlen(mode) + 1;
    int MSG_LEN = str_ptr - buffer;

    // printf("Total msg len: %d\n", MSG_LEN);
    struct sockaddr_in     servaddr;
  
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
  
    memset(&servaddr, 0, sizeof(servaddr));
      
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
      
    int n, len;
      
    sendto(sockfd, (const char *)buffer, MSG_LEN,
        MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
            sizeof(servaddr));
    printf("Hello message sent.\n");
          
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 
                MSG_WAITALL, (struct sockaddr *) &servaddr,
                &len);
    buffer[n] = '\0';
    printf("Server sent %d bytes: %s\n", n, buffer);
  
    close(sockfd);
    return 0;
}
