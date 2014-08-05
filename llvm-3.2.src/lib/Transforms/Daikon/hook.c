/**
 * \author Arijit Chattopadhyay
 */


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#define TRUE 1
#define FALSE 0

#define SIZE 100
#define BIGSIZE 1000
#define SMALL 30

typedef int bool;

static __thread int callStackCounbter  = 0 ;
static const char *fileName = "program.dtrace";
FILE *fp;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static __thread int threadId;

//This contains the ower-threadid from inpect;
static int currentLockOwerId = -1; 
static bool isCurrentOwnerOfLock();
//Use this api to lock unlock instead of the 
//pthread_mutex_lock or
//pthread_mutex_unlock
void std_lock();
void std_unlock();

//Inspect Linking
extern int getThreadId(pthread_t *threadStr);

//Inspect Linking ends
						
static int flagToWriteVersionIntoDtrace = 0;

//Local Utility Functions
static void dump_basic_data_types(FILE *fp,void *data,char *varName,char *varType);
static void dump_pointer_data_types(FILE *fp,void *data,char *varName,char *varType);
static void dump_array_data_types(FILE *fp,void *data,char *varName,char *varType, size_t size);
//static void dump_pointer_data_types)


void writeInfoIntoDtrace() {
	flagToWriteVersionIntoDtrace = 1;
	fp = fopen(fileName,"a");
	const char *data = "input-language C/C++\ndecl-version 2.0\nvar-comparability none\n";
	if(fp != NULL) {
		fputs(data,fp);
		fputs("\n",fp);
		fclose(fp);
	}
}

//int getRunCount() ;
//void writeCounter() {
//	FILE *run_count_fp = fopen("/home/ari/cvs/src/run_counter","w+");
//	char data_buffer[SMALL];
//	if( run_count_fp  != NULL) {
//		int result  = getRunCount();
//		result = result+1;
//		memset(data_buffer,'\0',SMALL);
//		sprintf(data_buffer,"%d",result);
//		fputs(data_buffer,run_count_fp);
//	}
//}
//int getRunCount() {
//	static int flag = 0 ;
//	static int result = -1;
//	if(flag == 0) {
//		atexit(writeCounter);
//		flag =1;
//	}
//	FILE *run_count_fp = fopen("/home/ari/cvs/src/run_counter","r");
//	char data_buffer[SMALL];
//	memset(data_buffer,'\0',SMALL);
//	if( fgets(data_buffer,SMALL,run_count_fp)  != NULL) {
//		memset(data_buffer,'\0',SMALL);
//		sscanf(data_buffer,"%d",&result);
//		flag = 1;
//		return result;
//	}
//	return -1;
//
//}

void clap_hookBefore(int varCount, ...) {
	va_list vararg;
	va_start(vararg,varCount);
	int i;
	for( i = 0 ; i < varCount ;++i) {
		int *data = va_arg(vararg,int*);
		char *varName = va_arg(vararg,char*);
		printf("The varaiable %s Before is %d\n",varName,*data);

	}
}






//Must be called after acquiring a lock
void write_thread_id(FILE *fp) {
	pthread_t tt = pthread_self();
	int id = getThreadId(&tt);
	char buffer[SMALL];
	memset(buffer,'\0',SMALL);
	sprintf(buffer,"%s::%d","ThreadId::",id);
	fputs(buffer,fp);
	fputs("\n",fp);
}

