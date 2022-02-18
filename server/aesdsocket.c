#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<string.h>

#define PORT_NUMBER ("9000")
#define FAMILY (AF_INET6) //set the AI FAMILY
#define SOCKET_TYPE (SOCK_STREAM) //set to TCP
#define FLAGS (AI_PASSIVE)
#define IP_ADDR	(0)
#define MAX_PENDING_CONN_REQUEST  (3)
int get_address_info(struct addrinfo** serveinfo)
{
	int status = 0;
	//get address
	struct addrinfo hints;
	 
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = FAMILY; //don't care if IPV4 or IPV6
	hints.ai_socktype = SOCKET_TYPE; //TCP
	hints.ai_flags = FLAGS; //fill in IP for me
	status = getaddrinfo(NULL,PORT_NUMBER,&hints, serveinfo);
	return status;

}
int main()
{
	int status = 0;
		
	//get address info	
	struct addrinfo* serveinfo = NULL;
	status = get_address_info(&serveinfo);
	if(status != 0)
	{
		printf("get_addrress_info() failed\n\r");
		return -1;
	}

	//socket creation
	int sockfd= socket(FAMILY,SOCKET_TYPE, IP_ADDR);
	if(sockfd == -1)
	{
		printf("socket() API failed\n\r");
		return -1;
	}
	
	//After creation of the socket, bind function binds the socket to address and port number
	status = bind(sockfd, serveinfo->ai_addr,  serveinfo->ai_addrlen);
	if(status == -1 )        
	{
		printf("%s",gai_strerror(status));
		printf("bind() is failed/n/r");
		return -1;
	}
	
	//Listen
	status = listen(sockfd,MAX_PENDING_CONN_REQUEST) ;
	if(status == -1)
	{
		printf("listen() failed\n\r");
		return -1;
	}
	printf("waiting for connection from client\n\r");
	//Accept
	status = accept(sockfd,serveinfo->ai_addr, &(serveinfo->ai_addrlen) );	
	if(status == -1)
	{
		printf("accept() failed\n\r");
		return -1;
	}	

	printf("Accepted connection from client \n\r");
	
}
