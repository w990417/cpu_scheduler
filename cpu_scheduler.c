#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cpu_scheduler.h"

// global constants
#define MAX_PROCESS 5
#define MAX_ARRIVAL_TIME 20
#define MAX_PRIORITY 4
#define DEFAULT_PRIORITY 0
#define MAX_CPU_BURST 20
#define DEFAULT_CPU_BURST 10
#define MAX_IO_BURST 5
#define DEFAULT_IO_BURST 2

// global clock variable
int CLK;


// FUNCTIONS //

Process* _create_process(Config *cfg){
    /* 
    Create a process (struct Process)
    
    Parameters
    ----------
    Config *cfg: pointer to Config struct which determines whether to randomise process attributes or not

    Returns
    -------
    Process *new_process: pointer to new process with attributes...
    
    ... pid: (Config.rand_pid=true) 1001 ~ 9999
             (Config.rand_pid=false) 1 ~ 99 (NOT IMPLEMENTED YET)
    
    ... arrival_time: (Config.rand_arrival=true) 0 ~ MAX_ARRIVAL_TIME (20)
                      (Config.rand_arrival=false) CLK (current time)
    
    ... priority: (Config.rand_priority=true) 1 ~ MAX_PRIORITY (4)
                  (Config.rand_priority=false) DEFAULT_PRIORITY (0: PRIORITY NOT USED)
    
    ... cpu_burst_init: (Config.rand_cpu_burst=true) 1 ~ MAX_CPU_BURST (20)
                        (Config.rand_cpu_burst=false) DEFAULT_CPU_BURST (10)

    ... io_burst_init: (Config.rand_io_burst=true) 1 ~ MAX_IO_BURST (5)
                       (Config.rand_io_burst=false) DEFAULT_IO_BURST (2)

    ... state: 0=new {1=ready, 2=running, 3=waiting, 4=terminated}
    
    ... time related attributes are initialised to -1
     */
    
    Process *new_process = (Process*) malloc(sizeof(Process));
    

    new_process->pid = rand()%8999 + 1001;   // rand_pid=false is currently not implemented
    new_process->arrival_time = cfg->rand_arrival ? rand()%MAX_ARRIVAL_TIME + 1 : CLK;
    new_process->priority = cfg->rand_priority ? rand()%MAX_PRIORITY + 1 : DEFAULT_PRIORITY;
    new_process->cpu_burst_init = cfg->rand_cpu_burst ? rand()%MAX_CPU_BURST + 1 : DEFAULT_CPU_BURST;
    new_process->io_burst_init = cfg->rand_io_burst ? rand()%MAX_IO_BURST + 1 : DEFAULT_IO_BURST;
    new_process->state = 0; // new

    // time related attributes are initialised to -1
    new_process->ready_wait_time = -1;
    new_process->io_wait_time = -1;
    new_process->total_wait_time = -1;
    new_process->turnaround_time = -1;
    new_process->finish_time = -1;

    return new_process; // return pointer to new process
}

Queue* create_queue(int priority){
    /*Create a Queue*/
    Queue *new_queue = (Queue*)malloc(sizeof(Queue));
    new_queue->head = NULL;
    new_queue->tail = NULL;
    new_queue->priority = priority;
    new_queue->cnt = 0;

    return new_queue;
}


void enqueue(Queue *q, Process *p){
    /*
    Create/allocate a new Node and assign a process to it
    */
    
    // assert q->priority == p->priority
    if(q->priority != p->priority){
        printf("Error: Process priority does not match Queue's Priority\n");
        exit(1);
    }

    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->p = p;

    if(q->head == NULL){
        q->head = new_node;
        q->tail = new_node;
        new_node->left = NULL;
        new_node->right = NULL;
    } // queue is empty
    else{
        q->tail->right = new_node;
        new_node->left = q->tail;
        new_node->right = NULL;
        q->tail = new_node;
    } // queue is not empty
    q->cnt++;

}



void print_process_info(Process* p){
    // prints process attributes (except time related attributes for evaluation)
    printf("[%d]\n==========\n", p->pid);
    switch (p->state) {
        case 0:
            printf("State: new\n");
            break;
        case 1:
            printf("State: ready\n");
            break;
        case 2:
            printf("State: running\n");
            break;
        case 3:
            printf("State: waiting\n");
            break;
        case 4:
            printf("State: terminated\n");
            break;
        default:
            printf("State: unknown\n");
            break;
    }
    printf("Priority: %d\n", p->priority);
    printf("CPU Burst Time: %d\n", p->cpu_burst_init);
    printf("I/O Burst_Time: %d\n", p->io_burst_init);
    printf("Arrival_time: %d\n", p->arrival_time);
}


void print_queue(Queue *q){

    // check if empty
    if(q->head == NULL){
        printf("Queue is empty\n");
        return;
    }

    // print queue info
    printf("Queue Priority: %d\n", q->priority);
    printf("Processes in queue: %d\n", q->cnt);
    
    // print queue
    Node *curr = q->head;
    while (curr != NULL) {
        printf("[%d] -->", curr->p->pid);
        curr = curr->right;
    }
    printf("NULL\n");
}


int main(){
    
    
    // set test cfg
    Config cfg = {
        .rand_pid = true,
        .rand_arrival = true,
        .rand_priority = false,
        .rand_cpu_burst = true,
        .rand_io_burst = true
    };
    CLK = 0;
    srand(99);
    
    // job pool is a simple array of pointers to processes for now
    Process* job_pool[MAX_PROCESS];
    int p_cnt = 0; // number of processes in job pool

    // create processess
    for(int i=0; i<MAX_PROCESS; i++){
        job_pool[i] = _create_process(&cfg);
        print_process_info(job_pool[i]);
        p_cnt++;
    }

    // queue test
    Queue* test_queue = create_queue(0);
    for(int i=0; i<MAX_PROCESS; i++){
        enqueue(test_queue, job_pool[i]);
    }

    // print queue
    print_queue(test_queue);



    return p_cnt;
}

