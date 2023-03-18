#include "common.h"

typedef std::chrono::high_resolution_clock time_clock; 

void get_random_position(int* x, int* y)
{
    std::srand(static_cast<unsigned int>(std::time(0)));
    *x = std::rand() % 500;
    *y = std::rand() % 500; 
}

int main(void)
{
    TcpState tcp_state = TcpState();
    EpollState epoll_state = EpollState();
    int epoll_ret; 

    tcp_state.server_socket.init_socket(); 
    tcp_state.server_socket.bind_socket(); 
    tcp_state.server_socket.listen_socket();

    
    if(!epoll_state.init_epoll())
    {
        std::cout<<"epoll_init() Failed: "<<errno<<std::endl;
        return 0;
    }
    /* 서버 TCP 디스크립터 등록 */
    if(!epoll_state.add_epoll(tcp_state.server_socket.get_fd(), EPOLLIN | EPOLLPRI))
    {
        std::cout<<"epoll_ctl() error: "<<errno<<std::endl;
        return 0;
    }

    bool is_running = true; 
    while(is_running)
    {
        time_clock::time_point start = time_clock::now();
        /* CODE BASE */
        epoll_ret = epoll_state.wait_epoll(); 
        if(epoll_ret == -1)
        {
            std::cout<<"epoll_wait() Failed: "<<errno<<std::endl;
            break;
        }
        if(epoll_ret >= 1)
        {
            struct epoll_event* events = epoll_state.get_events();
            /* Event Check */
            for(int i=0; i<epoll_ret; i++)
            {
                int fd = events[i].data.fd; 
                /* Accept new client */
                if(fd == tcp_state.server_socket.get_fd())
                {
                    try
                    {
                        Client client = tcp_state.add_client();
                        epoll_state.add_epoll(client.socket.get_fd(), EPOLLIN | EPOLLPRI);
                        std::cout<<"new connected client "<<client.socket.get_fd()<<std::endl;
                        /* 접속에 성공한 클라이언트에게 ID제공 */
                        /* 
                            Protocol(1byte) - Slot(4byte) - x(4byte) - y(4byte)
                        */
                        int send_bytes = 0;
                        int x, y; 
                        char protocol = (char)ServerMessage::AssignId; 
                        char buffer[SOCKET_BUFFER_SIZE]; 
                        memset(buffer, 0, SOCKET_BUFFER_SIZE);
                        memcpy(&buffer[0], &protocol, sizeof(protocol)); 
                        send_bytes+=sizeof(protocol); 
                        memcpy(&buffer[1], &client.slot, sizeof(client.slot));
                        send_bytes+=sizeof(client.slot);
                        get_random_position(&x, &y); 
                        memcpy(&buffer[send_bytes], &x, sizeof(x)); 
                        send_bytes+=sizeof(x); 
                        memcpy(&buffer[send_bytes], &y, sizeof(y));
                        send_bytes+=sizeof(y);
                        if(!client.socket.socket_send(buffer, send_bytes))
                        {
                            std::cout<<"socket send():Failed"<<std::endl;
                        }   
                        /* 다른 플레이어에게 해당 플레이어 접속 사실 알리기 */
                        /* Protocol(1byte) - Slot(4byte) - x(4byte) - y(4byte) */
                        send_bytes = 0; 
                        memset(buffer, 0, SOCKET_BUFFER_SIZE);
                        buffer[0] = (char)ServerMessage::JoinPlayer; 
                        send_bytes += 1;
                        memcpy(&buffer[send_bytes], &client.slot, sizeof(client.slot)); 
                        send_bytes += sizeof(client.slot);
                        memcpy(&buffer[send_bytes], &x, sizeof(x)); 
                        send_bytes += sizeof(x);
                        memcpy(&buffer[send_bytes], &y, sizeof(y)); 
                        send_bytes += sizeof(y);
                        /* 전송 */
                        for(int i=0; i<MAX_CLIENT; i++)
                        {
                            Client c = tcp_state.clients[i];
                            // 자기자신 제외 
                            if(c.slot == UNUSED || c.socket.get_fd() == client.socket.get_fd()){
                                continue; 
                            }
                            if(!c.socket.socket_send(buffer, send_bytes))
                            {
                                std::cout<<"socket send() Failed: "<<errno<<std::endl;
                            }
                        }
                        continue;
                    }
                    /* 클라이언트 배열에 남은 공간이 없을 시 */
                    catch(int fd)
                    {
                        std::cout<<"Full Client: "<<fd<<std::endl;
                        continue;
                    }
                    
                }
                /* close connection */
                if(events[i].events & (EPOLLERR | EPOLLHUP)){
                    std::cout<<"Client "<<fd<<": "<<"has disconnected"<<std::endl;
                    tcp_state.del_client(fd);
                    epoll_state.del_epoll(fd);
                    continue;
                }
                /* Client Event Handler */
                Client client;
                try
                {
                    client = tcp_state.get_client(fd); //Catch
                }
                catch(int fd)
                {
                    std::cout<<"Not Found Client: "<<fd<<std::endl;
                    continue;
                }
                /* 패킷 읽음 */
                char buffer[SOCKET_BUFFER_SIZE];
                int received_bytes = 0;
                memset(buffer, 0, SOCKET_BUFFER_SIZE); 
                int flag = client.socket.socket_read(buffer, &received_bytes);
                if(flag == NOT_YET_READ)
                {
                    continue; 
                }
                /* 다른 플레이어에게 해당 플레이어 disonnect 사실 알리기 */
                else if(flag == ERROR_READ)
                {
                    std::cout<<"Client "<<client.socket.get_fd()<<": "<<"has disconnected"<<std::endl;
                    tcp_state.del_client(fd);
                    epoll_state.del_epoll(fd);
                    continue;
                } 
                /* CAN_READ */
                /* Read Client Packet And Save Client Input */
                /* 2개 이상 프로토콜을 담을 경우가 있음*/
                int bytes = 0;
                while(bytes < received_bytes)
                {
                    char protocol = buffer[bytes];  
                    bytes+=sizeof(protocol);
                    switch((ClientMessage)protocol)
                    {
                        case ClientMessage::Input:
                        {
                            /* ID와 주소 인증 과정 필요 */
                            /* 입력값들 client_input에 저장 */
                            int recv_slot_id; 
                            float x, y; 
                            memcpy(&recv_slot_id, &buffer[bytes], sizeof(recv_slot_id)); 
                            bytes+=sizeof(recv_slot_id);  
                            memcpy(&x, &buffer[bytes], sizeof(x)); 
                            bytes+=sizeof(x); 
                            memcpy(&y, &buffer[bytes], sizeof(y)); 
                            bytes+=sizeof(y);
                            //std::cout<<"Id:"<<recv_slot_id<<"->"<<"("<<x<<", "<<y<<")"<<std::endl;

                            if(recv_slot_id < 0 && received_bytes >= MAX_CLIENT)
                            {
                                continue; 
                            }
                            Client* client = &tcp_state.clients[recv_slot_id]; 
                            client->client_input.x = x; 
                            client->client_input.y = y;
                            break;
                        }
                        /* 무한 루프 가능성 있음 */
                        default:
                            std::cout<<"Invalid Protocol, Peek Next Byte"<<std::endl;
                            bytes+=1;
                            break; 
                    }
                }
            }
        }
        char numOfplayer = 0; 
        /* Process Client Input */
        for(int i=0; i<MAX_CLIENT; i++)
        {
            Client* client = &tcp_state.clients[i]; 
            if(client->slot == UNUSED)
            {
                continue; 
            }
            numOfplayer+=1; 
            client->velocity.x = client->client_input.x * SPEED; 
            client->velocity.y = client->client_input.y * SPEED; 
            //std::cout<<client->client_input.x<<" "<<client->velocity.x<<std::endl;
        }

        /* Create Game State Packet */
        /*
            State(1byte) numOfplayer(1byte)
            slot(4byte) - x(4byte) - y(4byte)
            slot(4byte) - x(4byte) - y(4byte)
            ...
        */
        //std::cout<<"Game State Update"<<std::endl;
        int send_bytes = 0; 
        char buffer[SOCKET_BUFFER_SIZE]; 
        memset(buffer, 0, SOCKET_BUFFER_SIZE); 
        buffer[0] = (char)ServerMessage::State; 
        send_bytes += 1;
        buffer[1] = numOfplayer; 
        send_bytes += 1;

        for(int i=0; i<MAX_CLIENT; i++)
        {   
            Client client = tcp_state.clients[i];
            if(client.slot == UNUSED)
            {
                continue; 
            }
            memcpy(&buffer[send_bytes], &client.slot, sizeof(client.slot)); 
            send_bytes += sizeof(client.slot);
            memcpy(&buffer[send_bytes], &client.velocity.x, sizeof(client.velocity.x)); 
            send_bytes += sizeof(client.velocity.x); 
            memcpy(&buffer[send_bytes], &client.velocity.y, sizeof(client.velocity.y)); 
            send_bytes += sizeof(client.velocity.y); 
        }

        /* Send State Packet to all client */
        //std::cout<<"Game State Send All Client"<<std::endl;
        for(int i=0; i<MAX_CLIENT; i++)
        {
            Client client = tcp_state.clients[i]; 
            if(client.slot == UNUSED)
            {
                continue; 
            }
            if(!client.socket.socket_send(buffer, send_bytes))
            {
                std::cout<<"socket send() Failed: "<<errno<<std::endl; 
                continue; 
            }
        }

        time_clock::time_point end = time_clock::now();
        std::chrono::milliseconds ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start); 
        /* TICK당 걸리는 시간이 16.7ms. 이보다 적게 걸렸다면 뺸 시간 만큼 sleep하여 60fps 구현 */
        if(ms_duration.count() < (SECONDS_PER_TICK * 1000))
        {
            int wait_ms = (SECONDS_PER_TICK * 1000) - ms_duration.count();
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        }
    }
    return 0;
}
