#ifndef LISTH
#define LISTH
#include "lwp.h"

typedef struct list_struct{
	void (*addToQueue)(thread new, struct list_struct *this_list);
	void (*removeFromQueue)(thread victim, struct list_struct *this_list);
	thread head;
} list;

void addToQueue(thread new, list *this_list);
void removeFromQueue(thread victim, list *this_list);
#endif
