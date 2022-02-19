#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<string.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <stdlib.h>

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

int read_file(FILE* fptr1,char** buffer,int* read_data_len)
{
	
	if(buffer == NULL)
	{
		syslog(LOG_ERR,"read_file failed buffer is NULL");
		return -1;
	}
	
	//open file to write the received data from client
	FILE* fptr = fopen(RECV_FILE_NAME,"r"); //use a+ to open already existing file, w to create new file if not exist 
	if (fptr == NULL)
	{
		syslog(LOG_ERR,"read_file fptr is NULL");
		return -1;
	}
	else
	{    
		int status = 0;
		status = fseek(fptr, 0, SEEK_END);
		if(status != 0)
		{
			syslog(LOG_ERR, "read_file fseek failed");
			return -1;
		}
		int length = ftell(fptr);
		status = fseek(fptr, 0, SEEK_SET);
		if(status != 0)
		{
			syslog(LOG_ERR, "read_file fseek failed");
			return -1;
		}
		*buffer = malloc(length);
		if (*buffer == NULL)
		{
			syslog(LOG_ERR,"read_file malloc failed");
			return -1;
		}
		status = fread(*buffer, 1, length, fptr);
		puts("debugging:\n");
		for(int i = 0; i < length ; i++)
		{
			printf("%d\n",(*buffer)[i]);
		}
		//printf("read file %s length %d",*buffer,length);
		if(status != length)
		{
			syslog(LOG_ERR, "read_file fread failed expected length%d actual length%d",length, status);
			return -1;
		}
		*read_data_len = status;
	}
	fclose(fptr);
	return 0;
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

	int accepted_sockfd = 0;
	accepted_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);	

	if(accepted_sockfd == -1)
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

	//Receive data from client
	char recv_data[1000];
	memset(&recv_data[0],0,1000*sizeof(char));
	status = recv(accepted_sockfd, &recv_data[0],1000,0);
	if(status == -1)
	{
		syslog(LOG_ERR,"recv() failed");
		return -1;
	}
	puts(&recv_data[0]);
	status = fwrite(&recv_data[0],1,status,fptr);

	fclose(fptr);

	//Read the data from file /var/tmp/aesd_socket
	
	char* read_buffer = NULL;
	int read_data_len = 0;
	status = read_file(fptr,&read_buffer,&read_data_len);
	if(status == -1)
	{
		syslog(LOG_ERR, "read_file() failed");
		return -1;
	}
	printf("Received data %s from clinet length %ld\n\r",recv_data,strlen(recv_data));
	printf("Sending data %s to clinet length %d\n\r",read_buffer,read_data_len);

	
	//printf("\nsending string length %ld",strlen(read_file_data));

	//Send data to client that was read from file
	status = send(accepted_sockfd,read_buffer,read_data_len,0);
	if(status == -1)
	{
		syslog(LOG_ERR,"send() failed");
		return -1;
	}


	closelog();

}

