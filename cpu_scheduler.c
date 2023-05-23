#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cpu_scheduler.h"

// global constants
#define MAX_PROCESS 20
#define MAX_ARRIVAL_TIME 20
#define MAX_PRIORITY 4
#define DEFAULT_PRIORITY 0
#define MAX_CPU_BURST 20
#define DEFAULT_CPU_BURST 10
#define MAX_IO_BURST 5
#define DEFAULT_IO_BURST 2

#define MAX_TIME 100
// global clock variable
int CLK;


// FUNCTIONS //

Process** create_process(Config *cfg){
    /* 
    Creates a number of processes as specified and returns a job pool */
    
    int count = cfg->num_process;
    Process **new_pool = (Process**) malloc(sizeof(Process*)*count);
    
    // create processes and store them in new_pool
    for(int i=0; i<count; i++){
        new_pool[i] = _create_process(cfg);
    }
    
    return new_pool;
}

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
    
    ... priority: (Config.use_priority=true) 1 ~ MAX_PRIORITY (4)
                  (Config.use_priority=false) DEFAULT_PRIORITY (0: PRIORITY NOT USED)
    
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
    new_process->priority = cfg->use_priority ? rand()%MAX_PRIORITY + 1 : DEFAULT_PRIORITY;
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
    /*
    Create an empty Queue
    
    Parameters
    ----------
    int priority: priority of the queue (0, 1~4)
        if priority is 0, the queue does not use priority
    
    */
    Queue *new_queue = (Queue*)malloc(sizeof(Queue));
    new_queue->head = NULL;
    new_queue->tail = NULL;
    new_queue->priority = priority;
    new_queue->cnt = 0;

    return new_queue;
}


Table* create_table(Config *cfg){
    /*
    Create a Table which keeps track of all queues, running process, and current time

    *** prio_q (which is a queue of queues) will be used instead of ready_q, when priority is used ***

    Parameters
    ----------
    int priority: 0 (priority queue not created)
                  1~4 (`priority` number of priority queues created)
                  *** at this point, priority is not used ***
    */
    bool priority = cfg->use_priority;

    // create new table
    Table *new_table = (Table*)malloc(sizeof(Table));   
    new_table->new_pool = NULL;
    new_table->wait_q = create_queue(0);
    new_table->term_q = create_queue(0);
    new_table->running_p = NULL;
    new_table->clk = 0;


    // create ready_q or prio_q depending on `cfg.use_priority`
    if (priority == false){
        new_table->ready_q = create_queue(0);
        new_table->prio_q = NULL;
        return new_table;
    } // priority is not used (ready_q is used instead of prio_q)
    else{
        new_table->ready_q = NULL;
        new_table->prio_q = (Queue**)malloc(sizeof(Queue*)*MAX_PRIORITY);
        for(int i=0; i<MAX_PRIORITY; i++){
            new_table->prio_q[i] = create_queue(i+1);
        }
        return new_table;
    } // priority is used (prio_q is used instead of ready_q)
    
    return NULL;    // should not reach here
}

void arrived_to_ready(Table* tbl, int count){
    /*
    Check new_pool for processes that have arrived and enqueue them to ready_q

    Parameters
    ----------
    int count: maximum length of new_pool (i.e. cfg->num_process)
    */
    Process **new_pool = tbl->new_pool;
    Queue *ready_q = tbl->ready_q;

    for(int i=0; i<count; i++){
        if(new_pool[i]->arrival_time == tbl->clk){
            enqueue(ready_q, new_pool[i]);
            new_pool[i]->state = 1; // ready
        }
    }
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

Process* scheduler(Table* tbl, int algo){
    /*
    Reads the ready queue and assigns a process to CPU (i.e. state=2 (running))

    Queues* qs: pointer to the Queues struct
        ->ready_q: (Queue*) pointer to the ready queue
        ->wait_q: (Queue*) pointer to the waiting/blocking queue
        ->term_q:

    Returns
    -------
    Process *p: pointer to the process to be executed
        returns NULL if ready queue or priority queues are empty
    */

    // check if ready queue is empty
    if(tbl->ready_q->cnt == 0){
        return NULL;
    }

    
    switch(algo){
        case 0:
            return _FCFS(tbl->ready_q);
            break;
        default:
            printf("Error: Invalid algorithm number\n");
            exit(1);
    }


}

Process* _FCFS(Queue* q){
    /* 
    Returns the first (leftmost) process in the queue.
    */

    // check if queue is empty (unlike scheduler(), this checks for .head Node, not count)
    if(q->head == NULL){
        printf("Error: .head Node is NULL but count is not 0\n");
        return NULL;
    }

    Process *select_p = q->head->p;
    select_p->state = 2; // running
    q->head = q->head->right;
    q->head->left = NULL;
    q->cnt--;
    
    if(q->head == NULL){
        q->tail = NULL;
    } // queue is empty

    return select_p;
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
    printf("Arrival_time: %d\n\n", p->arrival_time);
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
    printf("NULL\n\n");
}





int main(){
    // set test init
    Config cfg = {
        .rand_pid = true,
        .rand_arrival = true,
        .use_priority = false,
        .rand_cpu_burst = true,
        .rand_io_burst = true,
        .num_process = 10
    };
    srand(99);
    

    // create an empty table. (empty new_pool, ready, wait, term queues are created. CLK <-- 0)
    Table *tbl = create_table(&cfg);
    
    // create processes, store them in new_pool (tbl->new_pool)
    tbl->new_pool = create_process(&cfg);

    // print process info
    /* for(int i=0; i<cfg.num_process; i++){
        print_process_info(new_pool[i]);
    } */

    // loop
    while(tbl->clk < MAX_TIME){
        // add processes that arrived to ready_queue
        arrived_to_ready(tbl, cfg.num_process);

        
        // check running_p (done, NULL)


        // run scheduler --> running_p is either assigned a new process or remains (depending on algo)
        // if running_p is NULL, increment idle time (if implement)
        // for now (FCFS), running_p shouldn't be NULL unless ready queue is empty


        //check if done (new_pool is empty AND term_q count == num_process)

        tbl->clk++;
    }

    // print ready queue
    printf("Ready Queue\n");
    print_queue(tbl->ready_q);

    return 0;
}

