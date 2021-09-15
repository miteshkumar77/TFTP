#ifndef TFTP_CLI_SRV_H
#define TFTP_CLI_SRV_H

#include <string>
#include <iostream>
#include <array>
#include <functional>

extern "C" {
    #include "../unpv13e/lib/unp.h"
}

namespace tftp_server {

class tftp_session {
public:
    tftp_session(int , SA *, socklen_t);
    void accept_message(bool);

private:

    void RRQ();
    void WRQ();
    void DATA();
    void ACK();
    void ERROR();
    void UNKNOWN();

    int curr_block{0};

    int mesg_len;
    char mesg[MAXLINE];
    socklen_t len;



    std::string filename;
    int const sockfd;
    SA * pcliaddr;
    socklen_t const clilen;
};

};

#endif