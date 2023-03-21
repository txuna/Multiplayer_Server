#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <ctime>


#define PORT 9999

#define SOCKET_BUFFER_SIZE 1024
#define TICKS_PER_SECOND 60
#define SECONDS_PER_TICK 1.0f / float(TICKS_PER_SECOND)
#define MAX_CLIENT 64
#define UNUSED -1 

#define ERROR_READ 0 
#define CAN_READ 1 
#define NOT_YET_READ 2

#define FULL_CLIENT 1

#define SPEED 300

enum class ServerMessage : char{
    State, 
    AssignId, 
    JoinPlayer, 
    LeavePlayer,
    SetOtherPlayerPosition
}; 

enum class ClientMessage : char{
    Input,
    Position
};

class Net
{
    public:
        struct sockaddr_in addr; 
        socklen_t addr_len;
        virtual bool init_socket() = 0; 
        virtual bool bind_socket() = 0; 
        virtual bool listen_socket() = 0; 
        virtual int socket_read(char* buffer, int* received_bytes) = 0; 
        virtual bool socket_send(char* buffer, int send_bytes) = 0; 
};

class Socket : public Net
{
    private: 
        int fd = 0; 

    public:
        Socket();
        virtual bool init_socket(); 
        virtual bool bind_socket(); 
        virtual bool listen_socket(); 
        /* 자신의 소켓에 대해서 읽거나 전송*/
        virtual int socket_read(char* buffer, int* received_bytes); 
        virtual bool socket_send(char* buffer, int send_bytes); 
        Socket accept_socket();
        int get_fd();
        void set_fd(int fd);
};

struct ClientInput{
    float x, y; 
}; 

struct Velocity{
    float x, y;
};

class Client
{
    public:
        Client(Socket socket){
            this->socket = socket; 
        };
        Client() {
            this->client_input = {};
            this->velocity = {};
            this->slot = UNUSED;
        }; 
        int slot;
        Socket socket;         
        struct ClientInput client_input;
        struct Velocity velocity; 
        struct Velocity position; 
};


class EpollState
{
    private:
        int epoll_fd; 
        struct epoll_event events[MAX_CLIENT];
        int timeout = 0;

    public:
        bool init_epoll(); 
        bool add_epoll(int fd, int events); 
        bool del_epoll(int fd); 
        int wait_epoll();
        struct epoll_event* get_events(); 
};


class TcpState
{
    private:
        
    public:
        TcpState();
        Client clients[MAX_CLIENT];
        Socket server_socket;
        Client* add_client();
        bool del_client(int fd); 
        Client get_client(int fd);
        void debug(); 
};
