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
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/queue.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
//***********************************************************************************
//                              Macros
//***********************************************************************************
#define PORT_NUMBER ("9000")
#define FAMILY (AF_INET6) //set the AI FAMILY
#define SOCKET_TYPE (SOCK_STREAM) //set to TCP
#define FLAGS (AI_PASSIVE)
#define IP_ADDR	(0)
#define MAX_PENDING_CONN_REQUEST  (10)
//#define RECV_FILE_NAME ("/var/tmp/aesd_socketdata")
#define TEN_SECOND (10)
#define BAD_FILE_DESCRIPTOR (9)
#define GRACEFUL_EXIT   (2)

#define USE_AESD_CHAR_DEVICE	0
#if (USE_AESD_CHAR_DEVICE == 1)
	#define RECV_FILE_NAME ("/dev/aesdchar")
#else
	#define RECV_FILE_NAME ("/var/tmp/aesdsocketdata")
#endif
//***********************************************************************************
//                              Global variables
//***********************************************************************************
int sockfd = 0;
int accepted_sockfd = 0;
char* read_buffer = NULL;
char * recv_data = NULL;
int fptr = 0;
int read_data_len = 0;
struct sockaddr_in client_addr;
struct addrinfo* serveinfo = NULL;
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
SLIST_HEAD(slisthead, slist_data_s) head = SLIST_HEAD_INITIALIZER(head);

typedef struct 
{
	pthread_t thread_id;
	int accepted_sockfd;
	bool is_thread_complete;
}pthread_params_t;

struct slist_data_s
{
	pthread_params_t thread_params; //data
	SLIST_ENTRY(slist_data_s) entries ; //Pointer to next node
};

typedef struct slist_data_s slist_data_t;
slist_data_t* data_node_p = NULL;

//***********************************************************************************
//                              Function definition
//***********************************************************************************

/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Accept connection
 @param: None
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/

int accept_connection()
{
	//Accept
	memset(&client_addr, 0, sizeof(client_addr));
	socklen_t client_addr_len = 0;

	accepted_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);	
	if(accepted_sockfd == -1)
	{
		//Errno is set to bad file descriptor when shutdown occurs, and it still waits for conneciton
		if(errno == BAD_FILE_DESCRIPTOR)
		{
			return GRACEFUL_EXIT;
		}
		else
		{
			syslog(LOG_ERR,"accept() failed \n\r");
			printf("accept error: %s %d\n",strerror(errno),errno);

			freeaddrinfo(serveinfo);
			return -1;
		}
	}	

	return 0;
}
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
	int fptr = open(RECV_FILE_NAME,O_RDONLY); //use a+ to open already existing file, w to create new file if not exist 
	if (fptr == -1)
	{
		syslog(LOG_ERR,"read_file fptr is NULL");
		return -1;
	}
	else
	{    
		int status = 0;
		char dummy_read_char = 0;
		// int length = lseek(fptr, 0, SEEK_END);
		// if(length == -1)
		// {
		// 	perror("length is -1");
		// 	syslog(LOG_ERR, "read_file fseek seek end failed");
		// 	return -1;
		// }
		// status = lseek(fptr, 0, SEEK_SET);
		// if(status == -1)
		// {
		// 	syslog(LOG_ERR, "read_file fseek seek set failed");
		// 	return -1;
		// }

		printf("\nTest\n");
		// while(read(fptr,&dummy_read_char, 1) != -1)
		// {
		// 	// if(length == 0)
		// 	// {
		// 	// 	*buffer = malloc(1);
		// 	// 	(*buffer)[length] = dummy_read_char;
		// 	// }
		// 	// else
		// 	// {
		// 	// 	*buffer = realloc(*buffer,length+1);
		// 	// 	(*buffer)[length] = dummy_read_char;
		// 	// }
		// 	length++;
		// 	//printf("%c",dummy_read_char);
		// }
		int length = *read_data_len;
		printf("%d",length);
		*buffer = malloc(length);
		if (*buffer == NULL)
		{
			syslog(LOG_ERR,"read_file malloc failed");
			return -1;
		}
		status = read(fptr,*buffer,length);

		if(status != length)
		{
			syslog(LOG_ERR, "read_file fread failed expected length%d actual length%d",length, status);
			return -1;
		}
	}
	close(fptr);

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
		if(fptr != -1)
			close(fptr);

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
		
		slist_data_t* temp = NULL;
		//Delete the linked list
		while (!SLIST_EMPTY(&head)) 
		{          
		/* List Deletion. */
			temp = SLIST_FIRST(&head);
			SLIST_REMOVE_HEAD(&head, entries);
			free(temp);
		}
		closelog();
	}

}
/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief: Timer handler that gets fired every period
 @param: signal: Indicates signal number
 @return: None
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
//Reference: https://www.geeksforgeeks.org/strftime-function-in-c/
#if(USE_AESD_CHAR_DEVICE == 0)
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
		fptr = open(RECV_FILE_NAME,O_APPEND | O_WRONLY); //use a+ to open already existing file, w to create new file if not exist 
		if(fptr == -1)
		{
			syslog(LOG_ERR,"fopen() \n\r");
			freeaddrinfo(serveinfo);
			exit(1);
		}

		//write to file at server end /var/tmp/aesd_socket
		status = write(fptr,&formatted_time[0],timer_string_len);

		close(fptr);
		//Unlock the mutex
		status = pthread_mutex_unlock(&mutex_lock);
		if(status)
		{
			syslog(LOG_ERR,"pthread_mutex_unlock failed");
			exit(1);
		}
	}
}
#endif
/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Initialises timer to fire every time_period second
 @param: time_period: Time period after which timer fires
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
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
			printf("%d",cb_params->accepted_sockfd);
						printf("recv error: %s\n",strerror(errno));
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

	printf("Total: %d",total);
	//open file to write the received data from client
	fptr = open(RECV_FILE_NAME,O_APPEND | O_WRONLY); //use a+ to open already existing file, w to create new file if not exist 
	if(fptr == -1)
	{
		syslog(LOG_ERR,"fopen() line 400 \n\r");
		freeaddrinfo(serveinfo);
		exit(1);
	}

	//write to file at server end /var/tmp/aesd_socket
	status = write(fptr,&recv_data[0],total);
	free(recv_data);
	recv_data = NULL;

	close(fptr);

	
	//Read the data from file /var/tmp/aesd_socket
	read_buffer = NULL;
	read_data_len += total;
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
	//close(sockfd);
	return cb_params;
 }
