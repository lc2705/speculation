/*
**   AC.H 
**
**
*/
#ifndef AC_H
#define AC_H

/*
*   Prototypes
*/
#define ALPHABET_SIZE    256     

#define ACSM_FAIL_STATE   -1     

typedef struct _acsm_pattern {      

    struct  _acsm_pattern *next;
    unsigned char         *patrn;
    int     n;
    int* 	nmatch_array;
} ACSM_PATTERN;


typedef struct  {    

    /* Next state - based on input character */
    int      NextState[ ALPHABET_SIZE ];  

    /* Failure state - used while building NFA & DFA  */
    int      FailState;   

    /* List of patterns that end here, if any */
    ACSM_PATTERN *MatchList;   
    
    /*used for validation stage */
    int 	Depth;

}ACSM_STATETABLE; 


/*
* State machine Struct
*/
typedef struct {
	int acsmNumThread;

    int acsmMaxStates;  
    int acsmNumStates;  

    ACSM_PATTERN    * acsmPatterns;
    ACSM_STATETABLE * acsmStateTable;

}ACSM_STRUCT;

/*
*   Prototypes
*/
ACSM_STRUCT * acsmNew (int numThread);
int acsmAddPattern( ACSM_STRUCT * p, unsigned char * pat, int n);
int acsmCompile ( ACSM_STRUCT * acsm );
int acsmSearch ( ACSM_STRUCT * acsm,unsigned char * Tx, int *beg_state,int rank,void (*PrintMatch) (ACSM_PATTERN * pattern, ACSM_PATTERN * mlist, int rank));
int acsmSearchWithDepthCompare (ACSM_STRUCT * acsm, unsigned char *Tx, int *beg_state,int dist,int rank,void (*PrintMatch) (ACSM_PATTERN * pattern , ACSM_PATTERN * mlist, int rank)) ;
void acsmFree ( ACSM_STRUCT * acsm );
void PrintMatch (ACSM_PATTERN * pattern,ACSM_PATTERN * mlist,int rank) ;
void PrintSummary (ACSM_STRUCT * acsm) ;

#endif