void clap_hookFuncBegin(int varCount, ...) {
	std_lock();
	//pthread_mutex_lock(&lock);

	if(flagToWriteVersionIntoDtrace == 0) {
		writeInfoIntoDtrace();
	}
	fp = fopen(fileName,"a");
	if(fp != NULL) {
		write_thread_id(fp);
		char buffer[SMALL];
		va_list vararg;
		va_start(vararg,varCount);
		char *funcName = va_arg(vararg,char*);
#if 0		
		printf("\n\nEntering into Function Name : %s\n",funcName);
#endif		
		fputs(funcName,fp);
		fputs("\n",fp);
		fputs("this_invocation_nonce",fp);
		fputs("\n",fp);
		memset(buffer,'\0',SMALL);
		sprintf(buffer,"%d",callStackCounbter);
		fputs(buffer,fp);
		fputs("\n",fp);
		int i ;
		for( i = 0 ; i < varCount ;++i) {
			char *varName = va_arg(vararg,char*);
			char *varType = va_arg(vararg,char*);
			if(varName[0] == ':') {
				void *data = va_arg(vararg,void*);
				//Handle the pointer case separately
				if(strstr(varType,"*")!= NULL) {
					dump_pointer_data_types(fp,data,varName,varType);
				}
				else if(strstr(varType,"[]") != NULL)
				{
					size_t size = va_arg(vararg, size_t);	
					
					dump_array_data_types(fp, data, varName, varType, size);
				}
				
				else {
					dump_basic_data_types(fp,data,varName,varType);
				}
			}

#if 0			
			else if(strcmp(varType,"int") ==0 ){
				int data = va_arg(vararg,int);
#ifdef DARIJIT
				printf("The parameter %s at beginning is %d of type %s:\n",varName,data,varType);
#endif
				fputs(varName,fp);
				fputs("\n",fp);
				memset(buffer,'\0',SMALL);
				sprintf(buffer,"%d",data);
				fputs(buffer,fp);
				fputs("\n",fp);
				fputs("1\n",fp);
			}

#endif


		}
		fputs("\n",fp);
		fclose(fp);
		++callStackCounbter;
	}
	std_unlock();
	//pthread_mutex_unlock(&lock);
}


void clap_hookFuncEnd(int varCount, ...) {
	std_lock();
	//pthread_mutex_lock(&lock);
	fp = fopen(fileName,"a");
	if(fp != NULL) {
		--callStackCounbter;
		write_thread_id(fp);
		char buffer[SMALL];
		va_list vararg;
		va_start(vararg,varCount);
		char *funcName = va_arg(vararg,char*);
		printf("\n\nExiting from Function Name : %s\n",funcName);
		fputs(funcName,fp);
		fputs("\n",fp);
		fputs("this_invocation_nonce",fp);
		fputs("\n",fp);
		memset(buffer,'\0',SMALL);
		sprintf(buffer,"%d",callStackCounbter);
		fputs(buffer,fp);
		fputs("\n",fp);
		int i ;
		for( i = 0 ; i < varCount ;++i) {
			char *varName = va_arg(vararg,char*);
			char *varType = va_arg(vararg,char*);
			if(varName[0] == ':') {
				void *data = va_arg(vararg,void*);
				//Handle the pointer case separately
				if(strstr(varType,"*")!= NULL) {
					dump_pointer_data_types(fp,data,varName,varType);
				}else {
					dump_basic_data_types(fp,data,varName,varType);
				}
			}

#if 0			
			
			if(varName[0] == ':') {
				int *data = va_arg(vararg,int*);
				printf("The parameter %s at end is %d of type %s:\n",varName,*data,varType);
				fputs(varName,fp);
				fputs("\n",fp);
				memset(buffer,'\0',SMALL);
				sprintf(buffer,"%d",*data);
				fputs(buffer,fp);
				fputs("\n",fp);
				memset(buffer,'\0',SMALL);
				fputs("1\n",fp);
			}
			else if(strcmp(varType,"int") ==0 ){
				int data = va_arg(vararg,int);
				printf("The parameter %s at end is %d of type %s:\n",varName,data,varType);
				fputs(varName,fp);
				fputs("\n",fp);
				memset(buffer,'\0',SMALL);
				sprintf(buffer,"%d",data);
				fputs(buffer,fp);
				fputs("\n",fp);
				fputs("1\n",fp);
			}

#endif
		}
		fputs("\n",fp);
		fclose(fp);
	}
	std_unlock();
	//pthread_mutex_unlock(&lock);
}



