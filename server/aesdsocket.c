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
#include <sys/queue.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>

//***********************************************************************************
//                              Macros
//***********************************************************************************
#define PORT_NUMBER ("9000")
#define FAMILY (AF_INET6) //set the AI FAMILY
#define SOCKET_TYPE (SOCK_STREAM) //set to TCP
#define FLAGS (AI_PASSIVE)
#define IP_ADDR	(0)
#define MAX_PENDING_CONN_REQUEST  (10)
#define RECV_FILE_NAME ("/var/tmp/aesd_socketdata")
#define TEN_SECOND (10)

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
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;


typedef struct 
{
	pthread_t thread_id;
	int accepted_sockfd;
	bool is_thread_complete;
}pthread_params_t;

typedef struct slist_data_s
{
	pthread_params_t thread_params; //data
	SLIST_ENTRY(slist_data_s) entries ; //Pointer to next node
}slist_data_t;
slist_data_t* data_node_p = NULL;

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

//Reference: https://www.geeksforgeeks.org/strftime-function-in-c/
void timer_handler(int signal)
{
	if(signal == SIGALRM)
	{
		int status = 0;
		//Get time in seconds
		time_t t ;
    	time(&t);
		
		//Convert time in seconds to proper formatted time
  		//localtime() uses the time pointed by t ,
    	// to fill a tmp structure with the
    	// values that represent the
    	// corresponding local time.
   		struct tm *tmp = NULL;
    	tmp = localtime(&t);

		//Convert time into string
    	// using strftime to display time
		char formatted_time[50];
		int timer_string_len = 0;
    	timer_string_len = strftime(formatted_time, sizeof(formatted_time), "timestamp:%d.%b.%y - %k:%M:%S\n", tmp);

		//applying a mutex lock to protect file from race condition by multiple client 
		status = pthread_mutex_lock(&mutex_lock);
		if(status)
		{
			syslog(LOG_ERR,"pthread_mutex_lock failed");
			exit(1);
		}
		//Write to file
		//open file to write the received data from client
		fptr = fopen(RECV_FILE_NAME,"a"); //use a+ to open already existing file, w to create new file if not exist 
		if(fptr == NULL)
		{
			syslog(LOG_ERR,"fopen() \n\r");
			freeaddrinfo(serveinfo);
			exit(1);
		}

		//write to file at server end /var/tmp/aesd_socket
		status = fwrite(&formatted_time[0],1,timer_string_len,fptr);
		
		fclose(fptr);
		fptr = NULL;
		//Unlock the mutex
		status = pthread_mutex_unlock(&mutex_lock);
		if(status)
		{
			syslog(LOG_ERR,"pthread_mutex_unlock failed");
			exit(1);
		}
	}
}

int init_timer(time_t time_period)
{
	int status = 0;
	struct itimerval timer_attribute;
	timer_attribute.it_interval.tv_sec = time_period; //timer interval of 10 secs
	timer_attribute.it_interval.tv_usec = 0;
	timer_attribute.it_value.tv_sec = time_period; //time expiration of 10 secs
	timer_attribute.it_value.tv_usec = 0;

	status = setitimer(ITIMER_REAL, &timer_attribute, NULL);
	if(status == -1){
		syslog(LOG_ERR,"setitimer() failed");
		return -1;
	}

	return 0;
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
 @brief: Callback function that gets called when pthread is created
 @param: thread_param: Input structure to the callback function 
 @return: void* 
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
 void* thread_callback(void* thread_param)
 {
	 int status = 0;
	if(thread_param == NULL)
	{
		syslog(LOG_ERR,"thread_param is NULL");
		exit(1);
	}

	pthread_params_t* cb_params = (pthread_params_t *) thread_param;

	//applying a mutex lock to protect file from race condition by multiple client 
	status = pthread_mutex_lock(&mutex_lock);
	if(status)
	{
		syslog(LOG_ERR,"pthread_mutex_lock failed");
		exit(1);
	}
	//Receive data from client
	const int recv_len = 100;
	recv_data = malloc(sizeof(char) * recv_len);
	memset(recv_data,0,recv_len*sizeof(char));

	int n = 1, total = 0, found = 0;
	while (!found) 
	{	

		//reading from the client
		n = recv(cb_params->accepted_sockfd, &recv_data[total], recv_len, 0);
		if (n == -1) 
		{
			syslog(LOG_ERR,"\n\rrecv failed ");
			freeaddrinfo(serveinfo);
			exit(1);
		}
		total += n;
		found = (recv_data[total - 1] == '\n')?(1):(0);
		recv_data = realloc(recv_data, total + recv_len);
		if(recv_data == NULL)
		{
			syslog(LOG_ERR,"realloc failed()");
			freeaddrinfo(serveinfo);
			exit(1);
		}
	}


	//open file to write the received data from client
	fptr = fopen(RECV_FILE_NAME,"a"); //use a+ to open already existing file, w to create new file if not exist 
	if(fptr == NULL)
	{
		syslog(LOG_ERR,"fopen() \n\r");
		freeaddrinfo(serveinfo);
		exit(1);
	}

	//write to file at server end /var/tmp/aesd_socket
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
		exit(1);
	}
	//printf("Received data %s from clinet length %ld\n\r",recv_data,strlen(recv_data));
	//printf("Sending data %s to clinet length %d\n\r",read_buffer,read_data_len);

	
	//Send data to client that was read from file
	status = send(cb_params->accepted_sockfd,read_buffer,read_data_len,0);
	if(status == -1)
	{
		syslog(LOG_ERR,"send() failed");
		freeaddrinfo(serveinfo);
		exit(1);
	}
	free(read_buffer);
	read_buffer = NULL;
	

	//unlock the mutex 
	status = pthread_mutex_unlock(&mutex_lock);
	if(status){
		syslog(LOG_ERR,"pthread_mutex_unlock failed");
		exit(1);
	}

	cb_params->is_thread_complete = true;
	close(cb_params->accepted_sockfd);

	return cb_params;
 }

