#include "lwp.h"
#include "thread_queue.h"
#include <stddef.h>


// add thread to queue
void addToQueue(thread new_thread, list *this_list) {
    if (!new_thread || !this_list) { // check for null
		//printf("error: addToQueue - invalid input, new_thread or this_list is NULL\n");
        return;
    }

    if (!this_list->head) { // if list is empty, set new thread as head
        this_list->head = new_thread;
		//printf("addToQueue: adding thread %d as the head of the list\n", new_thread->tid);
    } else { // find end of list to add new thread
        thread tail = this_list->head;
        while (tail->sched_one) { // traverse
            tail = tail->sched_one;
        }
        tail->sched_one = new_thread; // append
		//printf("addToQueue: appending thread %d to the end of the list\n", new_thread->tid);
    }
    new_thread->sched_one = NULL; // end of list points to null
}

// remove a thread from queue
void removeFromQueue(thread victim_thread, list *this_list) {
    if (!victim_thread || !this_list || !this_list->head) { // check for valid input and non-empty list
		//printf("error: removeFromQueue - Invalid input or empty list\n");
        return;
    }

    thread prev_thread = NULL;
    thread curr_thread = this_list->head;
    while (curr_thread) {
        if (curr_thread == victim_thread) { // if found the victim
			//printf("removeFromQueue: found victim thread %d in the list\n", victim_thread->tid);
            if (prev_thread) {
                prev_thread->sched_one = curr_thread->sched_one; // bypass the victim
				//printf("removeFromQueue: femoving thread %d from middle or end of the list\n", victim_thread->tid);
            } else { // if the victim is the head
                this_list->head = curr_thread->sched_one; // new head is the next thread
				//printf("removeFromQueue: removing head thread %d from the list\n", victim_thread->tid);
            }
            break;
        }
        prev_thread = curr_thread;
        curr_thread = curr_thread->sched_one;
    }
}
