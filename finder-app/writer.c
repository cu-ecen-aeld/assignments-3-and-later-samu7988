/***********************************************************************************
* @file writer.c
 * @brief: Writes a string to a file
 *         User passes two arguments namely 
 *	   first argument: filename
 *        second argument: string to be written     	
 * @author Sayali Mule
 * @date 01/22/2022
 * @Reference:
 *
 *****************************************************************************/
//***********************************************************************************
//                              Include files
//***********************************************************************************
#include<stdio.h>
#include <string.h>
#include <syslog.h>
//***********************************************************************************
//                                  Macros
//***********************************************************************************
//***********************************************************************************
//                              Structures
//***********************************************************************************


//***********************************************************************************
//                                  Function definition
//***********************************************************************************


/*-----------------------------------------------------------------------------------------------------------------------------*/
/*
 @brief: Writes string to file
 @param: 
 @return: 
 */
/*-----------------------------------------------------------------------------------------------------------------------------*/
int main(char argc, char* argv[])
{

	openlog("WRITER", LOG_DEBUG, LOG_DAEMON);
	if(argc == 1)
	{
		syslog(LOG_ERR,"No arguments passed\n\r");
		printf("no arguments passed\n\r");
		return 1;
	}
	
	for(int i=0; i< argc; i++)
	{
	
		if(argv[i] == NULL)
		{
			printf("argv[%d] is NULL\n\r",i);
			syslog(LOG_ERR,"argv[%d] is NULL \n\r",i);
			return 1;
		}
	}


	FILE* fp = NULL;
	int file_status = 0;
	
	char* file_name = argv[1];
	fp = fopen(file_name, "w+");
	
	if(fp == NULL)
	{
		printf("fopen() failed\n\r");
		syslog(LOG_ERR,"fopen() failed\n\r");
		return 1;
	}
	
    char*  string_to_be_written = argv[2];
	file_status = fputs( string_to_be_written, fp);
	syslog(LOG_INFO, "Writing string %s to file %s",string_to_be_written,file_name);

	
	if(file_status == EOF)
	{
		printf("fputs() failed\n\r");
		syslog(LOG_ERR,"fputs() failed\n\r");
		return 1;
	}
	

	file_status = fclose(fp);
	if(file_status == EOF)
	{
		printf("fclose failed\n\r");
		syslog(LOG_ERR,"fclose() failed\n\r");
		return 1;
	}
	
	closelog();
	return 0;
	
}