int open_socket_then_bind()
{
	int status = 0;
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
		freeaddrinfo(serveinfo);
		return -1;
	}
	return 0;
}

int register_signal_handler()
{
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

	if(signal(SIGALRM,timer_handler) == SIG_ERR)
	{
		syslog(LOG_ERR,"SIGALARM failed");
		return -1;
	}
}

int daemonise_process(int argc ,char* argv[])
{
	int status = 0;
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

	return 0;
}

int create_new_file()
{
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

	return 0;

}


int initialise_mutex_lock()
{
	int status = 0;
	
	status  = pthread_mutex_init(&mutex_lock, NULL);
	if(status != 0)
	{	
		syslog(LOG_ERR,"pthread_mutex_init failed");
		return -1;
	}
	return 0;
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
	if(argc > 2)
	{
		syslog(LOG_ERR,"Number of arguments passed are greater than 2");
		return -1;
	}

	openlog("AESD_SOCKET", LOG_DEBUG, LOG_DAEMON); //check /var/log/syslog

	int status = 0;

	status = register_signal_handler();
	if(status == -1)
	{
		syslog(LOG_ERR,"register_signal_handler");
		return -1;
	}
	
	status = open_socket_then_bind();
	if(status == -1)
	{
		syslog(LOG_ERR,"open_socket_then_bind failed");
		return -1;
	}
	
	status = daemonise_process(argc ,argv);
	if(status == -1)
	{
		syslog(LOG_ERR, "daemonise_process failed");
		return -1;
	}

	status = create_new_file();
	if(status == -1)
	{
		syslog(LOG_ERR,"create_new_file failed");
		return -1;
	}

	SLIST_HEAD(slisthead, slist_data_s) head;
	SLIST_INIT(&head);

	status = initialise_mutex_lock();
	
	


	status = init_timer(TEN_SECOND);
	if(status == -1)
	{
		syslog(LOG_ERR,"init_timer() failed");
		return -1;
	}

	while(1)
	{
		//Listen
		status = listen(sockfd,MAX_PENDING_CONN_REQUEST) ;
		if(status == -1)
		{
			syslog(LOG_ERR,"listen() failed\n\r");
			freeaddrinfo(serveinfo);
			return -1;
		}

		syslog(LOG_ERR,"Waiting for connection from client() \n\r");
		
		//Accept
		//struct sockaddr client_addr; 
		memset(&client_addr, 0, sizeof(client_addr));
		socklen_t client_addr_len = 0;

		accepted_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);	
		if(accepted_sockfd == -1)
		{
			syslog(LOG_ERR,"accept() \n\r");
			// printf("accept failed\n\r");
			freeaddrinfo(serveinfo);
			return -1;
		}	
		syslog(LOG_ERR,"Accepted connection from %s\n\r",inet_ntoa(client_addr.sin_addr)); //inet_ntoa converts raw address into human readable format

		//Create threads
		data_node_p = malloc(sizeof(slist_data_t));
		
		if(data_node_p == NULL)
		{
			syslog(LOG_ERR,"main() data_node_p is NULL");
			return -1;
		}
		SLIST_INSERT_HEAD(&head,data_node_p,entries);
		data_node_p->thread_params.accepted_sockfd = accepted_sockfd;
		data_node_p->thread_params.is_thread_complete = 0;

		status = pthread_create(&(data_node_p->thread_params.thread_id),NULL,thread_callback,&data_node_p->thread_params);
		if(status != 0)
		{
			syslog(LOG_ERR,"pthread_create failed");
			return -1;
		}

		SLIST_FOREACH(data_node_p,&head,entries)
		{
			pthread_join(data_node_p->thread_params.thread_id,NULL);

			if(data_node_p->thread_params.is_thread_complete == true){

				data_node_p = SLIST_FIRST(&head);
				SLIST_REMOVE_HEAD(&head, entries);
				free(data_node_p);
			}

		}


		
		


	}
	//

		//closing any open sockets, 
		close(sockfd);
		close(accepted_sockfd);
		
		syslog(LOG_ERR,"Closed connection from %s\n\r",inet_ntoa(client_addr.sin_addr)); //inet_ntoa converts raw address into human readable format

}