#include "tftp_srv.h"

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

void sigalrm_handler(int signum) {
}

int createEndpoint(int port, struct sockaddr_in& servaddr, in_addr_t in_addr) {
    bzero(&servaddr, sizeof(servaddr));
    int sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = in_addr; 
    servaddr.sin_port = htons(port);
    Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
    return sockfd;
}

void start_conn(int sockfd, SA *pcliaddr, socklen_t clilen, int tid)
{
    int n;
    socklen_t len;

    char mesg[MAXLINE];
    len = clilen;
    n = Recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);



    pid_t pid = Fork();
    if (pid == 0) {
        SA og_cli = *pcliaddr;
        socklen_t og_len = len;
        struct sockaddr_in servaddr;
        Close(sockfd);
        sockfd = createEndpoint(tid, servaddr, htonl(INADDR_ANY));

// ((sockaddr_in*)pcliaddr)->sin_addr.s_addr
        char prev_send[MAXLINE];
        int prev_nbytes{0};
        
        Signal(SIGALRM, &sigalrm_handler);

        tftp_server::sender_t sender = [&](char d[MAXLINE], int d_len) {
            memcpy(prev_send, d, MAXLINE);
            prev_nbytes = d_len;
            std::cerr << "Sending " << d_len << " bytes" << std::endl;
            Sendto(sockfd, d, d_len, 0, &og_cli, og_len);
        };

        // sender should always be called atleast once before
        // receiver is called, so that prev_send, prev_nbytes have values
        // otherwise if there is a timeout, receiver will not have any
        // data to retransmit.
        tftp_server::receiver_t receiver = [&](char d[MAXLINE]) -> int {
            int iters = 0;
            int nrecv_receiver;
            do {
                alarm(SRESEND);
                nrecv_receiver = Recvfrom(sockfd, d, MAXLINE, 0, pcliaddr, &len);
                alarm(0);
                ++iters;
                if (nrecv_receiver == -1) {
                    if (errno != EINTR) {
                        perror("ERROR: Read from udp socket failed. Exiting...\n");
                        exit(EXIT_FAILURE);
                    } else if (iters <= STERM && prev_nbytes != 0) {
                        perror("1s timeout reached. Retransmitting previous packet...\n");
                        sender(prev_send, prev_nbytes);
                    }
                }
            } while(nrecv_receiver == -1 && iters <= STERM);

            if (iters == STERM) {
                std::cerr << "Client timed out. Exiting..." << std::endl;
                exit(EXIT_SUCCESS);
            }
            return nrecv_receiver;
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
    struct sockaddr_in      servaddr;
    int sockfd = createEndpoint(start_port, servaddr, htonl(INADDR_ANY));

    for (int curr_port = start_port+1; 
        curr_port <= end_port; ++curr_port) {
        struct sockaddr_in cliaddr;
        start_conn(sockfd, (SA *) &cliaddr, sizeof(cliaddr), curr_port);
    }
    Close(sockfd);
    while(N_term < N_children) usleep(10);
    return EXIT_SUCCESS;
}