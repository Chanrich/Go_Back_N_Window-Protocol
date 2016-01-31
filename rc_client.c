// UDP Client - Richard Chan
// EE450

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/time.h>
#include <time.h>
#define BUFLEN 8192
#define WINDOW 255
#define TOTAL_SIZE 10 * 1024 * 1024
#define PORT 34569
 
void err(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc, char** argv)
{
    struct sockaddr_in serv_addr;
    struct timeval timeout, tm1,tm2;
    int sockfd, i, slen=sizeof(serv_addr);
    unsigned int current_packet_index;
    int socket_buffer_size = 64 * 1024; //64KB
    unsigned char send_counter = 0, ack_counter = 0, ack_msg;
    unsigned int total_data = TOTAL_SIZE, number_of_packets;
    int window_size = WINDOW, buf_len = BUFLEN;
    int ack_loss_count;
    unsigned long long t;
    char buf[BUFLEN], msg[3], timeoutoutput;

 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        err("socket");
 
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_aton("192.168.1.104", &serv_addr.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    //Set Timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000; // 200 milliseconds
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout)) < 0){
    	printf("Set Timeout Failed\n");
    	exit(1);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*) &timeout, sizeof(timeout)) < 0){
    	printf("Set Timeout Failed\n");
    	exit(1);
    }

    // Set Receive and Send Buffer to 64KB
    if ( setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &socket_buffer_size, sizeof(socket_buffer_size)) == -1){
    	printf("Error setting socket\n");
    }
    if ( setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &socket_buffer_size, sizeof(socket_buffer_size)) == -1){
    	printf("Error setting socket\n");
    }

    // Building the message
 	for (i = 1; i < BUFLEN; i++){
 		buf[i] = 'R';
 	}
 	buf[BUFLEN-1] = '\0';

 	// Calculate the packets and total data to send: 10MB
 	number_of_packets =  TOTAL_SIZE / BUFLEN;
 	printf("\n========  Info  ==========\n");
 	printf("\tTotal Data to send:%i\n", TOTAL_SIZE);
 	printf("\tNumber of Packets to send: %i\n", number_of_packets);
 	printf("\n==========================\n");

 	//Connect With server
 	timeoutoutput = 0;
 	while (1){
	 	sendto(sockfd, "connect", sizeof("connect"), 0, (struct sockaddr*)&serv_addr, slen);
		if ( recvfrom(sockfd, msg, sizeof(msg), 0, NULL, NULL) < 0){
			// Timeout Reached
			if (timeoutoutput == 0){
				printf("Timeout\n");	
				timeoutoutput = 1;
			}
		}
		printf("msg: %s\n", msg);
		if (strncmp(msg,"OK",2) == 0) {
			sendto(sockfd, "ty", sizeof("ty"), 0, (struct sockaddr*)&serv_addr, slen);
			break;
		}
		//printf("msg:%s\n",msg);
 	}
	printf("Received msg:%s -- Connection established -- \n", msg);

 	//Start sending data, record the start time
 	current_packet_index = 0;
 	ack_loss_count = 0;
 	send_counter = 0;
 	gettimeofday(&tm1, NULL); //Record the beginning time
  	while (current_packet_index < number_of_packets){
 		// Not finish sending yet
 		for (i = 0; i < window_size; i++){
 			// In window, send data
 			buf[0] = send_counter;
 			//printf("Sending Packet Number: %d\n", buf[0]);
 			sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
 			//printf("Sent: %s\n", &buf[1]);
 			send_counter++;
 			if (send_counter >= window_size){
 				// max reached, restart counter
 				//printf("Send counter recount, from %d to %d\n", send_counter, send_counter % window_size);
 				send_counter = send_counter % window_size;
 			}
 		}

 		//Receive Ack, Ack is how many packets the receiver received correctly
 		if (recvfrom(sockfd, &ack_msg, sizeof(ack_msg), 0, (struct sockaddr*)&serv_addr, (unsigned int*)&slen) < 0){
 			//ACk is lost, dont update current_packet_index
 			printf("ACK is lost\n");
 			break;
 		} else {
			//printf("Received ACK:%d\n", ack_msg);
			// Check if Ack is a good or bad
			current_packet_index = current_packet_index + ack_msg;
 		}
		printf("current_packet_index:%u last ack:%u\n", current_packet_index, ack_msg);

		if (ack_loss_count > 10){
			break;
		}
 	}
 	printf("Transfer complete: number_of_packets needed:%u current_packet_index:%u\n", number_of_packets, current_packet_index);
 	printf("Printing Time...\n");
 	gettimeofday(&tm2, NULL);
 	t = 1000 * (tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec) / 1000;
 	printf("%llu ms\n", t);

 
    close(sockfd);
    return 0;
}