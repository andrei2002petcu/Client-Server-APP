#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "helpers.h"

int main(int argc, char **argv) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    DIE (argc != 2, "FAILURE: wrong arguments");

//UDP SOCKET
    int sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE (sock_udp < 0, "FAILURE: UDP socket");

    //fill server info
    struct sockaddr_in servaddr, udp_servaddr;
    unsigned int udp_len = sizeof(udp_servaddr);

    memset (&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(atoi(argv[1]));

    //bind socket with srv addr
    int ret = bind(sock_udp, (const struct sockaddr *)&servaddr, sizeof(struct sockaddr));
    DIE (ret < 0, "FAILURE: UDP binding");

//TCP SOCKET
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE (sock_tcp < 0, "FAILURE: TCP socket");

    struct sockaddr_in tcp_servaddr;
    unsigned int tcp_len = sizeof(tcp_servaddr);

    //bind + listen
    ret = bind(sock_tcp, (const struct sockaddr *)&servaddr, sizeof(struct sockaddr));
    DIE (ret < 0, "FAILURE: TCP binding");
    int optval = 1;
    setsockopt(sock_tcp, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int));
    DIE (listen(sock_tcp, 5) < 0, "FAILURE: listen");

    //init FD
    fd_set fd;
    FD_ZERO(&fd);

    FD_SET(sock_udp, &fd);
    FD_SET(sock_tcp, &fd);
    FD_SET(STDIN_FILENO, &fd);

    //get the max fd
    int max_fd;
    max_fd = (sock_tcp > sock_udp) ? sock_tcp : sock_udp;

    //allocate an array of clients
    client *clients = calloc(256, sizeof(client));

    int server_online = 1; //flag for server status
    while(server_online) {

        fd_set fd_copy = fd;
        DIE (select(max_fd + 1, &fd_copy, NULL, NULL, NULL) < 0, "FAILURE: select");

        for (int i = 0; i <= max_fd; i++) {
            char buff[BUFFLEN];
            memset(buff, 0, BUFFLEN);
            
            if (FD_ISSET(i, &fd_copy)) {
                         
                if (i == sock_tcp) { //new connection request from TCP client

                    int newsock = accept(sock_tcp, (struct sockaddr *)&tcp_servaddr, &tcp_len);
                    DIE (newsock < 0, "FAILURE: accept");
                    
                    //take client id
                    recv(newsock, buff, 10, 0);

                    //search for client in the clients array
                    int index = -1, status;
                    for (int j = 5; j <= max_fd; j++) {
                        if (strcmp(clients[j].id, buff) == 0) {
                            index = j;
                            status = clients[j].online;
                            break;
                        }
                    }
                    
                    //if client is connected and offline
                    if (index != -1 && status == 0) {
                        //wake up client (update his status and socket)
                        FD_SET(newsock, &fd);
                        clients[index].online = 1;
                        clients[index].socket = newsock;
                        printf("New client %s connected from %s:%d.\n", clients[index].id, inet_ntoa(tcp_servaddr.sin_addr), ntohs(tcp_servaddr.sin_port));

                        //send all stored messages
                        for (int j = 0; j < clients[index].stored_packets_no; j++) {
                            send(clients[index].socket, &clients[index].stored_packets[j], sizeof(struct tcp_packet), 0);
                        }
                    }
                    //if client is connected and online
                    else if (index != -1 && status == 1) {
                        printf("Client %s already connected.\n", clients[index].id);
                        close(newsock);
                    }
                    //if client is not connected
                    else if (index == -1) {
                        //add the new socket and update max_fd
                        FD_SET(newsock, &fd);
                        if (newsock > max_fd)
                            max_fd = newsock;

                        //add the new client
                        clients[max_fd].online = 1;
                        clients[max_fd].socket = newsock;
                        strcpy(clients[max_fd].id, buff);

                        printf("New client %s connected from %s:%d.\n", clients[max_fd].id, inet_ntoa(tcp_servaddr.sin_addr), ntohs(tcp_servaddr.sin_port));
                    }
                }
                else if (i == sock_udp) { //new message from UDP client

                    char buffp[1551] = {0};
                    ret = recvfrom(sock_udp, buffp, 1551, 0, (struct sockaddr *)&udp_servaddr, &udp_len); 
                    DIE (ret < 0, "FAILURE: recvfrom");

                    //get the received packet (UDP)
                    struct udp_packet *packet_recv = packet_recv = (struct udp_packet *)buffp;
                    
                    //create new packet to send to clients (TCP)
                    struct tcp_packet packet_send = {0};
                    strcpy(packet_send.topic, packet_recv->topic);
                    packet_send.topic[50] = '\0';
                    packet_send.port = htons(udp_servaddr.sin_port);
                    strcpy(packet_send.ip_addr, inet_ntoa(udp_servaddr.sin_addr));

                    //check the data type and convert it
                    if(packet_recv->data_type == 0) {
                        strcpy(packet_send.type, "INT");

                        uint32_t n = ntohl(*(uint32_t *)(packet_recv->payload + 1));
                        if (packet_recv->payload[0] == 1)
                            n = n * (-1);
                            
                        sprintf(packet_send.payload, "%d", n);
                    }
                    else if(packet_recv->data_type == 1) {
                        strcpy(packet_send.type, "SHORT_REAL");

                        double n;
                        n = ntohs(*(uint16_t *)(packet_recv->payload)) / 100.0;

                        sprintf(packet_send.payload, "%.2f", n);
                    }
                    else if(packet_recv->data_type == 2) {
                        strcpy(packet_send.type, "FLOAT");

                        double n = (ntohl(*(uint32_t *)(packet_recv->payload + 1)));

                        uint8_t power;
                        memcpy(&power, packet_recv->payload + 5, sizeof(uint8_t));
                        while(power) {
                            n = n / 10.0;
                            power--;
                        }
                        if(packet_recv->payload[0] == 1)
                            n = n * (-1);

                        sprintf(packet_send.payload, "%f", n);
                    }
                    else if(packet_recv->data_type == 3) {
                        strcpy(packet_send.type, "STRING");
                        strcpy(packet_send.payload, packet_recv->payload);
                    }

                    //send to all clients subscribed to the topic
                    for (int j = 5; j <= max_fd; j++)
                        //parse client's topics to check if subscribed
                        for (int k = 0; k < clients[j].topics_no; k++)
                            if (strcmp(clients[j].topics[k].title, packet_recv->topic) == 0) {
                                
                                //if client is online send message
                                if(clients[j].online == 1) {
                                    ret = send(clients[j].socket, &packet_send, sizeof(struct tcp_packet), 0);
                                    DIE (ret < 0, "FAILURE: send");
                                }
                                // else store message
                                else {
                                    if(clients[j].topics[k].sf == 1) {
                                        clients[j].stored_packets[clients[j].stored_packets_no] = packet_send;
                                        clients[j].stored_packets_no++;
                                    }
                                }
                            break;
                            }
                }
                else if (i == STDIN_FILENO) { //new command from stdin

                    //read command and check if server need to shutdown
                    fgets(buff, BUFFLEN - 1, stdin);
                    if (strncmp(buff, "exit", 4) == 0) {
                        server_online = 0;
                        break;
                    }
                }
                else { //new command from TCP client

                    ret = recv(i, buff, sizeof(struct packet), 0);
                    DIE (ret < 0, "FAILURE: recv");
                    
                    //get the client that sent the message
                    int index = 5;
                    while (index <= max_fd && clients[index].socket != i)
                        index++;
                    struct client *client = &clients[index];

                    if (ret > 0) {
                        struct packet *packet_recv = (struct packet *)buff;

                        if (strcmp(packet_recv->req_type, "subscribe") == 0) {
                            //check if client is already subscribed to the topic
                            index = -1;
                            for (int j = 0; j < client->topics_no; j++)
                                if (strcmp(client->topics[j].title, packet_recv->topic.title) == 0) {
                                    index = j;
                                    break;
                                }
                            
                            //if client is not subscribed to the topic, add topic to his topics
                            if (index == -1) {
                                strcpy(client->topics[client->topics_no].title, packet_recv->topic.title);
                                client->topics[client->topics_no].sf = packet_recv->topic.sf;
                                client->topics_no++;
                            }
                        }
                        else if (strcmp(packet_recv->req_type, "unsubscribe") == 0) {
                            //check if client is subscribed to the topic
                            index = -1;
                            for (int j = 0; j < client->topics_no; j++)
                                if (strcmp(client->topics[j].title, packet_recv->topic.title) == 0) {
                                    index = j;
                                    break;
                                }
                            
                            //if client is subscribed to the topic, remove it
                            if (index != -1) {
                                //move last topic to the position of the topic that needs to be removed
                                strcpy(client->topics[index].title, client->topics[client->topics_no - 1].title);
                                client->topics[index].sf = client->topics[client->topics_no - 1].sf;
                                client->topics_no--;
                            }
                        }
                        else if (strcmp(packet_recv->req_type, "exit") == 0) {
                            //close connection with client and update his status to '0' (offline)
                            printf("Client %s disconnected.\n", client->id);
                            
                            client->online = 0;
                            client->socket = -1;
                            FD_CLR(i, &fd);
                            close(i);
                            break;
                        }
                    }
                    //if client shut down
                    else if (ret == 0) {
                        //close connection with client and update his status to '0' (offline)
                        printf("Client %s disconnected.\n", client->id);
                            
                        client->online = 0;
                        client->socket = -1;
                        FD_CLR(i, &fd);
                        close(i);
                        break;
                    }
                }
            }
        }
    }
    
    //close all the connections
    for(int i = 3; i <= max_fd; i++)
        if (FD_ISSET(i, &fd))
            close(i);

    return 0;
}