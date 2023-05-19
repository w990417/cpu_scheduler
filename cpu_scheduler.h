// defines variables and data structures that are used in cpu_scheduler.c

/* STRUCTS
Status: keeps track of processes and their states
Process: holds information about a process
Queue: priority queue. has 4 queues (one for each priority)
Node: linked list of processes of the same priority
Config: keeps track of current configuration
 */

#ifndef CPU_SCHEDULER_H
#define CPU_SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef MAX_PROCESS
#define MAX_PROCESS 5
#endif


// structs
typedef struct {
    // initial data
    int pid;             // 1001 ~ 9999
    int state;           // 0=new, 1=ready, 2=running, 3=waiting, 4=terminated
    int priority;        // 1~MAX_PRIORITY=4 where 1 is the highest priority and default is 3
    int cpu_burst_init;  // remaining cpu burst
    int io_burst_init;   // remaining io burst
    int arrival_time;    // 0 ~ MAX_ARRIVAL_TIME, default is global process_cnt
    
    // for evaluation()
    int ready_wait_time;
    int io_wait_time;
    int total_wait_time;
    int turnaround_time;
    int finish_time;
}Process;


typedef struct{
    /*
    Priority Queue: each entry is a queue of processes of the same priority (i.e. linked list)
     */
    // queue of processes of priority 1
    struct Node *p1;
    // queue of processes of priority 2
    struct Node *p2;
    // queue of processes of priority 3
    struct Node *p3;
    // queue of processes of priority 4
    struct Node *p4;

    // processes count
    int p_cnt;
    int p1_cnt;
    int p2_cnt;
    int p3_cnt;
    int p4_cnt;

}PQueue;

typedef struct{
    /* One dimensional queue (linked list) with no priority*/
    struct Node *head;
    int p_cnt;
}Queue;


typedef struct{
    /*
    Queue (linked list) of processes of the same priority

    Processes enter the queue from left to right (i.e. left node entered the queue earlier)
    */
    struct Process *p;
    struct Node *left;  // left node entered the queue earlier
    struct Node *right; // right node entered the queue later

}Node;


typedef struct{
    bool rand_pid;      // false: (default) increments from 1001
                        // true: random (1001 ~ 9999)
    bool rand_arrival;  // false: (default) increments from 0
                        // true: random (0 ~ MAX_ARRIVAL_TIME)
    bool rand_priority; // false: (default) priority is fixed to DEFAULT_PRIORITY for all processes
                        // true: priority is random (0 ~ MAX_PRIORITY)
    bool rand_cpu_burst;// false: (default) fixed to DEFAULT_CPU_BURST for all processes
                        // true: random (1 ~ MAX_CPU_BURST)
    bool rand_io_burst; // false: (default) fixed to DEFAULT_IO_BURST for all processes                   
}Config;

// function prototypes
Queue* create_queue(int priority);
// Process* create_process();
Process* _create_process(Config *cfg);
void print_process_info(Process *p);
void _enqueue();


#endif  // CPU_SCHEDULER_H

