#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

#define MAX_OCCUPANCY      3 // 3
#define NUM_ITERATIONS     100 // 100
#define NUM_PEOPLE         20
#define FAIR_WAITING_COUNT 4

void recordWaitingTime (int waitingTime);

/**
 * You might find these declarations useful.
 */
enum Endianness {LITTLE = 0, BIG = 1};
const static enum Endianness oppositeEnd [] = {BIG, LITTLE};

struct Well {
//  int enter;
  int num_of_drinkers;
  int type_of_drinker;
  uthread_mutex_t mutex;
  uthread_cond_t little_end;
  uthread_cond_t big_end;

};

struct Well* createWell() {

  struct Well* Well = malloc (sizeof (struct Well));
//    Well->enter;
  Well->num_of_drinkers = 0;
  Well->type_of_drinker= -1; // -1 means this person is neither BIG nor LITTLE
  Well->mutex = uthread_mutex_create();
  Well->little_end = uthread_cond_create(Well->mutex);
  Well->big_end = uthread_cond_create(Well->mutex);
  // TODO
  return Well;
}

struct Well* Well;
int littleWaitingCount = 0;
int bigWaitingCount = 0;

#define WAITING_HISTOGRAM_SIZE (NUM_ITERATIONS * NUM_PEOPLE) // 100 * 20 = 2000 for now
int             entryTicker;                                          // incremented with each entry
int             waitingHistogram         [WAITING_HISTOGRAM_SIZE];
int             waitingHistogramOverflow;
// waitingHistogrammutex is separated from the Well's uthread_mutex_t
uthread_mutex_t waitingHistogrammutex;
int             occupancyHistogram       [2] [MAX_OCCUPANCY + 1];

void enterWell (enum Endianness g) {

  uthread_mutex_lock(Well->mutex);

  // A person try to enter
  int startWaiting = entryTicker;
//    printf("%d\n", entryTicker);

  // While this person cannot enter.
  while(Well->type_of_drinker != -1 && (Well->type_of_drinker != g || Well->num_of_drinkers == MAX_OCCUPANCY)){
    if (g == LITTLE && littleWaitingCount<FAIR_WAITING_COUNT){
      littleWaitingCount++;
      uthread_cond_wait(Well->little_end);
        littleWaitingCount = 0;
    }
    else if (g == LITTLE && littleWaitingCount>=FAIR_WAITING_COUNT){
        while (Well->num_of_drinkers != 0) { // waiting for the well to be empty before letting the opposite endianese people to have a term
        uthread_cond_wait(Well->big_end);
        }
        Well->type_of_drinker == oppositeEnd[Well->type_of_drinker];
        littleWaitingCount = 0;
    }
    else if (g == BIG  && bigWaitingCount<FAIR_WAITING_COUNT){
      bigWaitingCount++;
      uthread_cond_wait(Well->big_end);
        bigWaitingCount = 0;
    }

    else if (g == BIG  && bigWaitingCount>=FAIR_WAITING_COUNT){

        while (Well->num_of_drinkers != 0) {// waiting for the well to be empty before letting the opposite endianese people to have a term
            uthread_cond_wait(Well->little_end);
        }
        Well->type_of_drinker == oppositeEnd[Well->type_of_drinker];
        bigWaitingCount = 0;
    }
  }

    int endTime = entryTicker;
    int waitingTime = endTime - startWaiting;

    recordWaitingTime(waitingTime);
 
    
    if(Well->type_of_drinker == -1){
        Well->type_of_drinker = g;
    }

//  Well->enter++;
  Well->num_of_drinkers ++;
  uthread_mutex_lock (waitingHistogrammutex);
//    printf("%d\n", entryTicker);
  entryTicker ++;
//    printf("%d\n", entryTicker);

  occupancyHistogram[Well->type_of_drinker][Well->num_of_drinkers] ++;
    
  uthread_mutex_unlock (waitingHistogrammutex);
   
  uthread_mutex_unlock(Well->mutex);
  // TODO
}


void leaveWell() {
//  printf("Line 97\n" );

  uthread_mutex_lock(Well->mutex);
  Well->num_of_drinkers --;

  if(Well->num_of_drinkers != 0) {
    if(Well->type_of_drinker == LITTLE)
      uthread_cond_signal(Well->little_end);
    if(Well->type_of_drinker == BIG)
      uthread_cond_signal(Well->big_end);
  }
  else {
      Well->type_of_drinker = -1;
      uthread_cond_broadcast(Well->little_end);
      uthread_cond_broadcast(Well->big_end);
  }

  uthread_mutex_unlock(Well->mutex);
}

void recordWaitingTime (int waitingTime) {

  uthread_mutex_lock (waitingHistogrammutex);
  if (waitingTime < WAITING_HISTOGRAM_SIZE)
    waitingHistogram [waitingTime] ++;

  else
    waitingHistogramOverflow ++;
  uthread_mutex_unlock (waitingHistogrammutex);
}


void * repeat(void * k) {

  int *n = (int*) k;
  // Decide the type of this person.
  enum Endianness e = *n;
  for(int i=0; i<NUM_ITERATIONS; i++) {
    // After enter well, yield N times
//    enterWell(e, &littleCount, &bigCount);
    enterWell(e);
    for(int j=0; j<NUM_PEOPLE; j++)
      uthread_yield();
    // After leave well, yield N times again.
    leaveWell();
    for(int i=0; i<NUM_PEOPLE; i++)
      uthread_yield();
  }
  return NULL;
}
//
// TODO
// You will probably need to create some additional produres etc.
//

int main (int argc, char** argv) {

  uthread_init (1);
  Well = createWell();
  uthread_t pt [NUM_PEOPLE];
  waitingHistogrammutex = uthread_mutex_create ();

  // TODO
  int people [NUM_PEOPLE];
  srand(time(0));   // we use srand here to make sure that every time we run this program we will have different endianese input, so the output might not be the same every time we run it.
  for(int i=0; i<NUM_PEOPLE; i++) {
    people[i] = (int) random() % 2;
    pt[i] = uthread_create(repeat, &people[i]);
  }
  for(int i=0; i<NUM_PEOPLE; i++)
    uthread_join(pt[i], NULL);

  printf("done\n");
  printf ("Times with 1 little endian %d\n", occupancyHistogram [LITTLE]   [1]);
  printf ("Times with 2 little endian %d\n", occupancyHistogram [LITTLE]   [2]);
  printf ("Times with 3 little endian %d\n", occupancyHistogram [LITTLE]   [3]);
  printf ("Times with 1 big endian    %d\n", occupancyHistogram [BIG] [1]);
  printf ("Times with 2 big endian    %d\n", occupancyHistogram [BIG] [2]);
  printf ("Times with 3 big endian    %d\n", occupancyHistogram [BIG] [3]);
  printf ("Waiting Histogram\n");
  for (int i=0; i<WAITING_HISTOGRAM_SIZE; i++)
    if (waitingHistogram [i])
      printf ("  Number of times people waited for %d %s to enter: %d\n", i, i==1?"person":"people", waitingHistogram [i]);
  if (waitingHistogramOverflow)
    printf ("  Number of times people waited more than %d entries: %d\n", WAITING_HISTOGRAM_SIZE, waitingHistogramOverflow);
}
