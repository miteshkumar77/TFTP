#ifndef TFTP_CLI_SRV_H
#define TFTP_CLI_SRV_H

#include <string>
#include <iostream>
#include <array>
#include <functional>
#include <fstream>

extern "C" {
    #include "../unpv13e/lib/unp.h"
}

#define RRQ_ID 1
#define WRQ_ID 2
#define DATA_ID 3
#define ACK_ID 4
#define ERROR_ID 5

namespace tftp_server {

typedef std::function<int(char [MAXLINE])> receiver_t;
typedef std::function<void(char [MAXLINE], int)> sender_t;

static unsigned short get_opcode(char mesg[MAXLINE]) {
    unsigned short* opcode_ptr = (unsigned short*)mesg;
    return ntohs(*opcode_ptr);
}

template<typename T>
class tftp_sesh{
public:
    tftp_sesh(receiver_t const& receiver, sender_t const& sender, char buffer[MAXLINE]) : 
        receiver(receiver), sender(sender) {
        memcpy(mesg, buffer, MAXLINE);
    }
    
    void accept_message() {
        mesg_len = receiver(mesg);
        auto opcode = get_opcode(mesg);
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

    void RRQ() {
        T& u = static_cast<T&>(*this);
        if (u.handle_RRQ()) {
            accept_message();
        }
    }

    void WRQ() {
        T& u = static_cast<T&>(*this);
        if (u.handle_WRQ()) {
            accept_message();
        }
    }

    void DATA() {
        T& u = static_cast<T&>(*this);
        if (u.handle_DATA()) {
            accept_message();
        }
    }

    void ACK() {
        T& u = static_cast<T&>(*this);
        if (u.handle_ACK()) {
            accept_message();
        }
    }

    void ERROR() {
        T& u = static_cast<T&>(*this);
        u.handle_ERROR();
    }

    void UNKNOWN() {
        T& u = static_cast<T&>(*this);
        u.handle_UNKNOWN();
    }

    void send_error(unsigned short ec, std::string const& msg) {
        unsigned short* opcode_ptr = (unsigned short*)mesg;
        *opcode_ptr = htons(ERROR_ID);
        strcpy(mesg + 2, msg.c_str());
        mesg_len = msg.length() + 3;
        sender(mesg, mesg_len);
    }

    int mesg_len;
    char mesg[MAXLINE];
    receiver_t const &receiver;
    sender_t const &sender;
};

class tftp_reader : public tftp_sesh<tftp_reader> {
public:
    bool handle_RRQ() {
        return false;
    }

    bool handle_WRQ() {
        return false;
    }

    bool handle_DATA() {
        return false;
    }

    bool handle_ACK() {
        return false;
    }

    void handle_ERROR() {
    }

    void handle_UNKNOWN() {
    }

};


class tftp_writer: public tftp_sesh<tftp_writer> {
public:
    bool handle_RRQ() {
        return false;
    }

    bool handle_WRQ() {
        return false;
    }

    bool handle_DATA() {
        return false;
    }

    bool handle_ACK() {
        return false;
    }

    void handle_ERROR() {
    }

    void handle_UNKNOWN() {
    }

};

class tftp_session{
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

    void send_data() const;
    void send_ack() const;
    void send_error(unsigned short, std::string const&);

    int curr_block{0};
    std::ifstream file_input;
    std::ofstream file_output;

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