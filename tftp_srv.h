#ifndef TFTP_SRV_H
#define TFTP_SRV_H 

#include <string>
#include <iostream>
#include <array>
#include <functional>
#include <fstream>
#include <algorithm>
// #define LIBDIR "../unpv13e/lib/unp.h"
#define LIBDIR "unp.h"
extern "C" {
    #include LIBDIR 
}

#define RRQ_ID 1
#define WRQ_ID 2
#define DATA_ID 3
#define ACK_ID 4
#define ERROR_ID 5

#define SRESEND 1
#define STERM 10

namespace tftp_server {

// function to read from the udp endpoint
// after every fixed duration of inactivity, retransmit 
// what was last sent by sender
typedef std::function<int(char [MAXLINE])> receiver_t;

// function to send a packet from the udp endpoint
// save a copy of this buffer externally so that it 
// can be retransmitted after a timeout
typedef std::function<void(char [MAXLINE], int)> sender_t;


static unsigned short get_opcode(char mesg[MAXLINE]) {
    unsigned short* opcode_ptr = (unsigned short*)mesg;
    return ntohs(*opcode_ptr);
}

static unsigned short get_block(char mesg[MAXLINE]) {
    unsigned short* block_ptr = ((unsigned short*)mesg)+1;
    return ntohs(*block_ptr);
}

static void set_opcode(char mesg[MAXLINE], unsigned short opcode) {
    unsigned short* opcode_ptr = (unsigned short*)mesg;
    *opcode_ptr = htons(opcode);
}

static void set_block(char mesg[MAXLINE], unsigned short block) {
    unsigned short* block_ptr = ((unsigned short*)mesg) + 1;
    *block_ptr = htons(block);
}

static void set_ec(char mesg[MAXLINE], unsigned short ec) {
    unsigned short* ec_ptr = ((unsigned short*)mesg) + 1;
    *ec_ptr = htons(ec);
}

static unsigned short get_ec(char mesg[MAXLINE]) {
    unsigned short* ec_ptr = ((unsigned short*)mesg) + 1;
    return ntohs(*ec_ptr);
}

static std::string get_ErrMsg(char mesg[MAXLINE]) {
    char* msg_ptr = ((char*)mesg) + 4;
    return std::string(msg_ptr);
}

template<typename T>
class tftp_sesh{
public:
    // provide a application layer receiver and sender API to handle
    // sending/timeout/resending of packets, as well as the starting buffer
    // contents.
    tftp_sesh(receiver_t const& receiver, sender_t const& sender, char buffer[MAXLINE]) :
        receiver(receiver), sender(sender) {
        memcpy(mesg, buffer, MAXLINE);
    }
    
    // accept_message gets called after each event handler.
    // Blocks on udp receive.
    // It calls the event handler that corresponds to the 
    // opcode in the received packet
    // The handlers tail-recursively call accept_message if 
    // the the transaction continues, otherwise the original
    // call terminates.
    void accept_message() {
        std::cerr << "Blocked on recv" << std::endl;
        mesg_len = receiver(mesg);
        auto opcode = get_opcode(mesg);
        std::cout << "Got opcode: " << opcode << std::endl;
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
        std::cerr << "Done with transaction, exiting child process..." << std::endl;
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
        set_opcode(deliver, ERROR_ID);
        set_ec(deliver, ec);
        strcpy(deliver + 4, msg.c_str());
        int deliver_len = msg.length() + 5;
        sender(deliver, deliver_len);
    }

    unsigned short curr_block{0};
    int mesg_len;
    char mesg[MAXLINE];
    char deliver[MAXLINE];
    receiver_t const &receiver;
    sender_t const &sender;

    std::string filename;
};

// tftp RRQ session
class tftp_reader : public tftp_sesh<tftp_reader> {
public:

    // handle an RRQ packet in an RRQ session
    bool handle_RRQ() {
        // start by sending the first block
        curr_block = 1;
        filename = std::string(mesg + 2);
        std::string mode = std::string(mesg + 3 + filename.length());
        if (mode != "octet") {
            send_error(0, "Invalid mode");
            return false;
        }
        std::cerr << "Ready to read file: " << filename << " in octet mode" << std::endl;

        // the RRQ handler is responsible for sending the first
        // data packet.
        int num_sent = send_data();
        return true;
    }

    // A WRQ packet is an invalid TFTP request within an RRQ session
    bool handle_WRQ() {
        send_error(0, "Sent WRQ in a RRQ.");
        return false;
    }

