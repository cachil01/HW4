#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>

#define GRAPH_COLUMNS 50
#define MAX_NUM_OF_STAR 20

// Support for thread safe timers
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
	struct timeval startTime;
	struct timeval endTime;
} Timer;

Timer timer[1000];

void startTime(int i);
void stopTime(int i);
void elapsedTime(int i);

void startTime(int i) {
	//printf("Start Timer...");
	pthread_mutex_lock(&timer_mutex);
	gettimeofday(&(timer[i].startTime), NULL);
	pthread_mutex_unlock(&timer_mutex);
}

void stopTime(int i) {
	//printf("Stop Timer...");
	pthread_mutex_lock(&timer_mutex);
	gettimeofday(&(timer[i].endTime), NULL);
	pthread_mutex_unlock(&timer_mutex);
}

void elapsedTime(int i) {
	float elapseTime = (float) ((timer[i].endTime.tv_sec
			- timer[i].startTime.tv_sec)
			+ (timer[i].endTime.tv_usec - timer[i].startTime.tv_usec) / 1.0e6);
	printf("Time: %4.2f Sec.\n", elapseTime);
}

/*http://en.wikipedia.org/wiki/Counting_sort*/
void CountingSort(int size, int setSize, int * theArray){
	int *counts, i;
	int	j, k = 0;
	counts = malloc((setSize+1)*sizeof(int));
	for (i = 0; i <= setSize; i++) counts[i]=0;
	for (i = 0; i < size; i++)counts[theArray[i]]++;
	for (i = 0; i <= setSize; i++)
		for (j = 0; j < counts[i]; j++){
			theArray[k]= i;
			k++;
			}
}
//DO NOT USE THIS. This is a reference implementaion.
// Use the Advice from your Professor!!!
void SKEWinitData(char * vector, int size, char c) {
	int i;
	printf("Generating ***Skewed*** Distributed Data:\n");
	srand (time(NULL));
	for (i = 0; i < size; i++){
		if (i < size/2) vector[i]=c;
		else vector[i] = rand()%26+'a';
	}
}