#include <unistd.h>
#include <stdlib.h>
#include <list>
#include <string>
#include <cstring>
#include <thread>

void invalidate(const char *filename);
void add_file(const char *filename);
bool file_check(const char *filename);
void validate(const char *filename);
void invalidate_simulator();
void create_threads(int NTHREADS);

typedef struct
{
	std::string filename;
	bool valid;
} file_entry;
std::list<file_entry *> flist;
void add_file(const char *filename)
{
	file_entry *fe = new file_entry;
	fe->filename = std::string(filename);
	fe->valid = true;
}
void client()
{
	int invalidations = 0;
	int total = 0;
	for(int i = 0; i < 100; i++)
	{
		for(std::list<file_entry *>::iterator it = flist.begin(); it != flist.end(); it++)
		{
			total++;
			//If you dont have the file, it was updated
			if(!file_check((*it)->filename.c_str()))
			{
				invalidations++;
				if(rand() % 10 < 5)
					validate((*it)->filename.c_str());
			}
		}
	}
	float percentinvalid = (float)(invalidations/total);
	printf("Number of invalidations: %d\n",invalidations);
	printf("Percent invalid: %%%2f\n",percentinvalid);
	printf("Percent valid: %%%2f\n",100-percentinvalid);
}
int main()
{
	add_file("1kb.txt");
	add_file("2kb.txt");	
	add_file("3kb.txt");	
	add_file("4kb.txt");	
	add_file("5kb.txt");	
	add_file("6kb.txt");	
	add_file("7kb.txt");	
	add_file("8kb.txt");	
	add_file("9kb.txt");	
	add_file("10kb.txt");	
	//Get all files initially
	
}
void create_threads(int nthreads)
{
	pthread_t threads[nthreads];
	int i;
	for(i = 0; i < nthreads; i++)
	{
		if(i == 0)
			pthread_create(&threads[i],NULL,client,NULL);
		else
			pthread_create(&threads[i],NULL,invalidate_simulator,NULL);
	}
	/* Terminate all threads */
	for(i = 0; i < nthreads; i++)
	{
		pthread_join(threads[i],NULL);
	}
}
bool file_check(const char *filename)
{
	for(std::list<file_entry *>::const_iterator it = flist.begin(); it != flist.end(); it++)
	{
		if((*it)->valid == false)
			return false;
	}
	return true;
}
void invalidate_simulator()
{
	for(std::list<file_entry *>::iterator it = flist.begin(); it != flist.end(); it++)
	{
		sleep(1);
		invalidate((*it)->filename.c_str());
	}
}
void validate(const char *filename)
{
	for(std::list<file_entry *>::iterator it = flist.begin(); it != flist.end(); it++)
	{
		if(strcmp((*it)->filename.c_str(),filename)==0)
		{
			(*it)->valid = true;
		}
	}
}
void invalidate(const char *filename)
{
	for(std::list<file_entry *>::iterator it = flist.begin(); it != flist.end(); it++)
	{
		if(strcmp((*it)->filename.c_str(),filename)==0)
		{
			(*it)->valid = false;
		}
	}
}