/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Open socket connection and then bind
 @param: None
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
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
/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Register signal handler
 @param: None
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
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

	#if (USE_AESD_CHAR_DEVICE == 0)
	if(signal(SIGALRM,timer_handler) == SIG_ERR)
	{
		syslog(LOG_ERR,"SIGALARM failed");
		return -1;
	}
	#endif
}
/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Daemonise the process when user passes -d as argument
 @param: arg: Number of argument passed through command line
 @param: argv[]: Pointer that holds the arguments passed through command line
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
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
/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Creates new file
 @param: None
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
int create_new_file()
{
	//open file to write the received data from client
	fptr =  open(RECV_FILE_NAME, O_CREAT | O_RDWR | O_APPEND | O_TRUNC, 0766);
	if(fptr == -1)
	{
		syslog(LOG_ERR,"open() open failed for %s \n\r",RECV_FILE_NAME);
		freeaddrinfo(serveinfo);
		close(fptr);

		return -1;	
	}



	return 0;

}

/*------------------------------------------------------------------------------------------------------------------------------------*/
 /*
 @brief:Initialises mutex lock
 @param: None
 @return: returns -1 on failure, 0 on success
 */
 /*-----------------------------------------------------------------------------------------------------------------------------*/
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

	SLIST_INIT(&head);

	status = initialise_mutex_lock();
	
	#if USE_AESD_CHAR_DEVICE == 0
	status = init_timer(TEN_SECOND);
	#endif
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

		//Accept
		status = accept_connection();
		if(status == -1)
		{
			syslog(LOG_ERR,"accept_connection() failed");
			return -1;
		}
		else if(status == GRACEFUL_EXIT)
		{
			return 0;
		}

		syslog(LOG_ERR,"Accepted connection from %s\n\r",inet_ntoa(client_addr.sin_addr)); //inet_ntoa converts raw address into human readable format

		//Create threads
		data_node_p = malloc(sizeof(slist_data_t));
		if(data_node_p == NULL)
		{
			syslog(LOG_ERR,"main() data_node_p is NULL");
			return -1;
		}

		data_node_p->thread_params.accepted_sockfd = accepted_sockfd;
		data_node_p->thread_params.is_thread_complete = 0;
		
		SLIST_INSERT_HEAD(&head,data_node_p,entries);

		status = pthread_create(&(data_node_p->thread_params.thread_id),NULL,thread_callback,&(data_node_p->thread_params));
		if(status != 0)
		{
			syslog(LOG_ERR,"pthread_create failed");
			return -1;
		}
		
		slist_data_t* temp = NULL;
		SLIST_FOREACH(temp,&head,entries)
		{

			if(temp->thread_params.is_thread_complete == true)
			{
							pthread_join(temp->thread_params.thread_id,NULL);

				// slist_data_t* temp_head = NULL;
				// temp_head = SLIST_FIRST(&head);
				// SLIST_REMOVE_HEAD(&head, entries);
				// printf("\n\rremove free:%lu",sizeof(*temp_head));
				// free(temp_head);
			}
		}


		
	}
	//

		//closing any open sockets, 
		cleanup();
		
		syslog(LOG_ERR,"Closed connection from %s\n\r",inet_ntoa(client_addr.sin_addr)); //inet_ntoa converts raw address into human readable format

}
