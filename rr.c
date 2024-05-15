#include "lwp.h"
#include "rr.h"
#include <stddef.h>

static thread head = NULL; // head = null
static thread tail = NULL; // tail = null
static int queue_length = 0; // queue_length = num of threads

// initialize rr
void rr_init(void) {
    head = NULL;
    tail = NULL;
    queue_length = 0;
    //printf("rr_init: round-robin scheduler initialized\n");
}

// admit a new thread to the scheduler
void rr_admit(thread new_thread) {
    if (!new_thread) {
        //printf("error: rr_admit - Null thread cannot be admitted\n");
        return;
    }
    if (!head) {
        head = tail = new_thread;
        new_thread->sched_one = NULL;
        //printf("rr_admit: first thread %d admitted to scheduler\n", new_thread->tid);
    } else {
        tail->sched_one = new_thread;
        tail = new_thread;
        tail->sched_one = NULL;
        //printf("rr_admit: thread %d admitted to scheduler\n", new_thread->tid);
    }
    queue_length++; 
    //printf("rr_admit: queue length now %d\n", queue_length);
}

// remove a thread from the scheduler
void rr_remove(thread victim) {
    if (!victim || !head) {
        //printf("rr_remove: invalid removal attempt, victim or head is NULL\n");
        return; 
    }
    thread prev_thread = NULL;
    thread curr_thread = head;
    
    // Find and remove the victim thread
    while (curr_thread) {
        if (curr_thread == victim) {
            //printf("rr_remove: found victim thread %d for removal\n", victim->tid);
            if (prev_thread) {
                prev_thread->sched_one = curr_thread->sched_one;
                if (tail == curr_thread) {
                    tail = prev_thread;
                }
            } else {
                head = curr_thread->sched_one;
                if (!head) {
                    tail = NULL;
                }
            }
            queue_length--; 
            //printf("rr_remove: thread %d removed, queue length now %d\n", victim->tid, queue_length);
            break;
        }
        prev_thread = curr_thread;
        curr_thread = curr_thread->sched_one;
    }
}

// get the next thread to run from the queue
thread rr_next(void) {
    if (!head) {
        // printf("rr_next: No threads in scheduler\n");
        return NULL;
    }

    thread next_thread = head; 
    if (head == tail) {  // if there is only one thread in the queue
        // printf("rr_next: only one thread in scheduler, no rotation needed\n");
        return next_thread; // return the only thread, without rotation
    }

    // rotate the queue
    head = head->sched_one; // move head to the next thread
    tail->sched_one = next_thread; // place curr at the end of the queue
    tail = next_thread; // update tail
    tail->sched_one = NULL; // tail should be null now
    
    //printf("rr_next: rotated to thread %d\n", next_thread->tid);
    return next_thread;
}

// shutdown the scheduler
void rr_shutdown(void) {
    head = NULL;
    tail = NULL;
    queue_length = 0;
    //printf("rr_shutdown: Round-robin scheduler shutdown, cleared all entries\n");
}

int rr_qlen(void) {
    return queue_length;
}

struct scheduler rr_scheduler = {
    .init = rr_init,
    .shutdown = rr_shutdown,
    .admit = rr_admit,
    .remove = rr_remove,
    .next = rr_next,
    .qlen = rr_qlen
};
