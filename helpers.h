#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro for handling errors
 * Example:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

//Macro was taken from lab
#define DIE(assertion, call_description)                                       \
    do {                                                                       \
        if (assertion) {                                                       \
            fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                 \
            perror(call_description);                                          \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)

//Structure for storing a TCP message
typedef struct tcp_packet {
    char ip_addr[16];
    uint16_t port;
    
    char topic[51]; // 50 + '\0'
    char type[11];
    char payload[1501]; // 1500 + '\0'
}tcp_packet;

//Structure for storing an UDP message
typedef struct udp_packet {
    char topic[50];
    uint8_t data_type;
    char payload[1500];
}udp_packet;

//Structure for client's topics array (to store topic's title and sf)
typedef struct topic {
    char title[50];
    int sf;
}topic;

//Structure for storing a client
typedef struct client {
    char id[10];
    int socket;
    int online;
    struct topic topics[100]; //topics that client is subscribed to
    int topics_no;
    struct tcp_packet stored_packets[100]; //packets received while offline
    int stored_packets_no;
}client;

//Structure for storing a packet that is sent to server (from TCP clients)
typedef struct packet {
    struct topic topic;
    uint8_t data_type;
    char req_type[50]; //type of request (subscribe, unsubscribe, exit)
    char payload[1501];
    
    char ip_addr[16];
    uint16_t port;
}packet;

#define BUFFLEN 256

#endif

