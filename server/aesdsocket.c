#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<string.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
 #include<signal.h>

#define PORT_NUMBER ("9000")
#define FAMILY (AF_INET6) //set the AI FAMILY
#define SOCKET_TYPE (SOCK_STREAM) //set to TCP
#define FLAGS (AI_PASSIVE)
#define IP_ADDR	(0)
#define MAX_PENDING_CONN_REQUEST  (3)
#define RECV_FILE_NAME ("/var/tmp/aesd_socketdata")

int sockfd = 0;
int accepted_sockfd = 0;
char* read_buffer = NULL;
char * recv_data = NULL;
FILE* fptr = NULL;
struct sockaddr_in client_addr;

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

void sighandler(int signal)
{
	if(signal == SIGINT || signal == SIGTERM)
	{

		//completing any open connection operations, 
		// fclose(fptr);

		//closing any open sockets, 
		close(sockfd);
		close(accepted_sockfd);
		syslog(LOG_ERR,"Closed connection from %s\n\r",inet_ntoa(client_addr.sin_addr)); //inet_ntoa converts raw address into human readable format

		// //free malloced buffer
		free(read_buffer);
		// free(recv_data);

		//and deleting the file /var/tmp/aesdsocketdata.
		//remove(RECV_FILE_NAME);
	}

}

int main()
{
	signal(SIGINT,sighandler);
	signal(SIGTERM,sighandler);
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
	 accepted_sockfd = 0;

	sockfd= socket(FAMILY,SOCKET_TYPE, IP_ADDR);
	if(sockfd == -1)
	{
		syslog(LOG_ERR,"socket() failed\n\r");
		freeaddrinfo(serveinfo);
		return -1;
	}
	
	//After creation of the socket, bind function binds the socket to address and port number
	status = bind(sockfd, serveinfo->ai_addr,  serveinfo->ai_addrlen);
	if(status == -1 )        
	{
		syslog(LOG_ERR,"bind() failed\n\r");
		freeaddrinfo(serveinfo);
		return -1;
	}
	
	//Listen
	status = listen(sockfd,MAX_PENDING_CONN_REQUEST) ;
	if(status == -1)
	{
		syslog(LOG_ERR,"listen() failed\n\r");
		freeaddrinfo(serveinfo);
		return -1;
	}

	//open file to write the received data from client
	FILE* fptr = fopen(RECV_FILE_NAME,"w"); //use a+ to open already existing file, w to create new file if not exist 
	if(fptr == NULL)
	{
		syslog(LOG_ERR,"fopen() \n\r");
		freeaddrinfo(serveinfo);
		return -1;	
	}

	fclose(fptr);

	while(1)
	{
		syslog(LOG_ERR,"Waiting for connection from client() \n\r");
		//Accept
		//struct sockaddr client_addr; 
		memset(&client_addr, 0, sizeof(client_addr));
		socklen_t client_addr_len = 0;


		accepted_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);	

		if(accepted_sockfd == -1)
		{
			syslog(LOG_ERR,"accept() \n\r");
			freeaddrinfo(serveinfo);
			return -1;
		}	


		//syslog(LOG_ERR,"Accepted connection from %d%d%d%d\n\r",);
		//printf("Client Adress = %s",inet_ntoa(client_addr.sin_addr));
		syslog(LOG_ERR,"Accepted connection from %s\n\r",inet_ntoa(client_addr.sin_addr)); //inet_ntoa converts raw address into human readable format


		//open file to write the received data from client
		fptr = fopen(RECV_FILE_NAME,"a"); //use a+ to open already existing file, w to create new file if not exist 
		if(fptr == NULL)
		{
			syslog(LOG_ERR,"fopen() \n\r");
			freeaddrinfo(serveinfo);
			return -1;	
		}

		//Receive data from client
		const int recv_len = 100;
		// char recv_data[RECV_LEN];
		recv_data = malloc(sizeof(char) * recv_len);
		memset(recv_data,0,recv_len*sizeof(char));

    	int n = 1, total = 0, found = 0;
		while (!found) 
		{
			n = recv(accepted_sockfd, &recv_data[total], recv_len, 0);
			if (n == -1) 
			{
				syslog(LOG_ERR,"\n\rrecv failed ");
				freeaddrinfo(serveinfo);
				return -1;	
			}
			total += n;
			found = (recv_data[total - 1] == '\n')?(1):(0);
			recv_data = realloc(recv_data, total + recv_len);
			if(recv_data == NULL)
			{
				syslog(LOG_ERR,"realloc failed()");
				freeaddrinfo(serveinfo);
				return -1;
			}
    	}

		
		status = fwrite(&recv_data[0],1,total,fptr);
		free(recv_data);

		fclose(fptr);

		//Read the data from file /var/tmp/aesd_socket
		
		read_buffer = NULL;
		int read_data_len = 0;
		status = read_file(fptr,&read_buffer,&read_data_len);
		if(status == -1)
		{
			syslog(LOG_ERR, "read_file() failed");
			freeaddrinfo(serveinfo);
			return -1;
		}
		//printf("Received data %s from clinet length %ld\n\r",recv_data,strlen(recv_data));
		//printf("Sending data %s to clinet length %d\n\r",read_buffer,read_data_len);

		
		//Send data to client that was read from file
		status = send(accepted_sockfd,read_buffer,read_data_len,0);
		if(status == -1)
		{
			syslog(LOG_ERR,"send() failed");
			freeaddrinfo(serveinfo);
			return -1;
		}


		closelog();
	}
	//
	syslog(LOG_ERR,"Closed connection from %s\n\r",inet_ntoa(client_addr.sin_addr)); //inet_ntoa converts raw address into human readable format

	close(sockfd);
	close(accepted_sockfd);

}

