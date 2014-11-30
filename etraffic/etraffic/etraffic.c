/*
 * Author: Mitul Panchal
 *
 * Description: This program sends a emergency message to specific server
 *              using UDP connection with provided DSCP code point and options
 *              field.
 *
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define BUFFER_SIZE  1024

char message[BUFFER_SIZE]; // Server is not expecting message more than 1000 bytes anyway
char resp_msg[BUFFER_SIZE]; // Why shall we expect response more than 1000 bytes!

int main(int argc, char *argv[])
{
    int udp_socket, tcp_socket, src_port, dst_port, n_sent,n_recv,n_msg=1, len,opt_val=0;
    struct hostent *host;
    struct sockaddr_in server, local;
    struct tm date_time;
    time_t T;
    int j=0,n=0, dscp=0;
 	int i;
    int flag = 1;

    char val[4];
    val[0]=136;
    val[1]=4;
    val[2]=150;


    /* Argument parsing */

    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <server> <dstport> <client> <srcport> <nmsg>\n", argv[0]);
        return 1;
    }

    dscp = 0x38;
    dst_port = atoi(argv[2]);
    src_port = atoi(argv[4]);
    if(argc>5)
	n_msg = atoi(argv[5]);
    if(argc>6)
        opt_val=atoi(argv[6]);
    opt_val=1;
    if(argc>7)
	dscp = atoi(argv[7]);
    /* Get Time and Date */
    time(&T);
    date_time = *localtime(&T);

// 	n=sprintf(message+n,"THIS IS EMERGENCY SERIVE AF11 %d",1);
   /* Emergency message construction */
 	for(j=0;j<1000;j++)
	{
		if(j==999)
		{
		    n=j+sprintf(message+n, "THIS IS AN EMERGENCY MESSAGE AF11: Pkt=");
		}
		else
		   message[j] = 'E';
	}
    /* Populate server client structure */
    server.sin_family = AF_INET;
    server.sin_port = htons(dst_port);
    inet_aton(argv[1], &server.sin_addr);

    local.sin_family = AF_INET;
    local.sin_port = htons(src_port);
    inet_aton(argv[3], &local.sin_addr);

    /* Create udp socket */
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    int k= setsockopt(udp_socket, IPPROTO_IP,IP_OPTIONS, &opt_val,sizeof(opt_val));
    k |= setsockopt(udp_socket, IPPROTO_IP, IP_TOS, &dscp, sizeof(dscp));
    printf("Setsockopt return value is %d",k);
    /* Bind socket to assign local port */
    if(bind(udp_socket, (struct sockaddr*)&local, sizeof(struct sockaddr_in))<0)
    {
        printf("UDP Client:bind failed\n");
        return 1;
    }

    /* Send message to Server */
    for(i=0; i< n_msg; i++)
    {
      char pktnum[250], temp[1500];
      /* Sequence number update */
	  sprintf(pktnum,"%08d",i);
      strcpy(temp, message);
	  strcat(temp, pktnum);
	  printf("%s\n", temp);
          if(sendto(udp_socket, temp, strlen(temp),\
              0, (struct sockaddr*)&server, sizeof(struct sockaddr_in))<0)
          {
               printf("UDP Client:sendto failed\n");
              return 1;
          }
    }
    printf("Emergency Client: Sent : %d\n",n_msg);
    /* Close the socket */
    close(udp_socket);
    return 0;
}
