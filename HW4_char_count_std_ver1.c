/****************************************************************
 * HW4_char_count_std.c
 * Login to machine 103ws??.in.cs.ucy.ac.cy 
 * Compile using:
 * gcc -O3 -mavx2 -lpthread ./HW4_char_count_std.c -o HW4_char_count_std.out
 * Run as:
 *	./HW4_char_count_std.out 0 10 10000000000 0 c
 * Author: Petros Panayi, PhD, CS, UCY, Feb. 2019
 ****************************************************************/
 
#include <immintrin.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "support.h"

#define ARRAY_SIZE_MB 1024*1024		

typedef struct{
	char * start;
	int  size;
 	char  character;
	long  * sum;
	int id;
} INPUT;

typedef struct{
	char * start;
	int  size;
	int threads;
 	char  character;
	long  * sum;
	int id;
} INPUTI;


void initData(char * vector, int size) {
	int i;
	printf("Default: Generating Uniformly Distributed Data:\n");
	srand (time(NULL));
	for (i = 0; i < size; i++){
		vector[i] = rand()%26+'a';
		//vector[i] = 'a';
	}
}

void printArray(char * charArray, int size) {
	int i;
	for (i = 0; i < size; i++)
		printf("%c", charArray[i]);
	printf("\n");
}

int char_count_serial(char * vector, int size, char c){
	int i, sum =0;
	for (i=0; i<size; i++)
			if (vector[i] == c) sum++;

	//printf("sum is : %d\n", sum);
	return sum;
}

void * work(void *in){
	INPUT t = *((INPUT*) in);
	int i;
	int tid;
	
	tid = t.id;
	//printf("Thread %d starting...\n",tid);
	
	*(t.sum) =(long) char_count_serial(t.start , t.size, t.character); 
	
	//printf("Thread %d done with a result of %ld \n", tid, *(t.sum));
	pthread_exit((void*)in);


}

void * workerInterleaved(void * in){

	INPUTI t = *((INPUTI*)in);
	int i ;
	int tid;

	tid = t.id;
	//printf("Thread %d starting...\n",tid);
	
	int sum = 0;
	t.start += tid;
	for (i = 0; i < t.size*t.threads; i += t.threads){
		if (t.start[i] == t.character){
			 sum++;
		}
	}
	*(t.sum) = sum;
	//printf("Thread %d done with a result of %ld \n", tid, *(t.sum));
	
	pthread_exit((void*)in);
}


int char_count_AVX2(char * vector, int size, char c){
	int sum_vec = 0;													// here the accumulated amount is stored
	__m256i comp = _mm256_set1_epi8(c);						// store in a vector multiple times the key character
	__m256i AND = _mm256_set1_epi8(1);									// store in a vector the 0x01 multiple times 
	int i, j;																	// for using later with the AND operation
	j =0;																// initialize the offset counter to 0
	while(j<size/32){
		__m256i sum = _mm256_set1_epi8(0);								// here we initialize a vector that is used to 
		// temporarily store the sum of the appearances of
		// the key in the table
		for(i =0; i < 127 ; i++){										// the for loop goes for 127 times to avoid having overflow
			// in the vector 
			__m256i x = _mm256_loadu_si256((__m256i *)(vector+j*32));	// load into the vector 32 characters from the table
			__m256i y = _mm256_cmpeq_epi8(x, comp);						// compare the characters
			y = _mm256_and_si256(AND, y);  								// turn the -1 to 1 using AND logical operation
			sum = _mm256_add_epi8(y, sum);								// add the found caracters to the counter vector
			j++;														// increase j that is used as an offset for the vector pointer
			if(j >=size/32){
				break;
			}

		}
		__m128i low =  _mm256_extractf128_si256(sum, 0); 				
		__m128i high =  _mm256_extractf128_si256(sum, 1);				// we split the 256 bit vector to two 128 bit vectors
		__m256i low_ext = _mm256_cvtepi8_epi16(low);					// and then we sign extend them to integer 16 
		__m256i high_ext = _mm256_cvtepi8_epi16(high);					// so as to allow the use of hadd(horizontal add)
		// that is not supported for the integer 8 bit vector
		__m256i tmp = _mm256_add_epi16(low_ext, high_ext);				// we add the two new vectors 
		tmp = _mm256_hadd_epi16(tmp, tmp);								// here we accumulate the findings within a single vector
		tmp = _mm256_hadd_epi16(tmp, tmp);								// by using horizontal addition with itself
		tmp = _mm256_hadd_epi16(tmp, tmp);								// this results in the vector containing the accumulation
		sum_vec += ((short*)& tmp)[0]+((short*)&tmp)[8];				// split in two spots that we finally add to the variable that	
	}																	// holds our result
	return sum_vec;
}

void * workAVX2(void *in){
	INPUT t = *((INPUT*) in);
	int i;
	int tid;
	
	tid = t.id;
	//printf("Thread %d starting...\n",tid);
	
	*(t.sum) = char_count_AVX2(t.start , t.size, t.character); 
	
//	printf("Thread %d done with a result of %ld \n", tid, *(t.sum));
	pthread_exit((void*)in);


}


