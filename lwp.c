#include "lwp.h"
#include "rr.h"
#include "thread_queue.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define STACK_SIZE (1024 * 1024 * 8) // fefine stack size for each thread as 8 MB
#define MAX_LWPS 32 // max # of LWPs (threads)

static thread current = NULL; // current thread
static tid_t last_tid = 0; // id of last used thread
static thread *threads[MAX_LWPS] = {NULL};
static list wait_list = {addToQueue, removeFromQueue, NULL};
static list blocked_list = {addToQueue, removeFromQueue, NULL};

scheduler current_scheduler = &rr_scheduler;

// wraps user thread 
void lwp_wrap(lwpfun fun, void *arg) {
    int ret_val = fun(arg); // execute the user's function
    //printf("lwp_wrap: Function executed, returned %d\n", ret_val);
    lwp_exit(ret_val);
}

// creates a new lightweight process (lwp) to execute the given function with its arguments
tid_t lwp_create(lwpfun function, void *argument) {
    //printf("lwp_create: Creating new thread\n");
    thread new_thread = (thread)malloc(sizeof(context));
    if (!new_thread) {
        //printf("error: lwp_create - failed to allocate memory for new thread\n");
        return NO_THREAD;
    }
	
	unsigned long *base = (unsigned long *)mmap(
		NULL, 
		STACK_SIZE, 
		PROT_READ | PROT_WRITE, 
		MAP_PRIVATE | MAP_ANONYMOUS, 
		-1, 
		0
	);
    if (base == MAP_FAILED) {
        printf("error: lwp_create - memory mapping failed\n");
        free(new_thread);
        return NO_THREAD;
    }

    // Initialize stack and context
    unsigned long *top = base + STACK_SIZE / sizeof(unsigned long);
    top--;
    *top = (unsigned long)lwp_wrap; // entry point to lwp_wrap
    top--;

    last_tid++; 
    new_thread->tid = last_tid;
    new_thread->stack = base;
    new_thread->stacksize = STACK_SIZE;
    new_thread->state.rsp = (unsigned long)top; // stack ptr
    new_thread->state.rbp = (unsigned long)top; // base ptr
    new_thread->state.rdi = (unsigned long)function; // func arg
    new_thread->state.rsi = (unsigned long)argument; // func arg
    new_thread->state.fxsave = FPU_INIT; // floating pt
    new_thread->status = LWP_LIVE; // set thread status to live

    // admit the new thread to the scheduler
    current_scheduler->admit(new_thread);
    threads[new_thread->tid] = new_thread; // add it to threads

    //printf("lwp_create: thread %d created with stack at %p\n", new_thread->tid, (void *)base);
    return new_thread->tid; // returns lightweight thread id of the new thread
}

// starts the lwp system
void lwp_start(void) {
    //printf("lwp_start: Starting LWP system\n");
    thread main_thread = (thread)malloc(sizeof(context));
    if (!main_thread) {
        //printf("error: lwp_start - failed to allocate memory for main thread\n");
        return;
    }
    main_thread->tid = 0;
    main_thread->stack = NULL;
    main_thread->status = LWP_LIVE;
    current_scheduler->admit(main_thread); // admit main thread to scheduler
    current = main_thread;

    //printf("error: lwp_start - Main thread admitted and set as current\n");
    lwp_yield(); // yield to start the main thread or the next ready thread
}

// yields control to another lwp... depends on the scheduler
void lwp_yield(void) {
    thread prev = current;
    //printf("lwp_yield: current thread %d yielding\n", prev->tid);

    // if the previous thread terminated, remove it from the scheduler
    if (LWPTERMINATED(prev->status)) { 
        //printf("lwp_yield: thread %d has terminated, removing from scheduler\n", prev->tid);
        current_scheduler->remove(prev);
        thread curr_blocked = blocked_list.head;
        bool added_to_wait_list = true;

        // find a blocked thread to replace
        while (curr_blocked) {
            if (!curr_blocked->lib_one) {
                curr_blocked->lib_one = prev;
                blocked_list.removeFromQueue(curr_blocked, &blocked_list);
                current_scheduler->admit(curr_blocked);
                added_to_wait_list = false;
                //printf("lwp_yield: unblocked thread %d and admitted to scheduler\n", curr_blocked->tid);
                break;
            }
            curr_blocked = curr_blocked->sched_one;
        }
        if (added_to_wait_list) {
            wait_list.addToQueue(prev, &wait_list);
            //printf("lwp_yield: no unblocked threads available, added thread %d to wait list\n", prev->tid);
        }
    }
    current = current_scheduler->next();

    if (current == prev) {
        //printf("lwp_yield: switching context from thread %d to %d\n", prev->tid, current->tid);
        return;
    } //else {
        //printf("lwp_yield: no other threads to run, staying on thread %d\n", current->tid);
    //}
    swap_rfiles(&prev->state, &current->state);
}

void lwp_exit(int status) {
    //printf("lwp_exit: exiting thread %d with status %d\n", current->tid, status);
    current->status = MKTERMSTAT(LWP_TERM, status);
    if (current->tid == 0) {
        //printf("lwp_exit: exiting main thread, freeing resources\n");
        free(current);
        if (current_scheduler->shutdown) {
            current_scheduler->shutdown();
        }
        return;
    }
    lwp_yield();
}

tid_t lwp_wait(int *status) {
    //printf("lwp_wait: waiting for a thread to terminate\n");
    if (!wait_list.head && current_scheduler->qlen() == 0) {
        //printf("lwp_wait: no threads in wait list or scheduler queue\n");
        return NO_THREAD; 
    }
    if (!wait_list.head) {
        current_scheduler->remove(current);
        blocked_list.addToQueue(current, &blocked_list);
        lwp_yield(); // block the current thread and yield
    } else {
        current->lib_one = wait_list.head; // connect the current with the head of the wait list
        wait_list.removeFromQueue(wait_list.head, &wait_list); // remove the head from the wait list
    }

    thread terminated = (thread)current->lib_one; // retrieve terminated thread
    if (!terminated) {
        //printf("lwp_wait: no terminated thread found\n");
        return NO_THREAD;
    }

    if (status) {
        *status = terminated->status;
    }

    if (munmap(terminated->stack, terminated->stacksize) == -1) {
        perror("munmap error");
    }

    tid_t terminated_tid = terminated->tid;
    free(terminated);
    current->lib_one = NULL;
    
    //printf("lwp_wait: Thread %d terminated and cleaned up\n", terminated_tid);
    return terminated_tid;
}

tid_t lwp_gettid(void) {
    return current ? current->tid : 0; 
}

thread tid2thread(tid_t tid) {
    if (tid >= 0 && tid < MAX_LWPS) {
        return threads[tid];
    }
    return NULL;
}

void lwp_set_scheduler(scheduler s) {
    if (s) {
        current_scheduler = s;
        if (s->init) {
            s->init();
        }
    }
}

scheduler lwp_get_scheduler(void) {
    return current_scheduler;
}