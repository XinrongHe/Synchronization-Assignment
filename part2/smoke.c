#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 10000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

int ms;              //number of matches available
int ps;              //number of paper available
int ts;              //number of tobacco available

int smoke_count  [5];  // # of times smoker with resource smoked

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);
  return agent;
}

struct Pool{
  struct Agent* agent;
  uthread_cond_t  tobaccoSmoker;
  uthread_cond_t  paperSmoker;
  uthread_cond_t  matchSmoker;
};

struct Pool* createPool(struct Agent* a){
  struct Pool* pool = malloc(sizeof(struct Pool));

  pool->agent = a;
  pool->tobaccoSmoker   = uthread_cond_create (a->mutex);
  pool->paperSmoker   = uthread_cond_create (a->mutex);
  pool->matchSmoker   = uthread_cond_create (a->mutex);
  return pool;
}

struct Smoker{
  int type;             // 4: the smoker who has tobacco; 2: the smoker who has paper; 1: the smoker who has match
  struct Pool* pool;
};

struct Smoker* createSmoker(struct Pool* c, int type){
  struct Smoker* smoker = malloc(sizeof(struct Smoker));
  smoker->pool = c;
  smoker->type = type;
  return smoker;
}

void* pool1(void* v){
  struct Pool* p = v;
  struct Agent* a = p->agent;
  uthread_mutex_lock(a->mutex);
  while(1){
  uthread_cond_wait(a->match);
      ms = 1;
      if(ps == 1){
        uthread_cond_signal(p->tobaccoSmoker);
      } else if (ts == 1){
        uthread_cond_signal(p->paperSmoker);
      }
    }
  //uthread_mutex_unlock(a->mutex);
}

void* pool2(void* v){
  struct Pool* p = v;
  struct Agent* a = p->agent;
  uthread_mutex_lock(a->mutex);
  while(1){
  uthread_cond_wait(a->paper);
  ps = 1;
  if(ms == 1){
    uthread_cond_signal(p->tobaccoSmoker);
  } else if (ts == 1){
    uthread_cond_signal(p->matchSmoker);
  }
}
  //uthread_mutex_unlock(a->mutex);
}

void* pool4(void* v){
  struct Pool* p = v;
  struct Agent* a = p->agent;
  uthread_mutex_lock(a->mutex);
  while(1){
  uthread_cond_wait(a->tobacco);
  ts = 1;
  if(ps == 1){
    uthread_cond_signal(p->matchSmoker);
  } else if (ms == 1){
    uthread_cond_signal(p->paperSmoker);
  }
}
  //uthread_mutex_unlock(a->mutex);
}


void* smoke(void* v){
  struct Smoker* s = v;
  struct Pool* p = s->pool;
  struct Agent* a = p->agent;
  uthread_mutex_lock(a->mutex);
  while(1){
    switch(s->type){
      case 4:
        while (!(ms == 1 && ps == 1)){
          uthread_cond_wait(p->tobaccoSmoker);
        }
        ms = 0;
        ps = 0;
        smoke_count[s->type]++;
        uthread_cond_signal(a->smoke);
        break;

      case 2:
        while(!(ms == 1 && ts == 1)){
          uthread_cond_wait(p->paperSmoker);
        }
        ms = 0;
        ts = 0;
        smoke_count[s->type]++;
        uthread_cond_signal(a->smoke);
        break;

      case 1:
        while(!(ps == 1 && ts == 1)){
          uthread_cond_wait(p->matchSmoker);
        }
        ps = 0;
        ts = 0;
        smoke_count[s->type]++;
        uthread_cond_signal(a->smoke);
        break;

      default:
        break;
    }
  }
  //uthread_mutex_unlock(a->mutex);
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
//int smoke_count  [5];  // # of times smoker with resource smoked

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  //struct Cigarette* ci = av;
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};

  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        uthread_cond_signal (a->tobacco);
      }

      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

int main (int argc, char** argv) {
  ms = 0;
  ps = 0;
  ts = 0;

  uthread_init (7);
  struct Agent*  a = createAgent();
  struct Pool* p = createPool(a);
  struct Smoker* s1 = createSmoker(p, 1);
  struct Smoker* s2 = createSmoker(p, 2);
  struct Smoker* s4 = createSmoker(p, 4);


  uthread_t sm = uthread_create(smoke, s1);
  uthread_t sp = uthread_create(smoke, s2);
  uthread_t st = uthread_create (smoke, s4);

  uthread_t gm = uthread_create(pool1, p);
  uthread_t gp = uthread_create(pool2, p);
  uthread_t gt = uthread_create(pool4, p);

  uthread_join (uthread_create (agent, a), 0);

  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
  printf ("Signal counts: %d matches, %d paper, %d tobacco\n",
          signal_count [MATCH], signal_count [PAPER], signal_count [TOBACCO]);
}