    // A DATA packet is an invalid TFTP request within an RRQ session
    bool handle_DATA() {
        send_error(0, "Sent DATA in a RRQ");
        return false;
    }

    bool handle_ACK() {
        unsigned short* block_ptr = ((unsigned short*)mesg) + 1;
        unsigned short block = ntohs(*block_ptr);
        std::cerr << "Got Acknowledgement for block " << block << " while curr_block = " << curr_block << std::endl;
        if (block < curr_block) { // Avoid SAS
            // We already got an acknowledgement
            // for a block higher than this, so we 
            // don't need to send this packet
            return true;
        }
        ++curr_block;
        return send_data() > 0;
    }

    // Handle error packet and exit.
    void handle_ERROR() {
        unsigned short ec = get_ec(mesg);
        std::string const err_msg = get_ErrMsg(mesg);
        std::cerr << "RRQ client sent Error Code " << ec << ": "
        << err_msg << std::endl;
    }

    void handle_UNKNOWN() {
        std::cerr << "Unknown op code" << std::endl;
    }

    // Send the data block associated with the current 512 byte (at most) block 
    // in our file
    int send_data() {
        std::cerr << "Reading bytes from file: " << filename << std::endl;
        unsigned short* opcode_ptr = (unsigned short*)deliver;
        *opcode_ptr = htons(DATA_ID);
        unsigned short* block_ptr = opcode_ptr + 1;
        *block_ptr = htons(curr_block);
        std::ifstream input_file{filename, std::ios_base::binary};
        if (!input_file.good()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            send_error(1, "Unable to open file");
            return 0;
        }

        input_file.seekg((curr_block - 1) * 512);
        input_file.read(deliver + 4, 512);
        int nread = input_file.gcount();
        input_file.close();
        sender(deliver, 4 + nread);
        std::cerr << "Read " << nread << " bytes from file: " << filename << std::endl;
        if (nread < 512) {
            std::cerr << "Done reading file: " << filename << std::endl;
        }
    
        return nread;
    }


};

// tftp WRQ session
class tftp_writer: public tftp_sesh<tftp_writer> {
public:

    // RRQ is invalid within a WRQ session
    bool handle_RRQ() {
        send_error(0, "Sent RRQ in a WRQ.");
        return false;
    }

    // Start a WRQ
    bool handle_WRQ() {
        filename = std::string(mesg + 2);
        std::string mode = std::string(mesg + 3 + filename.length());
        if (mode != "octet") {
            send_error(0, "Invalid mode");
            return false;
        }
        std::ofstream output_file{filename, std::ios_base::binary |
            std::ios_base::trunc};
        if (!output_file.good()) {
            send_error(1, "Error opening file");
            return false;
        }
        output_file.close();
        std::cerr << "Ready to write file: " << filename << " in octet mode" << std::endl;
        send_ACK();
        curr_block = 1;
        return true;
    }

    bool handle_DATA() {
        unsigned short* block_ptr = ((unsigned short*)mesg) + 1;
        unsigned short block = ntohs(*block_ptr);
        if (block == curr_block) {
            std::ofstream output_file{filename, std::ios_base::binary 
                | std::ios_base::app};
            if (!output_file.good()) {
                send_error(1, "Error opening file");
                return false;
            }
            output_file.write((char*)(block_ptr + 1), mesg_len-4);
            output_file.close();
            ++curr_block;
        }
        std::swap(curr_block, block);
        send_ACK();
        std::swap(curr_block, block);
        return mesg_len == 516;
    }

    bool handle_ACK() {
        send_error(0, "Sent ACK in a WRQ.");
        return false;
    }

    void handle_ERROR() {
        unsigned short ec = get_ec(mesg);
        std::string const err_msg = get_ErrMsg(mesg);
        std::cerr << "WRQ client sent Error Code " << ec << ": "
        << err_msg << std::endl;
    }

    void handle_UNKNOWN() {
        std::cerr << "Unknown op code" << std::endl;
    }

    void send_ACK() {
        unsigned short* opcode_ptr = (unsigned short*) deliver;
        unsigned short* block_ptr = opcode_ptr + 1;
        *opcode_ptr = htons(ACK_ID);
        *block_ptr = htons(curr_block);
        std::cerr << "Sending ACK for block: " << curr_block << std::endl;
        sender(deliver, 4);
    }

};

};

#endif