/*
int char_count_AVX23(char * vector, int size, char c){


	int sum=0;
	int j,i;
	__m256i vmask;
	__m256i a[sizeof(__m256i)];
	__m256i vkey;
	__m256i total;
	__m256i temp;
	int t;
	int sv;
	__m256i sv2;


	size = (size / (sizeof(__m256i))) - sizeof(__m256i);
		total = total ^ total; 
	for(j=0;j<sizeof(__m256i);j++){
		vmask = _mm256_cmpeq_epi8(a[j],vkey);
		total = _mm256_sub_epi8(total,vmask);
	}
	for(i=sizeof(__m256i);i<size;i=i+128){
		for(j=i;j<i+128-sizeof(__m256i);j++){
			vmask = _mm256_cmpeq_epi8(a[j],vkey);
			total = _mm256_sub_epi8(total,vmask);
		}
		temp=total;
		total = total ^ total;
		for(j=0;j<sizeof(__m256i);j++){
			vmask = _mm256_cmpeq_epi8(a[i+128-sizeof(__m256i)+j],vkey);
			total = _mm256_sub_epi8(total,vmask);
			t = _mm256_extract_epi8(sv2,0);
			sv = _mm_bsrli_si256(sv,1);
			sum+=t;
		}
	}

	return sum;

}

*/


