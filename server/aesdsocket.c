#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<string.h>
#include <syslog.h>
#include <arpa/inet.h>

#define PORT_NUMBER ("9000")
#define FAMILY (AF_INET6) //set the AI FAMILY
#define SOCKET_TYPE (SOCK_STREAM) //set to TCP
#define FLAGS (AI_PASSIVE)
#define IP_ADDR	(0)
#define MAX_PENDING_CONN_REQUEST  (3)
#define RECV_FILE_NAME ("/var/tmp/aesd_socketdata")

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
	openlog("AESD_SOCKET", LOG_DEBUG, LOG_DAEMON); //check /var/log/syslog

	int status = 0;
		
	//get address info	
	struct addrinfo* serveinfo = NULL;
	status = get_address_info(&serveinfo);
	if(status != 0)
	{
		syslog(LOG_ERR,"get_addrress_info() failed\n\r");
		return -1;
	}

	//socket creation
	int sockfd= socket(FAMILY,SOCKET_TYPE, IP_ADDR);
	if(sockfd == -1)
	{
		syslog(LOG_ERR,"socket() failed\n\r");
		return -1;
	}
	
	//After creation of the socket, bind function binds the socket to address and port number
	status = bind(sockfd, serveinfo->ai_addr,  serveinfo->ai_addrlen);
	if(status == -1 )        
	{
		syslog(LOG_ERR,"bind() failed\n\r");
		return -1;
	}
	
	//Listen
	status = listen(sockfd,MAX_PENDING_CONN_REQUEST) ;
	if(status == -1)
	{
		syslog(LOG_ERR,"listen() failed\n\r");
		return -1;
	}
	syslog(LOG_ERR,"Waiting for connection from client() \n\r");
	//Accept
	//struct sockaddr client_addr; 
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));

	socklen_t client_addr_len = 0;

	status = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);	

	if(status == -1)
	{
		syslog(LOG_ERR,"accept() \n\r");
		return -1;
	}	


	//syslog(LOG_ERR,"Accepted connection from %d%d%d%d\n\r",);
	//printf("Client Adress = %s",inet_ntoa(client_addr.sin_addr));
	syslog(LOG_ERR,"Accepted connection from %s\n\r",inet_ntoa(client_addr.sin_addr)); //inet_ntoa converts raw address into human readable format


	//open file to write the received data from client
	FILE* fptr = fopen(RECV_FILE_NAME,"w"); //use a+ to open already existing file, w to create new file if not exist 
	if(fptr == NULL)
	{
		syslog(LOG_ERR,"fopen() \n\r");
		return -1;	
	}
	closelog();

}

