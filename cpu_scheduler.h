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
#define MAX_PROCESS 20
#endif


// structs
typedef struct Process{
    // initial data
    int pid;             // 1001 ~ 9999
    int state;           // 0=new, 1=ready, 2=running, 3=waiting, 4=terminated
    int priority;        // 1~MAX_PRIORITY=4 where 1 is the highest priority and default is 3
    
    int cpu_burst_init;  // initial cpu_burst_time
    int io_burst_init;   // initial io_burst_time
    int cpu_burst_rem;  // remaining cpu burst
    int io_burst_rem;   // remaining io burst
    int arrival_time;    // 0 ~ MAX_ARRIVAL_TIME, default is global process_cnt
    
    // for evaluation()
    int ready_wait_time;
    int io_wait_time;
    int total_wait_time;
    int turnaround_time;
    int finish_time;
}Process;


typedef struct Queue{
    /* One dimensional queue (linked list) with no priority*/
    struct Node *head;
    struct Node *tail;
    int priority;       // 1~MAX_PRIORITY where 1 is the highest. If priority=0, then it is not used.
    int cnt;
}Queue;

typedef struct Table{
    /* Collection of Queue's */
    Process** new_pool;     // new
    struct Queue* ready_q;  // ready
    struct Queue* wait_q;   // waiting/blocked
    struct Queue* term_q;   // terminated
    Process* running_p;     // Process currently running
    int clk;                // current time
}Table;


typedef struct Node{
    /*
    Queue (linked list) of processes of the same priority

    Processes enter the queue from left to right (i.e. left node entered the queue earlier)
    */
    struct Process *p;
    struct Node *left;  // left node entered the queue earlier
    struct Node *right; // right node entered the queue later

}Node;


typedef struct Config{
    bool rand_pid;      // false: (default) increments from 1001
                        // true: random (1001 ~ 9999)
    bool rand_arrival;  // false: (default) increments from 0
                        // true: random (0 ~ MAX_ARRIVAL_TIME)
    bool use_priority; // false: (default) set to DEFAULT_PRIORITY for all processes (0: PRIORITY NOT USED)
                        // true: random (1 ~ MAX_PRIORITY)
    bool rand_cpu_burst;// false: (default) fixed to DEFAULT_CPU_BURST for all processes
                        // true: random (1 ~ MAX_CPU_BURST)
    bool rand_io_burst; // false: (default) fixed to DEFAULT_IO_BURST for all processes                   

    int num_process;    // number of processes to generate

    int algo;           // 0: FCFS, 1: SJF,
}Config;

// function prototypes
Process** create_process(Config *cfg);
Process* _create_process(Config *cfg);
Table* create_table(Config *cfg);
Queue* create_queue(int priority);

void arrived_to_ready(Table* tbl, int count);
void enqueue(Queue* q, Process* p);
void dequeue(Queue* q, Process* p);

void print_process_info(Process *p);
void print_queue(Queue *q);

void update_wait_time(Table* tbl);
void evaluate(Table* tbl);

int CPU(Table* tbl, int algo);
Process* _FCFS(Queue* q);
Process* _SJF(Queue* q);

#endif  // CPU_SCHEDULER_H

