#include "common.h"

TcpState::TcpState()
{
    for(int i=0; i<MAX_CLIENT; i++)
    {
        Client client = Client(); 
        client.slot = UNUSED;
        clients[i] = client; 
    }
}

// flag가 false일시 메시지 보내고 close()
Client* TcpState::add_client()
{
    for(int i=0; i<MAX_CLIENT; i++)
    {
        if(clients[i].slot == UNUSED)
        {
            Socket socket = server_socket.accept_socket();
            clients[i].slot = i;
            clients[i].socket = socket;;
            return &clients[i];
        }
    }
    /* accept 요청 온것을 거부할 수 없기때문에 요청 승인 후 close */
    Socket socket = server_socket.accept_socket();
    close(socket.get_fd());
    throw socket.get_fd();
}

void TcpState::debug()
{
    for(int i=0;i<MAX_CLIENT;i++)
    {
        if(clients[i].slot != UNUSED)
        {
            std::cout<<clients[i].socket.get_fd()<<": "<<clients[i].slot<<std::endl;
        }
    }
    return;
}

// 해당 커넥션 close() 
bool TcpState::del_client(int fd)
{
    bool flag = false;
    for(int i=0;i<MAX_CLIENT; i++)
    {
        Socket socket = clients[i].socket;
        if(socket.get_fd() == fd)
        {
            clients[i].slot = UNUSED;
            close(socket.get_fd());
            flag = true;
        }
    }
    return flag;
}


Client TcpState::get_client(int fd)
{
    for(int i=0;i<MAX_CLIENT; i++)
    {
        Socket socket = clients[i].socket;
        if(socket.get_fd() == fd)
        {
            return clients[i];
        }
    }
    throw fd;
}


bool EpollState::init_epoll()
{
    epoll_fd = epoll_create(MAX_CLIENT); 
    if(epoll_fd < 0)
    {
        return false;
    }
    return true;
}

bool EpollState::add_epoll(int fd, int events)
{
    struct epoll_event event; 
    memset(&event, 0, sizeof(struct epoll_event)); 

    event.events = events; 
    event.data.fd = fd; 
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        return false; 
    }
    return true;
}

bool EpollState::del_epoll(int fd)
{
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0)
    {
        return false; 
    }
    return true;
}

// epoll_ret = epoll_wait(tcp_state.epoll_fd, events, MAX_CLIENT, timeout);
int EpollState::wait_epoll()
{
    return epoll_wait(epoll_fd, events, MAX_CLIENT, timeout);
}

struct epoll_event* EpollState::get_events()
{
    return events;
}