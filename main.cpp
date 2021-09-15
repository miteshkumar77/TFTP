#include "tftp_srv.h"
/*

dg_echo(sockfd, (SA *) &cliaddr, sizeof(cliaddr));

void
dg_echo(int sockfd, SA *pcliaddr, socklen_t clilen)
{
        int                     n;
        socklen_t       len;
        char            mesg[MAXLINE];
 
        for ( ; ; ) {
                len = clilen;
                n = Recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
 
                Sendto(sockfd, mesg, n, 0, pcliaddr, len);
        }
}


*/


int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "USAGE: ./" << argv[0] << 
            " [start of port range] [end of port range]" << 
            std::endl;
        exit(EXIT_FAILURE);
    }

    int start_port = std::stoi(std::string(argv[1]));
    int end_port = std::stoi(std::string(argv[2]));

    for (int curr_port = start_port; 
        curr_port <= end_port; ++curr_port) {
        int                                     sockfd;
        struct sockaddr_in      servaddr, cliaddr;
 
        sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
 
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port        = htons(curr_port);
 
        Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
        
        int n;
        socklen_t len;
        char mesg[MAXLINE];

        //start_conn(sockfd, (SA *) &cliaddr, sizeof(cliaddr));
        tftp_server::tftp_session tftp(sockfd, (SA *) &cliaddr, sizeof(cliaddr));
        tftp.accept_message(true);
    }

    return EXIT_SUCCESS;
}