#include "common.h"


Socket::Socket()
{
    memset(&addr, 0, sizeof(addr));
    addr_len = sizeof(addr);
}

bool Socket::init_socket()
{
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(fd < 0)
    {
        std::cout<<"socket() Failed: "<<errno<<std::endl;
        return false; 
    }
    std::cout<<"init socket"<<std::endl;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; 
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    int opt = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) < 0)
    {
        std::cout<<"setsockopt() Failed: "<<errno<<std::endl;
        return false;
    }
    //addr_len = sizeof(addr); 
    return true;
}


bool Socket::bind_socket()
{
    if(bind(fd, (struct sockaddr*)&addr, addr_len) == -1)
    {
        std::cout<<"bind() Failed"<<errno<<std::endl;
        return false;
    }
    std::cout<<"bind socket"<<std::endl;
    return true;
}

bool Socket::listen_socket()
{
    if(listen(fd, 10) == -1)
    {
        std::cout<<"listen() Failed"<<errno<<std::endl;
        return false;
    }
    std::cout<<"listen socket"<<std::endl;
    return true;
}

Socket Socket::accept_socket()
{
    Socket socket;
    int _fd = accept(fd, (struct sockaddr*)&socket.addr, &socket.addr_len);
    socket.set_fd(_fd);

    return socket;
}

int Socket::socket_read(char* buffer, int* received_bytes)
{
    ssize_t recv_ret = recv(fd, buffer, SOCKET_BUFFER_SIZE, 0);
    /* close the connection */
    if(recv_ret == 0){
        return ERROR_READ; 
    }
    else if(recv_ret < 0){
        int err = errno; 
        /* 현재 읽을 데이터가 없기때문에 return하고 트리거가 되면 다시 호출 */
        if(err == EAGAIN){
            return NOT_YET_READ; 
        }
        return ERROR_READ;
    }
    *received_bytes = recv_ret;
    buffer[recv_ret] = '\0'; 
    return CAN_READ;
}

bool Socket::socket_send(char* buffer, int send_bytes)
{
    if(send(fd, buffer, send_bytes, 0) < 0)
    {
        return false; 
    }
    return true;
}

int Socket::get_fd()
{
    return fd; 
}

void Socket::set_fd(int fd)
{   
    this->fd = fd;
}