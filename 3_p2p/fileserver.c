#include "const.h"
void add_file(char *filename);
void free_files();
int file_check(char *filename);
void file_checker();

/* Adds a file to this clients file listing */
void add_file(char *filename)
{
	if(file_check(filename) == 1)
	{
		printf("File already registered\n");
		return;
	}
	int i;
	for(i = 0; i < MAXUSRFILES;i++)
	{
		if(files[i] == NULL)
		{
			files[i] = malloc(MAXLINE);
			strcpy(files[i],filename);
			break;
		}
		else if(i == MAXUSRFILES-1)
			printf("File list full\n");
	}
}
/* Frees all files from file listing for this client */
void free_files()
{
	int i;
	for(i = 0; i < MAXUSRFILES;i++)
		if(files[i] != NULL)
			free(files[i]);
}
//Checks if the file is in the file list
int file_check(char *filename)
{
	int i;
	for(i = 0; i < MAXUSRFILES;i++)
	{
		if(files[i] != NULL)
		{
			if(strcmp(filename,files[i]) == 0)	
				return 1;
		}
	}
	return 0;
}
/* Checks if any registered files from client were moved
 * or can no longer be opened. If so remove from central
 * indexing server
 */
void file_checker()
{
	int i;
	while(!done)
	{
		FILE *file;
		for(i = 0; i < MAXUSRFILES; i++)
		{
			if(files[i] != NULL)	
			{
				file = fopen((const char *)files[i],"r");
				if(file == NULL)
				{
					printf("File %s moved or deleted: Updating list\n",files[i]);
					//Tell server to remove from list, if available
					free(files[i]);
					files[i] = NULL;
				}
				else
					fclose(file);
			}
		}
		sleep(UPDATETIME);
	}
}
/* Conects with socket fd cfd to find and send
 * a file to that client */
void send_file(int cfd)
{
		char filename[MAXLINE];
		//Receive the filename, if no clients to accept, return
		if(recv(cfd,(void *)filename,MAXLINE,0) == 0)
		{
			close(cfd);
			return;
		}
		printf("Sending file: %s\n",filename);

		//Send file
		//printf("File would be sent here\n");
		//Open the file
		int fd = open(filename,O_RDONLY); 
		if(fd < 0)
		{
			printf("File couldnt be opened\n");
			close(cfd);
			return;
		}
		struct stat filestat;
		fstat(fd,&filestat);
		char filesize[MAXFILESIZECHARS];
		//Convert fstat filesize to string
		sprintf(filesize, "%d",(int)filestat.st_size);
		//Send the file size
		send(cfd,(void *)filesize,MAXFILESIZECHARS,0);
		off_t len = 0;
		//Send the entire file
		if(sendfile(fd,cfd,0,&len,NULL,0) < 0)
		{
			printf("Error sending file\n");
			close(cfd);
			return;
		}
		printf("File sent\n");
		//close file 
		close(fd);
		//close client connection
		close(cfd);
}
