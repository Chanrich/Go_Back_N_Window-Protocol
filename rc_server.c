// UDP Server - Richard Chan 
// EE450 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <time.h>


#define BUFLEN 8192
#define WINDOW 255
#define TOTAL_SIZE 10 * 1024 * 1024
#define PORT 34569
 
void err(char *str)
{
    perror(str);
    exit(1);
}
 
int main(void)
{
    struct sockaddr_in my_addr, cli_addr;
    int sockfd, i, current_receive_index, lost_packets_count, lost_packets_bytes; 
    int buf_len = BUFLEN; 
    struct timeval timeout;
    int socket_buffer_size = 64 * 1024; //64KB
    int number_of_packets = TOTAL_SIZE/BUFLEN; 
    int window_size= WINDOW;
    char packetIsLost = 0;
    socklen_t slen=sizeof(cli_addr);
    unsigned char buf[BUFLEN];
    unsigned char receive_counter = 0, ack_counter = 0;
    char msg[10];
    char *serverIP;
 	serverIP = "192.168.1.104";
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      err("socket");
    else
      printf("Server : Socket() successful\n");
 
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = inet_addr(serverIP);
    //Set Timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000; // 200 milliseconds
    if (bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr))==-1)
      err("bind");
    else
      printf("Server : bind() successful\n");
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout)) < 0){
        printf("Set Timeout Failed\n");
        exit(1);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*) &timeout, sizeof(timeout)) < 0){
        printf("Set Timeout Failed\n");
        exit(1);
    }
    // Set Receive and Send Buffer to 64KB
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &socket_buffer_size, sizeof(socket_buffer_size));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &socket_buffer_size, sizeof(socket_buffer_size));

 
    while(1)
    {
    	// Establish a connection with client
    	recvfrom(sockfd, msg, sizeof(msg), 0, (struct sockaddr*)&cli_addr, &slen);

    	if (strncmp(msg,"connect",sizeof("connect")) != 0) {
    		//printf("msg doesn't verify\n");
    		continue;
    	}
    	printf("Connected, Sending OK\n");
        while(1){
            sendto(sockfd, "OK", sizeof("OK"), 0, (struct sockaddr*)&cli_addr, slen);
            recvfrom(sockfd, msg, sizeof(msg), 0, (struct sockaddr*)&cli_addr, &slen);
            if (strncmp(msg,"ty",2) == 0){
                break;
            }
            printf("msg : %s\n",msg);
            
        }

        printf("Starting...\n");
        //Start Receiving packets
        receive_counter = 0; // First packet is #1
        current_receive_index = 0;
        lost_packets_count = 0;
        while(current_receive_index < number_of_packets){
        	packetIsLost = 0;
        	ack_counter = 0;
        	for (i = 0; i < window_size; i++){
                bzero(&buf, sizeof(buf));
	        	recvfrom(sockfd, buf, buf_len, 0, (struct sockaddr*)&cli_addr, &slen);
	        	printf("Received packet #: %u, receive_counter:%u\t\n", buf[0], receive_counter);
	        	if (buf[0] != receive_counter && !packetIsLost){
	        		// The packet number don't match and packet not lost before
	        		packetIsLost = 1;
	        		ack_counter = receive_counter;
	        		printf("Packet Didn't match, Received packet #:%u, Current counter:%u\n", buf[0], receive_counter);
	        		//exit(-1);
	        	} 
	        	//if (strncmp(buf, &receive_counter, 1) == 0){
                if (buf[0] == receive_counter){
	        		// Packet match
	        		receive_counter++;
	        		current_receive_index++;

	        	} else {
                    current_receive_index++;
                }
                if (packetIsLost){
	        		// Count lost packets
	        		lost_packets_count++;
                    printf("lost packet count: %d \n",lost_packets_count);
	        	}

	        	//update receive counter
	        	if (receive_counter >= window_size){
	        		//printf("Updating Receive counter from %d to %d\n", receive_counter, receive_counter % window_size);
	        		receive_counter = receive_counter % window_size ;
	        	}
        	}

        	if (packetIsLost == 1){
        		// Some packet is lost, send NCK?
        		sendto(sockfd, &ack_counter, sizeof(ack_counter), 0, (struct sockaddr*)&cli_addr, slen);
        	} else {
        		// Packet is not lost, send ACK
        		ack_counter = window_size;
        		sendto(sockfd, &ack_counter, sizeof(ack_counter), 0, (struct sockaddr*)&cli_addr, slen);
                printf("ACK: %u\n", ack_counter);
        	}
        	//printf("Sending ACK #:%d\n", ack_counter);
        	
        }
        lost_packets_bytes = (int)(lost_packets_count * window_size);
        printf("current_receive_index:%i\n", current_receive_index);
        printf("Number of packets:%d\n", number_of_packets);
        printf("Lost packets: %d Lost bytes:%d\n", lost_packets_count, lost_packets_bytes);
        exit(-1);
    }
 
    close(sockfd);
    return 0;
}