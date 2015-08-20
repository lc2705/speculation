
#define _GNU_SOURCE
#include "ac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sched.h>

#define MAXLEN 2500
#define MAXLINE 40000
#define MAXPATTERNLEN 202
#define THREAD_NUM 1
/*
*  Text Data Buffer
*/

unsigned char text[MAXLINE][MAXLEN];   
unsigned char pattern[MAXPATTERNLEN]; 
unsigned int line = 0;
unsigned int *valid_len_array;
unsigned int max_pattern_len = 0;

unsigned int        thread_num = THREAD_NUM;
pthread_t           *thread_array;
ACSM_STRUCT         *acsm;
pthread_barrier_t   barrier_validation;
pthread_barrier_t	barrier_thread;

/*sentinel*/
static int history[MAXLEN] = { ACSM_FAIL_STATE };

void* SearchThread(void* args);

int main (int argc, char **argv) 
{
    int i;
    unsigned int total_len = 0;
    struct timeval begtime,endtime;
    FILE *sfd,*pfd;
    char sfilename[20] = "string";
    char pfilename[20] = "pattern";

	
//=============================================== 
    if (argc < 4)
    {
        fprintf (stderr,"Usage: acsmx stringfile patternfile ...  -nocase\n");
        exit (0);
    }
    strcpy (sfilename, argv[1]);
    sfd = fopen(sfilename,"r");
    if(sfd == NULL)
    {
        fprintf(stderr,"Open file error!\n");
        exit(1);
    }

    strcpy(pfilename,argv[2]);
    pfd = fopen(pfilename,"r");
    if(sfd == NULL)
    {
        fprintf(stderr,"Open file error!\n");
        exit(1);
    }
    thread_num = atoi(argv[3]);
   	acsm = acsmNew (thread_num); 
   
//read patterns    
	i = 0;
    while(fgets(pattern,MAXPATTERNLEN,pfd))
    {
    	int len = strlen(pattern);
    	acsmAddPattern (acsm, pattern, len-1);
        if(max_pattern_len < len-1) 
            max_pattern_len = len-1;
		i++;
    }
    fclose(pfd);
//    printf("\n\nread %d patterns\n\n===============================",i);
    /* Generate GtoTo Table and Fail Table */
    acsmCompile (acsm);
//========================================================= 

    /*read string*/
    for(i = 0;i < MAXLINE;i++)
    {
    	if(!fgets(text[i],MAXLEN,sfd))
    		break;
   		total_len += strlen(text[i]) - 1; //ignore the last char '\n'
    }
    line = i;
    fclose(sfd);
//    printf("%d lines\t%d bytes",line,total_len);
    
    //create multi_thread
    thread_array = calloc(thread_num,sizeof(pthread_t));
	valid_len_array = calloc(thread_num,sizeof(unsigned int));
    pthread_barrier_init(&barrier_thread,NULL,thread_num);
    pthread_barrier_init(&barrier_validation,NULL,thread_num);

    gettimeofday(&begtime,0);
    for(i = 0;i < thread_num; i++)
	{
		pthread_create(&thread_array[i], NULL, SearchThread, (void*)i);
    }
//=========================================================== 
    int err;
    for(i = 0;i < thread_num;i++)
    {
        err = pthread_join(thread_array[i],NULL);
        if(err != 0)
        {
            printf("can not join with thread %d:%s\n", i,strerror(err));
        }
    }
    gettimeofday(&endtime,0);

    PrintSummary(acsm);
    acsmFree (acsm);

    printf ("%s\t",sfilename);
    printf("Time Cost: %lu us\n",(endtime.tv_sec - begtime.tv_sec)*1000000 + (endtime.tv_usec - begtime.tv_usec));
    for(i = 0;i < thread_num;i++)
        printf("rank%d\t%u\n",i,valid_len_array[i]);
    
    free(thread_array);
    free(valid_len_array);
    pthread_barrier_destroy(&barrier_thread);
    pthread_barrier_destroy(&barrier_validation);
    return 0;
}

void* SearchThread(void* args)
{
	unsigned int rank = (unsigned int)args;
	int iline = 0,state;
    unsigned int index,index_end,len,vlen;
    unsigned char active; // used by validation stage
    unsigned nthread;

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(rank,&mask);
    if(pthread_setaffinity_np(pthread_self(),sizeof(mask),&mask) < 0)
    {
        fprintf(stderr,"thread set affinity failed  \n");
        exit(1);
    }

	while(iline < line)
	{	
		pthread_barrier_wait(&barrier_thread);
		state = 0;
        active = 1;
		vlen = 0;
        nthread = thread_num;
		len = strlen(text[iline]) - 1;
        while(len/nthread < max_pattern_len && nthread > 1)
        {
            nthread--;
        }
        if(rank >= nthread)
        {
   //        printf("rank %d\t nthread %d\t line %d\n",rank,nthread,iline);
            iline++;
            pthread_barrier_wait(&barrier_validation);
            continue;
        }

		index = len/nthread * rank;
		if(rank != nthread - 1)
			index_end = index + len/nthread;
		else
			index_end = len;
		for( ;index < index_end; index++)
	    {   
	        acsmSearch(acsm,&text[iline][index],&state,rank,PrintMatch);
	        history[index] = state;
	        vlen++;
	    }
        
        pthread_barrier_wait(&barrier_validation);
        if(rank != nthread - 1 )
	 	{
            vlen = 0;
            while(active)
            {
                acsmSearch(acsm,&text[iline][index],&state,rank,PrintMatch);
                if(history[index] == state || index == index_end - 1)
                    active = 0;
                else
                {
                    history[index] = state;
                    index++;
                    vlen++;
                }
            }
	 	}
		valid_len_array[rank] += vlen;	
        iline++;
	}   
}