void clap_chcHook(int varCount, ...) {
	std_lock();
	//pthread_mutex_lock(&lock);
	FILE *fp = fopen(fileName,"a");
	va_list vararg;
	va_start(vararg,varCount);
	int entryOrExit = va_arg(vararg,int);
	char *varName = va_arg(vararg,char*);
	char *type = va_arg(vararg,char*);
	int *value = va_arg(vararg,int*);
	char *funcName = va_arg(vararg,char*);
	pthread_t thrd = pthread_self();
	int threadId = getThreadId(&thrd);
 //       printf("Yoyo Entry or Exit : %d : Name : %s : Type : %s : value %d from function : %s : thread id  : %d\n",entryOrExit,varName,type,*value,funcName,threadId);

	if(fp != NULL) {
                char buffer[BIGSIZE];

                //Write Thread Id
		fputs("ThreadId::::",fp);
		memset(buffer,'\0',BIGSIZE);
		sprintf(buffer,"%d\n",threadId);
		fputs(buffer,fp);

		//Write the entry or exit section
		memset(buffer,'\0',BIGSIZE);
		strcat(buffer,"..");
		strcat(buffer,funcName);
		if(entryOrExit == 1) {//Entry case
			strcat(buffer,"():::ENTER\n");
		}else if(entryOrExit == 0) {
			strcat(buffer,"():::EXIT0\n");
		}
		fputs(buffer,fp);

		//Write the constant (hard-coded)
		fputs("this_invocation_nonce\n",fp);

		if(entryOrExit == 0) {
			--callStackCounbter;
		}

		//Write the call stack counter
		memset(buffer,'\0',BIGSIZE);
		sprintf(buffer,"%d",callStackCounbter);		
		fputs(buffer,fp);
		fputs("\n",fp);

                //Write Variable Name
		memset(buffer,'\0',BIGSIZE);
		strcat(buffer,"::");
		strcat(buffer,varName);
		fputs(buffer,fp);
		fputs("\n",fp);

                //Write the value of the variable
		memset(buffer,'\0',BIGSIZE);
		sprintf(buffer,"%d",*value);
		fputs(buffer,fp);
		fputs("\n",fp);

		//Write Constant 1 (hard coded)
		memset(buffer,'\0',BIGSIZE);
		fputs("1\n\n",fp);

		if(entryOrExit == 1) {
			++callStackCounbter;
		}
		fclose(fp);
	}
	std_unlock();
	//pthread_mutex_unlock(&lock);
	
}



void clap_chcHookDynamic(int varCount, ...) {
	std_lock();
	FILE *fp = fopen(fileName,"a");
	va_list vararg;
	va_start(vararg,varCount);
	int entryOrExit = va_arg(vararg,int);
	int numOfVars  =  va_arg(vararg,int);
	char *funcName = va_arg(vararg,char*);
        if(fp != NULL) {

                char buffer[BIGSIZE];

                //Write Thread Id
        	pthread_t thrd = pthread_self();
        	int threadId = getThreadId(&thrd);
        	fputs("ThreadId::::",fp);
        	memset(buffer,'\0',BIGSIZE);
        	sprintf(buffer,"%d\n",threadId);
        	fputs(buffer,fp);

        	//Write the entry or exit section
        	memset(buffer,'\0',BIGSIZE);
        	strcat(buffer,"..");
        	strcat(buffer,funcName);
        	if(entryOrExit == 1) {//Entry case
        		strcat(buffer,"():::ENTER\n");
        	}else if(entryOrExit == 0) {
        		strcat(buffer,"():::EXIT0\n");
        	}
        	fputs(buffer,fp);

        	//Write the constant (hard-coded)
        	fputs("this_invocation_nonce\n",fp);

        	//if(entryOrExit == 0) {
        	//	--callStackCounbter;
        	//}

        	//Write the call stack counter
        	memset(buffer,'\0',BIGSIZE);
        	sprintf(buffer,"%d",callStackCounbter);		
        	fputs(buffer,fp);	
        	fputs("\n",fp);

        	//Write Variable Information
        	int i ;
        	for(i = 0 ;  i < numOfVars;  ++i) {
        		char *varName = va_arg(vararg,char*);
        		char *type = va_arg(vararg,char*);
        		int *value = va_arg(vararg,int*);

        		//Write Variable Name
        		memset(buffer,'\0',BIGSIZE);
        		strcat(buffer,"::");
        		strcat(buffer,varName);
        		fputs(buffer,fp);
        		fputs("\n",fp);

        		//Write the value of the variable
        		memset(buffer,'\0',BIGSIZE);
        		sprintf(buffer,"%d",*value);
        		fputs(buffer,fp);
        		fputs("\n",fp);

        		//Write Constant 1 (hard coded)
        		memset(buffer,'\0',BIGSIZE);
        		fputs("1\n",fp);
        	}
        	
		fputs("\n",fp);
		//if(entryOrExit == 1) {
		//	++callStackCounbter;
		//}
		fclose(fp);
	}
	std_unlock();
	
}

