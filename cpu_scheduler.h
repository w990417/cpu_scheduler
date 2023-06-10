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
    int priority;        // 1~MAX_PRIORITY=4 (0: PRIORITY NOT USED)
    
    int cpu_burst_init;  // initial cpu_burst_time
    int cpu_burst_rem;   // remaining cpu burst
    int arrival_time;    // 0 ~ MAX_ARRIVAL_TIME, default is global process_cnt
    int io_burst_start;  // # of cpu bursts after which io must be performed (1 ~ cpu_burst_init -1)
    int io_burst_rem;    // remaining io burst. Up to 1/2 of cpu burst time (1 ~ cpu_burst_init/2)

    // for evaluation()
    int ready_wait_time;
    int io_wait_time;
    int turnaround_time;
    int finish_time;
}Process;


typedef struct Queue{
    /* One dimensional queue (linked list) with no priority*/
    struct Node *head;
    struct Node *tail;
    int cnt;
}Queue;

typedef struct Table{
    /* Status Table */
    Process** new_pool;     // new
    struct Queue* ready_q;  // ready
    struct Queue* wait_q;   // waiting/blocked
    struct Queue* term_q;   // terminated
    Process* running_p;     // Process currently running
    Process* io_p;          // Process currently performing io
    int clk;                // current time
    int quantum;            // time quantum for RR
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
    bool use_priority;  // false: (default) set to DEFAULT_PRIORITY for all processes (0: PRIORITY NOT USED)
                        // true: random (1 ~ MAX_PRIORITY)                  
    int num_process;    // number of processes to generate

    int algo;           // 0: FCFS, 1: SJF, 2: SJF w/ preemption, 3: PRIO w/o preemption, 4: PRIO w/ preemption, 5: RR

    int quantum;        // quantum for RR
}Config;

// function prototypes
Process** create_process(Config *cfg);
Process* _create_process(Config *cfg);
Table* create_table(Config *cfg);
Queue* create_queue();

void arrived_to_ready(Table* tbl, int count);
void wait_to_ready(Table* tbl, int algo);
void enqueue(Queue* q, Process* p, int at_head);
void dequeue(Queue* q, Process* p);
void update_wait_time(Table* tbl);

int CPU(Table* tbl, int algo, int _quantum);
int io_service(Table* tbl, int algo);
Process* _SJF(Queue* q, int preemptive);
Process* _PRIO(Queue* q, Process* running_p, int preemptive);

void print_process_info(Process *p);
void print_queue(Queue *q);
void evaluate(Table* tbl, int algo, int gannt_size);
void print_gannt_chart(int gannt_size);

void display_config(Config* cfg);
void edit_config(Config* cfg);


#endif  // CPU_SCHEDULER_H