int char_count_pthreads_consecutive(char * vector, int size, char c, int numThreads){
	int sum =0;

	INPUT * data= (INPUT*)malloc(sizeof(INPUT)*numThreads);
	int jump = size/numThreads;
	int i ;
	for (i=0; i< numThreads; i++){
		data[i].start = vector+i*jump; 
		//data[i]->size = (int*)malloc(sizeof(int));
		data[i].size = jump;
		//data[i]->character = (char*)malloc(sizeof(char));
		data[i].character = c;
		long * tp = (long*)malloc(sizeof(long));
		data[i].sum = tp;
		*(data[i].sum) = 0;
		data[i].id = i;

	}	


	pthread_t thread[numThreads];
	pthread_attr_t attr;
	int rc;
	int t;
	void *status;


	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for(t=0; t<numThreads; t++) {
		//printf("Main: creating thread %ld\n", t);
		rc = pthread_create(&thread[t], &attr, work, (void *)&data[t]);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	for(t=0; t<numThreads; t++) {
		rc = pthread_join(thread[t], &status);
		if (rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
		//printf("Main: completed join with thread %ld having a status of %ld\n",t,(long)status);
	}
	pthread_attr_destroy(&attr);
	//printf("Main: program completed. Exiting.\n");
	//pthread_exit(NULL);



	for( i =0 ; i<numThreads; i++){
		sum += *(data[i].sum);
	}
	//printf("THIS IS THE FINAL SUM RETURNED %d\n", sum);
	return sum;
}

int char_count_pthreads_interleaved(char * vector, int size, char c, int numThreads){
	int sum =0;

	INPUTI * data= (INPUTI*)malloc(sizeof(INPUTI)*numThreads);
	int chunk = size/numThreads;
	int i ;
	for (i=0; i< numThreads; i++){
		data[i].start = vector;
		//data[i].start += i; 
		//data[i]->size = (int*)malloc(sizeof(int));
		data[i].size = chunk;
		//data[i]->character = (char*)malloc(sizeof(char));
		data[i].character = c;
		long * tp = (long*)malloc(sizeof(long));
		data[i].sum = tp;
		*(data[i].sum) = 0;
		data[i].id = i;
		data[i].threads = numThreads;

	}	


	pthread_t thread[numThreads];
	pthread_attr_t attr;
	int rc;
	int t;
	void *status;


	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for(t=0; t<numThreads; t++) {
		//printf("Main: creating thread %ld\n", t);
		rc = pthread_create(&thread[t], &attr, workerInterleaved, (void *)&data[t]);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	for(t=0; t<numThreads; t++) {
		rc = pthread_join(thread[t], &status);
		if (rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
		//printf("Main: completed join with thread %ld having a status of %ld\n",t,(long)status);
	}
	pthread_attr_destroy(&attr);
	//printf("Main: program completed. Exiting.\n");
	//pthread_exit(NULL);



	for( i =0 ; i<numThreads; i++){
		sum += *(data[i].sum);
	}
	//printf("THIS IS THE FINAL SUM RETURNED %d\n", sum);
	return sum;
}

int char_count_pthreads_consecutive_AVX2(char * vector, int size, char c, int numThreads){
	int sum =0;
	INPUT * data= (INPUT*)malloc(sizeof(INPUT)*numThreads);
	int jump = size/numThreads;
	int i ;
	for (i=0; i< numThreads; i++){
		data[i].start = vector+i*jump; 
		//data[i]->size = (int*)malloc(sizeof(int));
		data[i].size = jump;
		//data[i]->character = (char*)malloc(sizeof(char));
		data[i].character = c;
		long * tp = (long*)malloc(sizeof(long));
		data[i].sum = tp;
		*(data[i].sum) = 0;
		data[i].id = i;

	}	


	pthread_t thread[numThreads];
	pthread_attr_t attr;
	int rc;
	int t;
	void *status;


	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for(t=0; t<numThreads; t++) {
		//printf("Main: creating thread %ld\n", t);
		rc = pthread_create(&thread[t], &attr, workAVX2, (void *)&data[t]);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	for(t=0; t<numThreads; t++) {
		rc = pthread_join(thread[t], &status);
		if (rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
		//printf("Main: completed join with thread %ld having a status of %ld\n",t,(long)status);
	}
	pthread_attr_destroy(&attr);
	//printf("Main: program completed. Exiting.\n");
	//pthread_exit(NULL);



	for( i =0 ; i<numThreads; i++){
		sum += *(data[i].sum);
	}
	//printf("THIS IS THE FINAL SUM RETURNED %d\n", sum);

	return sum;
}

int main(int argc, char **argv) {
	unsigned long long int arraySize, sum, method = 0,generatorMethod;
	char count_char;
	int numThreads=0;
	if (argc == 6) {
		method = atoi(argv[1]);
		numThreads = atoi(argv[2]);
		arraySize = atoi(argv[3])*ARRAY_SIZE_MB;
		generatorMethod = atoi(argv[4]);
		count_char = argv[5][0];
	} else {
		printf("Usage: ./a.out Method NumberOfThreads NumberOfCharsInMB charGeneratorMethod countChar\n");
		printf("Methods: 0=Serial, 1=AVX2, 2=pThreads_consecutive,3=pThreads_interleaved, 4=pThreads_consecutive_AVX2, other=Data init ONLY.\n");
		printf("charGeneratorMethod: 0=Uniform Random, 1=Skewed Ranndom\n");
				

		return 0;
	}
	 __attribute__ ((aligned (256))) char * charArray = (char *) malloc(sizeof(char) * arraySize);
	if (generatorMethod==1) SKEWinitData(charArray, arraySize, count_char);
	else initData(charArray, arraySize);
	//printArray(charArray, arraySize);
	if (method == 0){
		startTime(999);
		sum = char_count_serial(charArray, arraySize,count_char);
		stopTime(999);elapsedTime(999);
		printf("Serial Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
		// fprintf(stderr, is needed since time command prints on stderr
		// Collect the data by using ./runAll.sh 2> output.txt
		fprintf(stderr,"Serial Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
	}else if (method == 1){
		startTime(999);
		sum = char_count_AVX2(charArray, arraySize,count_char);
		stopTime(999);elapsedTime(999); 
		printf("AVX2 Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
		fprintf(stderr,"AVX2 Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
	}else if (method == 2){
		startTime(999);
		sum = char_count_pthreads_consecutive(charArray, arraySize,count_char, numThreads);
		stopTime(999);elapsedTime(999);
		printf("pThread Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
		//fprintf(stderr,"pThread Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);

		printf("-------------------------------\n------------------------\n");
		sum = char_count_serial(charArray, arraySize, count_char);
		printf("Serial Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
		


	}else if (method == 3){
		startTime(999);
		sum = char_count_pthreads_interleaved (charArray, arraySize,count_char, numThreads);
		stopTime(999);elapsedTime(999);
		printf("pThread Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
		//fprintf(stderr,"pThread Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);


		printf("-------------------------------\n------------------------\n");
		sum = char_count_serial(charArray, arraySize, count_char);
		printf("Serial Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
		


	}else if (method == 4){

		startTime(999);
		sum = char_count_serial(charArray, arraySize,count_char);
		stopTime(999);elapsedTime(999);
		printf("Serial Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);

		startTime(999);
		sum = char_count_AVX2(charArray, arraySize,count_char);
		stopTime(999);elapsedTime(999); 
		printf("AVX2 Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);

		startTime(999);
		sum = char_count_pthreads_consecutive(charArray, arraySize,count_char, numThreads);
		stopTime(999);elapsedTime(999);
		printf("pThread Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);

		startTime(999);
		sum = char_count_pthreads_interleaved (charArray, arraySize,count_char, numThreads);
		stopTime(999);elapsedTime(999);
		printf("pThread Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);

		startTime(999);
		sum = char_count_pthreads_consecutive_AVX2(charArray, arraySize,count_char, numThreads);
		stopTime(999);elapsedTime(999);
		printf("pThread_AVX2 Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
		//fprintf(stderr,"pThread_AVX2 Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);

		/*
		   printf("-------------------------------\n------------------------\n");
		   sum = char_count_AVX2(charArray, arraySize, count_char);
		   printf("SIMD Method[%d]:Found %d '%c'.\n",arraySize,sum,count_char);
		   */
	
	}
  return 0;
}
