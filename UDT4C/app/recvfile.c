#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <udtc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>

#define LOCALPORT "8999"

int main(int argc, char* argv[])
{
   if ((argc != 5) || (0 == atoi(argv[2])))
   {
      printf("usage: recvfile server_ip server_port remote_filename local_filename\n");
      return -1;
   }

   // use this function to initialize the UDT library
   udt_startup();

   struct addrinfo hints, *peer,*local;

   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   //****************begin***************
   //client bind to local port
   if(0 != getaddrinfo(NULL,LOCALPORT,&hints,&local))
    {
        printf("incorrect network address.\n");
        return 0;
    }


   UDTSOCKET fhandle = udt_socket(local->ai_family, local->ai_socktype, local->ai_protocol);

   if ( -1 == udt_bind(fhandle, local->ai_addr, local->ai_addrlen))
   {
       printf("bind error");
       return 0;
   }
   //***************end*******************



   //get remote server's addrinfo
   if (0 != getaddrinfo(argv[1], argv[2], &hints, &peer))
   {
      printf("incorrect server/peer address. %s : %s ", argv[1], argv[2]);
      return -1;
   }


   // connect to the server, implict bind
   if (UDT_ERROR == udt_connect(fhandle, peer->ai_addr, peer->ai_addrlen))
   {
      return -1;
   }

   freeaddrinfo(peer);


   // send name information of the requested file
   int len = strlen(argv[3]);

   if (UDT_ERROR == udt_send(fhandle, (char*)&len, sizeof(int), 0))
   {
      //printf( "send: %s", udt_getlasterror().getErrorMessage()) ;
      return -1;
   }

   if (UDT_ERROR == udt_send(fhandle, argv[3], len, 0))
   {
      //printf("send: %" ,udt_getlasterror().getErrorMessage());
      return -1;
   }

   // get size information
   int64_t size;

   if (UDT_ERROR == udt_recv(fhandle, (char*)&size, sizeof(int64_t), 0))
   {
      //printf("send: %" ,udt_getlasterror().getErrorMessage());
      return -1;
   }

   if (size < 0)
   {
      printf("no such file %s  on the server\n",argv[3]);
      return -1;
   }

   // receive the file
   int64_t recvsize; 
   int64_t offset=0;

   if (UDT_ERROR == (recvsize = udt_recvfile2(fhandle, argv[4],&offset, size,7280000)))
   {
      //printf("send: %" ,udt_getlasterror().getErrorMessage());
      return -1;
   }

   udt_close(fhandle);


   // use this function to release the UDT library
   udt_cleanup();

   return 0;
}
