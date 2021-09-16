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



int N_children{0};
int N_term{0};

void sigchld_handler(int signum) {
    int wstatus;
    pid_t pid;

    while((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
        if (pid == -1) {
            perror("ERROR: waitpid failed");
            exit(EXIT_FAILURE);
        }
        ++N_term;
    }
}

void start_conn(int sockfd, SA *pcliaddr, socklen_t clilen)
{
    int n;
    socklen_t len;
    char mesg[MAXLINE];
    len = clilen;
    n = Recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);

    SA og_cli = *pcliaddr;
    socklen_t og_len = len;

    pid_t pid = Fork();
    if (pid == 0) {

        char prev_send[MAXLINE];
        int prev_nbytes{0};

        tftp_server::receiver_t receiver = [&](char d[MAXLINE]) -> int {
            return Recvfrom(sockfd, d, MAXLINE, 0, pcliaddr, &len);
        };

        tftp_server::sender_t sender = [&](char d[MAXLINE], int d_len) {
            std::cerr << "Sending " << d_len << " bytes" << std::endl;
            Sendto(sockfd, d, d_len, 0, &og_cli, og_len);
        };
        unsigned short opcode = tftp_server::get_opcode(mesg);
        if (opcode == RRQ_ID) {
            tftp_server::tftp_sesh<tftp_server::tftp_reader>
                sesh(receiver, sender, mesg);
                sesh.RRQ();
        } else if (opcode == WRQ_ID) {
            tftp_server::tftp_sesh<tftp_server::tftp_writer>
                sesh(receiver, sender, mesg);
                sesh.WRQ();
        } else {
            tftp_server::tftp_sesh<tftp_server::tftp_reader>
                sesh(receiver, sender, mesg);
            sesh.send_error(0, "Invalid request");
        }
        exit(EXIT_SUCCESS);
    }
    ++N_children;
}


int main(int argc, char** argv) {
    Signal(SIGCHLD, &sigchld_handler);
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
        start_conn(sockfd, (SA *) &cliaddr, sizeof(cliaddr));
    }

    while(N_term < N_children) usleep(10);
    return EXIT_SUCCESS;
}