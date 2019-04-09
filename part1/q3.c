#include <stdlib.h>
#include <stdio.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

uthread_mutex_t mx;
uthread_cond_t can_go;
int count = 0;

#define NUM_THREADS 3
uthread_t threads[NUM_THREADS];

void randomStall() {
  int i, r = random() >> 16;
  while (i++<r);
}

void waitForAllOtherThreads() {

  if(count == NUM_THREADS){
      uthread_cond_signal(can_go);
  }
  else{
    uthread_cond_wait(can_go);
  }

}

void* p(void* v) {
  randomStall();

  uthread_mutex_lock(mx);
  printf("a\n");
  count++;
  waitForAllOtherThreads();

  printf("b\n");
  if(count == NUM_THREADS){
      uthread_cond_signal(can_go);
  }
  uthread_mutex_unlock(mx);

  return NULL;

}

int main(int arg, char** arv) {
  uthread_init(4);
  mx = uthread_mutex_create();
  can_go = uthread_cond_create(mx);

  for (int i=0; i<NUM_THREADS; i++){

    threads[i] = uthread_create(p, NULL);
  //  count++;
  }

  for (int i=0; i<NUM_THREADS; i++){

    uthread_join (threads[i], NULL);
  }

  printf("------\n");
}
