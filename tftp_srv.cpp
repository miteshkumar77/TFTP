#include "tftp_srv.h"
#define RRQ_ID 1
#define WRQ_ID 2
#define DATA_ID 3
#define ACK_ID 4
#define ERROR_ID 5

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
    unsigned short opcode = ntohs(*opcode_ptr);
    switch(opcode) {
        case RRQ_ID :
            RRQ();
            break;
        case WRQ_ID :
            WRQ();
            break;
        case DATA_ID :
            DATA();
            break;
        case ACK_ID :
            ACK();
            break;
        case ERROR_ID :
            ERROR();
            break;
        default :
            UNKNOWN();
    }
    exit(EXIT_SUCCESS);
}

void tftp_session::send_error(unsigned short ec,
    std::string const& msg)
{
    unsigned short* opcode_ptr = (unsigned short*)mesg;
    *opcode_ptr = htons(ERROR_ID);
    strcpy(mesg + 2, msg.c_str());
    mesg_len = msg.length() + 3;
    Sendto(sockfd, mesg, mesg_len, 0, pcliaddr, len);
}

void tftp_session::RRQ()
{
    filename = std::string(mesg + 2);
    std::string mode = std::string(mesg + 3 + filename.length());
    if (mode != "octet") {
        send_error(0, "Only support octet.");
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