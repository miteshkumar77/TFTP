## Network Programming Assignment 1

Mitesh Kumar, Jason Lam

## Usage

1. `make`
2. `./tftp.out 60341 60441`

## Testing

1. `tftp`
2. `connect 127.0.0.1 60341`
3. `mode octet`
4. `verbose`
5. `trace`
6. `get <filename>` or `put <filename>`

## Overall Description

The TFTP server uses UDP Datagrams to read and write files via a connection between a client and server.

Program Workflow:
1. Read or write request is received on the starting port
2. Child process is created to communicate with the client
3. Opcode, filename, and modename are extracted from the packet
4. If the packet is a read request: send the first data packet, and wait until an acknowledgement is received, then send next data packet. Repeat until everything in file is sent.
5. If the packet is a write request: send an acknowledgement, wait until data packet is received, send back an acknowledgement, and repeat until data packet with fewer than 512 bytes is sent.

## Issues Encountered

- Faced some challenges with the "multi-client" test on Submitty.
- Figuring out a way to elegantly handle empty files, as well as handling SAS.

## Team work
Time spent: 7 hrs total between team members

Mitesh: Working application of assignment
Jason: Testing, readme, comments
