#ifndef _QUEUE_H_
#define _QUEUE_H_

// system includes
#include <deque>
#include <queue>
#include <ctime>
#include <stdexcept>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

namespace re_gen {

/*
 * the re_gen::queue class is a generic queue that contains objects of type Queue_Of_T. The queue is designed to be shared
 * by a group of threads. the threads that share this queue will block when they try to pop a Queue_Of_T off the queue. once
 * something puts a new Queue_Of_T on the queue, threads are awakened via pthread_condition_siginal and pop returns with
 * the Queue_Of_T that was taken off the queue.
 *
 * @param Queue_Of_T - the type of object that is queued. it must have a default constructor.
 *
 * @note the queue destructor does not release threads that are waiting on the queue. you should send each thread a special
 * Queue_Of_T object to indicate to the thread that you want it to shut down; then wait for the thread to end before you
 * destruct the queue.
 *
 */
template <typename Queue_Of_T>
class Queue {
 public:
    Queue(void);
    ~Queue(void);
    void push(const Queue_Of_T &obj);
    Queue_Of_T pop(void);
    void wait_empty(void);

 private:
    pthread_mutex_t q_mutex;
    pthread_cond_t q_push_cond;
    pthread_cond_t q_pop_cond;
    std::queue<Queue_Of_T> q;
    // don't allow copy or assignement
    Queue(const Queue&);
    const Queue& operator=(const Queue&);
};


/* =================================================================
 * Nothing to see beyond this point :-)
 * =================================================================
 */
namespace re_queue_helpers {
  //exception safety helper - in that it unlocks as it goes out of scope
  class lock {
  public:
    lock(pthread_mutex_t &mutex): m(&mutex) { if (pthread_mutex_lock(m) != 0) throw std::runtime_error("can't lock mutex"); }
    ~lock(void)                             { pthread_mutex_unlock(m);                                                      }
  private:
    pthread_mutex_t *m;
  };
}


//####################################################################
template <typename Queue_Of_T>
Queue<Queue_Of_T>::Queue(void) {
    pthread_mutex_init(&q_mutex, 0);
    pthread_cond_init(&q_push_cond, 0);
    pthread_cond_init(&q_pop_cond, 0);
}
//####################################################################
template <typename Queue_Of_T>
Queue<Queue_Of_T>::~Queue(void) {
    pthread_mutex_destroy(&q_mutex);
    pthread_cond_destroy(&q_push_cond);
    pthread_cond_destroy(&q_pop_cond);
}
//####################################################################
template <typename Queue_Of_T>
void Queue<Queue_Of_T>::push(const Queue_Of_T &obj) {
    re_queue_helpers::lock l(q_mutex);
    q.push(obj);
    pthread_cond_signal(&q_push_cond);
}
//####################################################################
template <typename Queue_Of_T>
Queue_Of_T Queue<Queue_Of_T>::pop(void) {
  re_queue_helpers::lock l(q_mutex);
  //loop to catch spurious wake ups
  for (;;) { 
    if (!q.empty()) {
      Queue_Of_T obj = q.front();
      q.pop();
      if (q.empty())
	pthread_cond_signal(&q_pop_cond);
      return(obj);
    }
    //pthread_cond_wait is called, and returns, with mutex locked
    pthread_cond_wait(&q_push_cond, &q_mutex);
  }
}
//####################################################################
template <typename Queue_Of_T>
void Queue<Queue_Of_T>::wait_empty(void) {
  re_queue_helpers::lock l(q_mutex);
  //loop to catch spurious wake ups
  for (;;) { 
    if (q.empty())
      return;
    //pthread_cond_wait is called, and returns, with mutex locked
    pthread_cond_wait(&q_pop_cond, &q_mutex);
  }
}
}//re_gen
#endif //_QUEUE_H_
