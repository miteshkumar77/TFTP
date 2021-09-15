#include "tftp_srv.h"

namespace tftp_server {
tftp_session::tftp_session(int sockfd, 
    SA *pcliaddr, socklen_t clilen) : sockfd(sockfd), 
        pcliaddr(pcliaddr), clilen(clilen)
{
}

void tftp_session::accept_message(bool first)
{
    len = clilen;
    mesg_len = Recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
    if (first) {
        pid_t pid = Fork();
        if (pid > 0) {
            return;
        }
    }

    unsigned short* opcode_ptr = (unsigned short*)mesg;
    switch(ntohs(*opcode_ptr)) {
        case 1 :
            RRQ();
            break;
        case 2 :
            WRQ();
            break;
        case 3 :
            DATA();
            break;
        case 4 :
            ACK();
            break;
        case 5 :
            ERROR();
            break;
        default :
            UNKNOWN();
    }
    exit(EXIT_SUCCESS);
}

void tftp_session::RRQ()
{
    filename = std::string(mesg + 2);
    std::string mode = std::string(mesg + 3 + filename.length());
    if (mode != "octet") {
        return;
    }

    accept_message(false);
}

void tftp_session::WRQ()
{

    accept_message(false);
}

void tftp_session::DATA()
{

    accept_message(false);
}

void tftp_session::ACK()
{
    accept_message(false);
}

void tftp_session::ERROR()
{

}

void tftp_session::UNKNOWN()
{

}

};