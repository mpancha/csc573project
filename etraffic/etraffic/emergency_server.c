/*
 * Author:
 * Description: This is the emergency server code.
 *              It keeps track of recieved emergency messages
 *              and prints out missed packets.
 *
 */
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

int prevSeq=-1, seqNo=0;
static int packetCounter =0;
char seq_no[8];

/* API to track recieved sequence number */
void updateSeq(char *sourceIP,char *message, int len, int portNo)
{
    int missedSeq=0, missedPackets=0;

    packetCounter++;
    bzero(seq_no,8);
    if(len<40)
	return;
    strncpy(seq_no, message+40,7);
    seq_no[7]='\0';
    seqNo = atoi(seq_no);

    if(packetCounter%50 == 0)
    {
         printf("packets receiving. Curr packet- %d\n",packetCounter);
    }

    if(prevSeq != seqNo-1)
    {
        missedPackets = seqNo - prevSeq - 1;
	if(missedPackets>0)
           printf("IP:%s, PortNo: %d, packets missed- %d, last seq- %d, Current Seq- %d \n",sourceIP,portNo,missedPackets,prevSeq,seqNo);
    }
    prevSeq = seqNo;
}

int main(int argc, char**argv)
{
   int sockfd,n;
   struct sockaddr_in servaddr,cad;
   socklen_t len;
   char message_to_archive[1501];
   char header[500], footer[200];
   char ack[100];
   char arch_filename[50];
   FILE *fp;
   struct timeval tv;
   struct tm nowtm;
   short proto_port = 8222;


   if (argc > 1) {
      proto_port = atoi (argv[1]);
   }

   sockfd=socket(AF_INET,SOCK_DGRAM,0);
   if(sockfd<0)
   {
      printf("sockfd faield\n");
      return;
   }
   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(proto_port);
   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

   while (1)
   {
      len = sizeof(cad);
      bzero (message_to_archive, 1501);
      n = recvfrom (sockfd,message_to_archive,1500,0,(struct sockaddr *)&cad,&len);
      if (n < 0) {
           fprintf (stderr, "ERROR reading from socket: %d", n);
           printf ("ERROR reading from socket: %d", n);
           continue;
      }
      updateSeq(inet_ntoa(cad.sin_addr),message_to_archive, n, ntohs(cad.sin_port));
   }
}
