/********************************************************************
 * @file aesdsocket.c
 * @brief Server implementation for socket IPC
 * @Description : Connects with client, receives data, writes to file and send data to client
 * @author Sayali Mule
 * @date  February 19, 2021
 * @Reference:
 **********************************************************************/
//***********************************************************************************
//                              Include files
//***********************************************************************************
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
#include<errno.h>

//***********************************************************************************
//                              Macros
//***********************************************************************************
#define PORT_NUMBER ("9000")
#define FAMILY (AF_INET6) //set the AI FAMILY
#define SOCKET_TYPE (SOCK_STREAM) //set to TCP
#define FLAGS (AI_PASSIVE)
#define IP_ADDR	(0)
#define MAX_PENDING_CONN_REQUEST  (3)
#define RECV_FILE_NAME ("/var/tmp/aesd_socketdata")

//***********************************************************************************
//                              Global variables
//***********************************************************************************
int sockfd = 0;
int accepted_sockfd = 0;
char* read_buffer = NULL;
char * recv_data = NULL;
FILE* fptr = NULL;
struct sockaddr_in client_addr;
struct addrinfo* serveinfo = NULL;

//***********************************************************************************
//                              Function definition
//***********************************************************************************

 /*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Populates the serveinfo pointer
 @param: serveinfo: Pointer to structure used during binding 
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
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
/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Reads files on server end
 @param: buffer: pointer in which files contents are read
 #param: read_data_len: Populates the length of file 
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
int read_file(char** buffer,int* read_data_len)
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
	fptr = NULL;
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Handler to signal
 @param: signal: signal number
 @return: None
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
void sighandler(int signal)
{
	if(signal == SIGINT || signal == SIGTERM)
	{


		//completing any open connection operations, 
		if(fptr != NULL)
			fclose(fptr);

		//closing any open sockets, 
		close(sockfd);
		close(accepted_sockfd);
		
		syslog(LOG_ERR,"Closed connection from %s\n\r",inet_ntoa(client_addr.sin_addr)); //inet_ntoa converts raw address into human readable format

		//free malloced buffer
		if(read_buffer != NULL)
			free(read_buffer);
		
		if(recv_data != NULL)
			free(recv_data);

		//and deleting the file /var/tmp/aesdsocketdata.
		remove(RECV_FILE_NAME);

		closelog();
	}

}

/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief: Closes file handler, socket and does cleanup
 @param: None
 @return: None
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
void cleanup()
{
	sighandler(0xff); //0xff is dummy byte
}

/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief: Main driver function
 @param: argc : Count of argument passed from command line
 @param: argv: Array of pointer to command line arguments
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
int main(int argc ,char* argv[])
{

	openlog("AESD_SOCKET", LOG_DEBUG, LOG_DAEMON); //check /var/log/syslog

	int status = 0;
	if(signal(SIGINT,sighandler) == SIG_ERR)
	{
		syslog(LOG_ERR,"SIGINT registration failed");
		return -1;
	}

	if(signal(SIGTERM,sighandler) == SIG_ERR)
	{
		syslog(LOG_ERR,"SIGTERM registration failed");
		return -1;
	}
	
	if(argc > 2)
	{
		syslog(LOG_ERR,"Number of arguments passed are greater than 2");
		return -1;
	}


	//get address info	
	serveinfo = NULL;
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
	
	status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR| SO_REUSEPORT, &(int){1}, sizeof(int));
	if(status == -1)
	{
		syslog(LOG_ERR,"setsockopt failed");
		return -1;
	}
	//After creation of the socket, bind function binds the socket to address and port number
	status = bind(sockfd, serveinfo->ai_addr,  serveinfo->ai_addrlen);
	if(status == -1 )        
	{
		syslog(LOG_ERR,"bind() failed\n\r");
		printf("bind error: %s\n",strerror(errno));
		freeaddrinfo(serveinfo);
		return -1;
	}
	
	//Daemonize the process after bind is successfull
	if(argc >= 2 && argv[1] != NULL && (strcmp(argv[1],"-d") == 0))
	{
	/* Reference: https://stackoverflow.com/questions/2966886/is-there-a-difference-calling-daemon0-0-from-within-a-program-and-launching-a
    	Re-parents the process to init by forking and then exiting the parent. Look in the ps list and you'll see that daemons are owned by PID 1.
    	Calls setsid().
    	Changes directory to /.
    	Redirects standard in, out, and error to /dev/null.
	*/

		status = daemon(0,0);
		if(status == -1)
		{
			syslog(LOG_ERR,"daemon failed");
			return -1;
		}
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
	fptr = NULL;

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
		recv_data = NULL;

		fclose(fptr);
		fptr = NULL;
		//Read the data from file /var/tmp/aesd_socket
		
		read_buffer = NULL;
		int read_data_len = 0;
		status = read_file(&read_buffer,&read_data_len);
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
		free(read_buffer);
		read_buffer = NULL;
		cleanup();
	}
	//



}