void do_nothing() {
	/**
	 * This function will do nothing
	 * if the statement in my implementation of assert
	 * evalautes to true
	 */
}

void write_assert_failure_trace() {
		int flag = 0;
		if(isCurrentOwnerOfLock()==FALSE) {
			std_lock();
			flag = 1;
		}  
		FILE *traceFp = fopen(fileName,"a");
		if(traceFp != NULL){
			fputs("\n======Bogus_Trace======\n\n",traceFp);
			fclose(traceFp);
		}

		if(flag == 1){
			//This means that this thread has acquired
			//lock
			std_unlock();
		}
}


void std_lock() {
	pthread_mutex_lock(&lock);
	pthread_t thrd = pthread_self();
	currentLockOwerId = getThreadId(&thrd);
}

void std_unlock() {
	currentLockOwerId  = -1;
	pthread_mutex_unlock(&lock);
}

bool isCurrentOwnerOfLock() {
	bool result = FALSE;
	pthread_mutex_t localLock;
	pthread_mutex_init(&localLock,NULL);
	pthread_mutex_lock(&localLock);
	pthread_t thrd = pthread_self();
	int tid = getThreadId(&thrd);
	if(tid == currentLockOwerId) {
		result = TRUE;
	}
	pthread_mutex_unlock(&localLock);
	pthread_mutex_destroy(&localLock);
	return result;
}

/**
 * Dumping the basic data types 
 * This function is also not thread safe.
 */
static void dump_basic_data_types(FILE *fp,void *data,char *varName,char *varType) {

	char buffer[SMALL];
#if DARIJIT
	printf("The parameter %s at beginning is %d of type %s:\n",varName,*data,varType);
#endif
	fputs(varName,fp);
	fputs("\n",fp);
	memset(buffer,'\0',SMALL);
	//sprintf(buffer,"%d",*data);


	printf("Variable type coming is %s\t for %s\n",varType,varName);

	if(strcmp(varType,"int") ==0 )
	{
		sprintf(buffer,"%d",*(int*)data);
	}
	else if(strcmp(varType,"float") ==0 )
	{
		sprintf(buffer,"%f",*(float*)data);
	}
	else if(strcmp(varType,"double") ==0 )
	{
		sprintf(buffer,"%f",*(double*)data);
	}
	else if(strcmp(varType,"char") ==0 )
	{
		//We have to take care of this section
	}
	else if(strcmp(varType,"short") ==0 )
	{
		sprintf(buffer,"%d",*(short*)data);

	}
	else if(strcmp(varType,"long") ==0 )
	{
		sprintf(buffer,"%ld",*(long*)data);
	}	
	
	fputs(buffer,fp);
	fputs("\n",fp);
	fputs("1\n",fp);
	memset(buffer,'\0',SMALL);

}

/**
 * This function hadnle the dumping case for the
 * Pointer data type.
 * This is not a thread safe implementation.
 * So call it only from thread-safe 
 * environment.
 *
 * Pointer elemenets should by type casted with 
 * pointer - pointer
 */

