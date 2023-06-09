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

int recv_all(int sockfd, void *buffer, size_t len) {

    size_t bytes_received = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;

        while(bytes_remaining) {
            int ret = recv(sockfd, buff, bytes_remaining, 0);
            DIE (ret < 0, "FAILURE: recv");
            bytes_received += ret;
            bytes_remaining -= ret;
            buff += ret;
        }

    return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_sent = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;

        while(bytes_remaining) {
            int ret = send(sockfd, buff, bytes_remaining, 0);
            DIE (ret < 0, "FAILURE: send");
            bytes_sent += ret;
            bytes_remaining -= ret;
            buff += ret;
        }

    return bytes_sent;
}

int main(int argc, char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    DIE (argc != 4, "FAILURE: wrong arguments");

    //create TCP socket
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE (sock_tcp < 0, "FAILURE: TCP socket");

    //set TCP_NODELAY
    int optval = 1;
    setsockopt(sock_tcp, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int));

    //fill subscriber info
    struct sockaddr_in tcp_subaddr;
    tcp_subaddr.sin_family = AF_INET;
    tcp_subaddr.sin_port = htons(atoi(argv[3]));
    inet_aton(argv[2], &tcp_subaddr.sin_addr);

    //connect to server
    int ret = connect(sock_tcp, (const struct sockaddr*) &tcp_subaddr, sizeof(tcp_subaddr));
    DIE (ret < 0, "FAILURE: TCP connect");

    //send ID to server
    char id[10] = {0};
    strcpy(id, argv[1]);
    ret = send_all(sock_tcp, id, 10);
    DIE (ret < 0, "FAILURE: TCP send");
    
    //init FD
    fd_set fd;
    FD_ZERO(&fd);

    FD_SET(sock_tcp, &fd);
    FD_SET(STDIN_FILENO, &fd);

    while(1) {
        fd_set fd_copy = fd;
        DIE (select(sock_tcp + 1, &fd_copy, NULL, NULL, NULL) < 0, "FAILURE: select");

        char buff[BUFFLEN];
        memset(buff, 0, BUFFLEN);

        if (FD_ISSET(STDIN_FILENO, &fd_copy)) {

            //read the command from stdin            
            fgets(buff, BUFFLEN - 1, stdin);

            //packet to send to the server
            struct packet packet_send = {0};
            
            if (strncmp(buff, "subscribe ", 10) == 0) {
                
                //parse command and fill packet_to_send fields
                char *token = strtok(buff, " ");
                token = strtok(NULL, " ");
                strcpy(packet_send.topic.title, token);
                token = strtok(NULL, " ");
                packet_send.topic.sf = atoi(token);
                strcpy(packet_send.req_type, "subscribe");

                //send the packet to server and print message
                ret = send_all(sock_tcp, &packet_send, sizeof(struct packet));
                DIE (ret < 0, "FAILURE: TCP send");
                printf("Subscribed to topic.\n");
            }
            else if (strncmp(buff, "unsubscribe ", 12) == 0) {
                
                //parse command and fill packet_to_send fields
                char *token = strtok(buff, " ");
                token = strtok(NULL, " ");
                strcpy(packet_send.topic.title, token);
                strcpy(packet_send.req_type, "unsubscribe");

                //send the packet to server and print message
                ret = send_all(sock_tcp, &packet_send, sizeof(struct packet));
                DIE (ret < 0, "FAILURE: TCP send");
                printf("Unsubscribed from topic.\n");
            }
            else if (strncmp(buff, "exit", 4) == 0) {
                //send exit type message to server
                strcpy(packet_send.req_type, "exit");
                ret = send_all(sock_tcp, &packet_send, sizeof(struct packet));
                DIE (ret < 0, "FAILURE: TCP send");
                break;
            }
            else {
                printf("Invalid command.\n");
            }
        }
        if (FD_ISSET(sock_tcp, &fd_copy)) {
            //received message from server
            
            struct tcp_packet packet_recv = {0};
            ret = recv_all(sock_tcp, &packet_recv, sizeof(struct tcp_packet));
            DIE (ret < 0, "FAILURE: TCP recv");
            if(ret == 0)
                break;

            //check if exit type message
            if (strncmp(packet_recv.type, "exit", 4) == 0)
                break;

            //print message info
            printf("%s:%d - %s - %s - %s\n", packet_recv.ip_addr, packet_recv.port, packet_recv.topic, packet_recv.type, packet_recv.payload);
        }
    }

    //close socket
    close(sock_tcp);
    return 0;
}