static void dump_pointer_data_types(FILE *fp,void *data,char *varName,char *varType) {

	char buffer[SMALL];
	
	fputs(varName,fp);
	fputs("\n",fp);
	memset(buffer,'\0',SMALL);
	//sprintf(buffer,"%d",*data);
	
	
	// We will double check that this 
	// function is handling the pointer type
	assert(strstr(varType,"*") != NULL);
	
	//Pointer data is same for 
	//all the data types
	sprintf(buffer,"%p",data);
	fputs(buffer,fp);
	fputs("\n",fp);
	fputs("1\n",fp);
	memset(buffer,'\0',SMALL);


	

	fputs(varName,fp);
	fputs("[..]",fp);
	fputs("\n",fp);
	memset(buffer,'\0',SMALL);
	
	if(strcmp(varType,"int*") ==0 ) {
		int **d = (int**)data;
        	sprintf(buffer,"[ %d ]",**d);
	} else if(strcmp(varType,"float*") ==0 ) {
		float **f = (float**)data;
        	sprintf(buffer,"[ %f ]",**f);
        } else if(strcmp(varType,"double*") ==0 ) {
        	double **d = (double**)data;
        	sprintf(buffer,"[ %f ]",**d);
        } else if(strcmp(varType,"short*") ==0 ) {
        	short **d = (short **)data;
        	sprintf(buffer,"[ %d ]",**d);
        }
        else if(strcmp(varType,"long*") ==0 ) {
        	long **d = (long **) data;
        	sprintf(buffer,"[ %ld ]",**d);
        }
	
       // else if(strcmp(varType,"char") ==0 ) {
       // 
       // 	//We have to take care of this section
       // }

	
	
	fputs(buffer,fp);
	fputs("\n",fp);
	fputs("1\n",fp);
	memset(buffer,'\0',SMALL);
}

static void dump_array_data_types(FILE *fp,void *data,char *varName,char *varType, size_t size) {

	char buffer[SMALL];
	
	fputs(varName,fp);
	fputs("\n",fp);
	memset(buffer,'\0',SMALL);
		
	
	
	// checking to make sure it's of type array
	assert(strstr(varType,"[]") != NULL);
	
	
	sprintf(buffer,"%p",data);
	fputs(buffer,fp);
	fputs("\n",fp);
	fputs("1\n",fp);
	memset(buffer,'\0',SMALL);

	fputs(varName,fp);
	fputs("[..]",fp);
	fputs("\n",fp);
	memset(buffer,'\0',SMALL);
	

	if(strcmp(varType,"int[]") ==0 ) {
		int (*d)[] = (int(*)[])data; 
		fputs("[",fp);
		size_t index = 0;
		for (; index <size; index++)
		{
		  if (index != size -1)
		  {
			  sprintf(buffer, " %d, ", (*d)[index]);
		  }
		  else
		  {
		  	 sprintf(buffer, " %d ", (*d)[index]);
		  }
		}
		fputs("]",fp);		
	}
       // else if(strcmp(varType,"float[]") ==0 ) {
       // 	float **f = (float**)data;
       // 	sprintf(buffer,"[ %f ]",**f);
       // }
       // else if(strcmp(varType,"double[]") ==0 ) {
       // 	double **d = (double**)data;
       // 	sprintf(buffer,"[ %f ]",**d);
       // }
       // else if(strcmp(varType,"short[]") ==0 ) {
       // 	short **d = (short **)data;
       // 	sprintf(buffer,"[ %d ]",**d);
       // }
       // else if(strcmp(varType,"long[]") ==0 ) {
       // 	long **d = (long **) data;
       // 	sprintf(buffer,"[ %ld ]",**d);
       // }
	

	
	
	fputs(buffer,fp);
	fputs("\n",fp);
	fputs("1\n",fp);
	memset(buffer,'\0',SMALL);
}
