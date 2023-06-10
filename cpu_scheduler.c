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
#define DEFAULT_IO_BURST 2
#define DEFAULT_IO_START 1

#define MAX_TIME 500
// global clock variable

int gannt[MAX_TIME] = {0};

// FUNCTIONS //

Process** create_process(Config *cfg){
    /* 
    Creates a number of processes as specified and returns a job pool */
    
    int count = cfg->num_process;
    int pid_list[MAX_PROCESS] = {0};
    Process **new_pool = (Process**) malloc(sizeof(Process*)*count);
    Process *new_process;

    // create processes and store them in new_pool
    for(int i=0; i<count; i++){
        new_process = _create_process(cfg);
        // check if pid already exists
        for(int j=0; j<i; j++){
            if(new_process->pid == pid_list[j]){
                new_process->pid = rand()%8999 + 1001;
                j = -1;
            }
        }

        new_pool[i] = new_process;
        pid_list[i] = new_process->pid;
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
    
    ... pid: 1001 ~ 9999
    
    ... arrival_time: 0 ~ MAX_ARRIVAL_TIME (20)

    ... priority: (Config.use_priority=true) 1 ~ MAX_PRIORITY (4)
                  (Config.use_priority=false) DEFAULT_PRIORITY (0: PRIORITY NOT USED)
    
    ... cpu_burst_init: 1 ~ MAX_CPU_BURST (20)

    ... io_burst_start: number of CPU burst cycles before I/O must be processed. Decrements by 1 every CPU burst.
                        I/O must be processed when io_burst_start == 0
                        if CPU burst is 1, no io burst

    ... io_burst_rem: (1 ~ cpu_burst_init/2) number of I/O burst cycles remaining (0 if cpu burst is 1)

    ... state: 0=new, 1=ready, 2=running, 3=waiting, 4=terminated
    
    ... time related attributes are initialised to 0
     */
    
    Process *new_process = (Process*) malloc(sizeof(Process));
    

    new_process->pid = rand()%8999 + 1001; // 1001 ~ 9999
    new_process->arrival_time = rand()%MAX_ARRIVAL_TIME + 1;
    new_process->priority = cfg->use_priority ? rand()%MAX_PRIORITY + 1 : DEFAULT_PRIORITY;
    new_process->cpu_burst_init = rand()%MAX_CPU_BURST;
    new_process->cpu_burst_rem = new_process->cpu_burst_init;
    if(new_process->cpu_burst_init == 1){
        new_process->io_burst_start = -1;
        new_process->io_burst_rem = -1;
    }
    else{
        new_process->io_burst_start = rand()%(new_process->cpu_burst_init-1) + 1;
        new_process->io_burst_rem = rand()%(new_process->cpu_burst_init/2) + 1;
    }
    new_process->state = 0; // new

    // time related attributes are initialised to -1
    new_process->ready_wait_time = 0;
    new_process->io_wait_time = 0;
    new_process->turnaround_time = 0;
    new_process->finish_time = 0;

    return new_process; // return pointer to new process
}

Queue* create_queue(){
    /*
    Create an empty Queue 
    */
    Queue *new_queue = (Queue*)malloc(sizeof(Queue));
    new_queue->head = NULL;
    new_queue->tail = NULL;
    new_queue->cnt = 0;

    return new_queue;
}


Table* create_table(Config *cfg){
    /*
    Create a Table which keeps track of all queues, running process, and current time

    Attributes
    ----------
    Process** new_pool: array of pointers to processes (assigned by create_process() in main())

    Queue* ready_q: queue of processes that are ready to be executed

    Queue* wait_q: queue of processes that are waiting for I/O to be completed

    Queue* term_q: queue of processes that have terminated

    Process* running_p: pointer to process that is currently running

    Process* io_p: pointer to process that is currently performing I/O (handled by CPU())

    int clk: current time

    int quantum: time quantum for Round Robin
    */

    Table *new_table = (Table*)malloc(sizeof(Table));   
    new_table->new_pool = NULL; // create_process() will create/allocate new_pool
    new_table->ready_q = create_queue();
    new_table->wait_q = create_queue();
    new_table->term_q = create_queue();
    new_table->running_p = NULL;
    new_table->io_p = NULL;
    new_table->clk = 0;
    new_table->quantum = cfg->quantum;

    return new_table;
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
            // log message
            printf("<@%d> ARRIVE: [%d] arrived to ready queue\n", tbl->clk, new_pool[i]->pid);
            enqueue(ready_q, new_pool[i], 1);
            new_pool[i]->state = 1; // ready
        }
    }
}


void wait_to_ready(Table* tbl, int algo){
    /* if I/O process is done, move to ready queue (or assign to CPU if non-preemptive) */
    
    if(tbl->io_p == NULL){
        return;
    }
    if(tbl->io_p->io_burst_rem == 0){
        if(algo == 2 || algo==4 || algo==5){    // if preemptive, move io_p to ready queue
            printf("<@%d> I/O COMPLETE: [%d]\n", tbl->clk-1, tbl->io_p->pid);
            printf("<@%d> READY: [%d] to ready queue\n", tbl->clk, tbl->io_p->pid);
            tbl->io_p->state = 1;   // ready
            enqueue(tbl->ready_q, tbl->io_p, 0);
            tbl->io_p = NULL;
        }
        else{   // if non-preemptive, insert at the head of ready queue
            printf("<@%d> I/O Complete: [%d]\n", tbl->clk-1, tbl->io_p->pid);
            printf("<@%d> READY: [%d] to CPU from I/O\n", tbl->clk, tbl->io_p->pid);
            tbl->io_p->state = 1;   // ready
            enqueue(tbl->ready_q, tbl->io_p, 1);
            tbl->io_p = NULL;
        }
    }
}

void enqueue(Queue *q, Process *p, int at_head){
    /*
    Create/allocate a new Node and assign a process to it
    Add the new Node to the head of the queue if at_head=1,
    otherwise add it to the tail of the queue
    */

    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->p = p;

    if(q->head == NULL){
        q->head = new_node;
        q->tail = new_node;
        new_node->left = NULL;
        new_node->right = NULL;
        q->cnt++;
        return;
    }
    if(at_head == 0){
        q->tail->right = new_node;
        new_node->left = q->tail;
        new_node->right = NULL;
        q->tail = new_node;
        q->cnt++;
    }
    else{
        q->head->left = new_node;
        new_node->right = q->head;
        new_node->left = NULL;
        q->head = new_node;
        q->cnt++;
    }
}


void dequeue(Queue* q, Process* p){
    /*
    Remove a node from the queue
    */
    Node* curr = q->head;
    // check if queue is empty
    if(curr == NULL){
        printf("Error: dequeue() called on empty queue\n");
        exit(1);
    }
    // search for the node to dequeue
    while(curr != NULL){
        if(curr->p == p){ // found the node to dequeue
            if(curr == q->head && curr == q->tail){ // curr is head and tail, only node
                q->head = NULL;
                q->tail = NULL;
                free(curr);
                q->cnt--;
                // q is now empty
                if(q->cnt != 0){
                    printf("Error: dequeue() empty queue's cnt is not 0\n");
                    exit(1);
                }
                return;
            } // if: curr is head and tail, only node
            else if(curr == q->head){ // curr is head, not tail
                q->head = curr->right;
                q->head->left = NULL;
                free(curr);
                q->cnt--;
                return;
            } // if: curr is head, not tail
            else if(curr == q->tail){ // curr is tail
                q->tail = curr->left;
                q->tail->right = NULL;
                free(curr);
                q->cnt--;
                return;
            } // if: curr is tail
            else{ // curr is not head, not tail
                curr->left->right = curr->right;
                curr->right->left = curr->left;
                free(curr);
                q->cnt--;
                return;
            } // if: curr is not head, not tail
        } // if: found the node to dequeue
        curr = curr->right;
    } // iterate through the queue
    printf("Error: dequeue() couldn't find the process to dequeue\n");
    exit(1);
}


void update_wait_time(Table* tbl){
    /*
    Increment Process.ready_wait_time for all processes in tbl.ready_q
    */

    if(tbl->ready_q == NULL){
        printf("<@%d> ERROR: update_wait_time() ready queue is empty but task not over\n", tbl->clk);
        exit(1);
    }   // just in case

    Node* curr;    
    // update wait time (ready queue)
    curr = tbl->ready_q->head;
    while(curr != NULL){
        curr->p->ready_wait_time++;
        curr = curr->right;
    }
    // update wait time (I/O)
    curr = tbl->wait_q->head;
    while(curr != NULL){
        curr->p->io_wait_time++;
        curr = curr->right;
    }
}


int CPU(Table* tbl, int algo, int _quantum){
    /* CPU()
    1. Schedule: select a Process to execute according to the scheduling algorithm specified by `algo`
        - if preemptive:
            - if running_p is NULL: DISPATCH
            - if running_p is not NULL: PREEMPT (if higher priority process in ready_q)
        - if non-preemptive:
            - if running_p is NULL and I/O is IDLE: DISPATCH
            - if running_p is NULL but I/O is BUSY: WAIT
            - if running_p is not NULL: keep running_p
    
    2. Compute:
        - Decrement tbl->quantum (if algo == 5, i.e. Round Robin)
        - Compute CPU burst for 1 CLK --> check if running_p is finished
        - Decrement running_p->io_burst_start --> check if I/O must be serviced
    
    Parameters
    ----------
    Table* tbl: tbl keeps track of all queues, running process, current time and other options required for scheduling

    int algo: specifies the scheduling algorithm to be used
        0: FCFS
        1: SJF w/o preemption
        2: SJF w/ preemption
        3: Priority w/o preemption
        4: Priority w/ preemption
        5: Round Robin - identical time quantum, no priority, always preempt

    Returns
    -------
    int: -1 if CPU is IDLE
          0 if running_p is finished (tbl->running_p == NULL)
          else: tbl->running_p->cpu_burst_rem
     */
    
    
    
    // schedule a Process to execute
    Process* out;   // return of _SJF() or _PRIO()
    switch(algo){
        case 0: // FCFS
            if(tbl->running_p == NULL && tbl->io_p == NULL){
                if(tbl->ready_q->head == NULL){
                    gannt[tbl->clk] = -1;
                    // log message: IDLE
                    printf("<@%d> IDLE: CPU and I/O are idle\n", tbl->clk);
                    return -1;  // CPU and I/O IDLE: running_p == NULL
                }
                // DISPATCH
                tbl->running_p = tbl->ready_q->head->p;
                printf("<@%d> DISPATCH: [%d] to CPU\n", tbl->clk, tbl->running_p->pid);
                tbl->running_p->state = 2;  // running
                dequeue(tbl->ready_q, tbl->running_p);   
            }
            break;           
        case 1: // SJF (non-preemptive)
            if(tbl->running_p == NULL && tbl->io_p == NULL){
                out = _SJF(tbl->ready_q, 0);
                if(out == NULL){
                    gannt[tbl->clk] = -1;
                    // log message: IDLE
                    printf("<@%d> IDLE: CPU and I/O are idle\n", tbl->clk);
                    return -1;  // CPU and I/O IDLE: running_p == NULL
                }
                // log message: DISPATCH
                printf("<@%d> DISPATCH: [%d] to CPU\n", tbl->clk, out->pid);
                tbl->running_p = out;
                tbl->running_p->state = 2;  // running
                dequeue(tbl->ready_q, tbl->running_p);
            }
            break;
        case 2: // preemptive SJF
            out = _SJF(tbl->ready_q, 1);   // NULL if ready_q is empty, else returns a Process
            if(out != NULL){
                if(tbl->running_p == NULL){
                    tbl->running_p = out;
                    printf("<@%d> DISPATCH: [%d] to CPU\n", tbl->clk, tbl->running_p->pid);
                    tbl->running_p->state = 2; // running
                    dequeue(tbl->ready_q, tbl->running_p);
                }
                else if(tbl->running_p->cpu_burst_rem > out->cpu_burst_rem){   // preempt running_p with out
                    printf("<@%d> PREEMPT: DISPATCH [%d] (%d clk) to CPU, [%d] (%d clk) to ready queue\n",
                           tbl->clk, out->pid, out->cpu_burst_rem, tbl->running_p->pid, tbl->running_p->cpu_burst_rem);
                    tbl->running_p->state = 1;  // preempt  to ready
                    enqueue(tbl->ready_q, tbl->running_p, 0);
                    tbl->running_p = out;
                    tbl->running_p->state = 2;  // running
                    dequeue(tbl->ready_q, tbl->running_p);
                }
                // else: keep running_p 
            }
            // else: if out == NULL --> keep running_p whether NULL or not.
            break;
        case 3: // priority w/o preemption
            if(tbl->running_p == NULL && tbl->io_p == NULL){
                out = _PRIO(tbl->ready_q, NULL, 0);
                if(out == NULL){
                    gannt[tbl->clk] = -1;
                    // log message: IDLE
                    printf("<@%d> IDLE: CPU and I/O are idle\n", tbl->clk);
                    return -1;  // CPU and I/O IDLE: running_p == NULL
                }
                // log message: DISPATCH
                printf("<@%d> DISPATCH: [%d] to CPU (priority: %d)\n", tbl->clk, out->pid, out->priority);
                tbl->running_p = out;
                tbl->running_p->state = 2;  // running
                dequeue(tbl->ready_q, tbl->running_p);
            }
            break;
        case 4: // priority w/ preemption
            out = _PRIO(tbl->ready_q, tbl->running_p, 1);
            if(out == NULL){
                break; // CPU is IDLE
            }
            // out != NULL       
            if(tbl->running_p == NULL){
                tbl->running_p = out;
                printf("<@%d> DISPATCH: [%d](p:%d) to CPU\n", tbl->clk, tbl->running_p->pid, tbl->running_p->priority);
                tbl->running_p->state = 2; // running
                dequeue(tbl->ready_q, tbl->running_p);
            }
            if(out != tbl->running_p){  // Premption: `out` replaces running_p
                printf("<@%d> PREEMPT: [%d](p: %d) (%d clk) to CPU, [%d](p:%d) (%d clk) to ready queue\n",
                       tbl->clk, out->pid, out->priority, out->cpu_burst_rem,
                       tbl->running_p->pid, tbl->running_p->priority ,tbl->running_p->cpu_burst_rem);
                tbl->running_p->state = 1;  // preempt  to ready queue
                enqueue(tbl->ready_q, tbl->running_p, 0);
                tbl->running_p = out;
                tbl->running_p->state = 2;  // running
                dequeue(tbl->ready_q, tbl->running_p);  // remove `out` from ready queue
            }    
            break;
        case 5: // Round Robin: identical time quantum, no priority, always preempt, renew quantum if no process in ready queue
            if(tbl->ready_q->head == NULL){ // empty ready queue
                if(tbl->running_p == NULL){
                    gannt[tbl->clk] = -1;
                    // log message: IDLE
                    printf("<@%d> IDLE: CPU idle\n", tbl->clk);
                    return -1;
                }
                if(tbl->quantum == 0){
                    // no other process to replace running_p --> renew quantum for running_p
                    printf("<@%d> RR-RENEW: [%d] (%d clk) has no other process to replace it.\n", tbl->clk, tbl->running_p->pid, tbl->running_p->cpu_burst_rem);
                    tbl->quantum = _quantum;    // reset quantum
                    break;
                }
            }
            if(tbl->running_p == NULL){
                tbl->running_p = tbl->ready_q->head->p; // first process in ready queue
                printf("<@%d> DISPATCH: [%d] to CPU\n", tbl->clk, tbl->running_p->pid);
                tbl->running_p->state = 2; // running
                dequeue(tbl->ready_q, tbl->running_p);
                tbl->quantum = _quantum;    // reset quantum
            }
            else{   // tbl->running_p is not finished
                if(tbl->quantum == 0){  // quantum expired
                    out = tbl->ready_q->head->p;
                    printf("<@%d> RR-SWITCH: [%d] (%d clk) to CPU, ", tbl->clk, out->pid, out->cpu_burst_rem);
                    printf("[%d] (%d clk) to ready queue\n", tbl->running_p->pid, tbl->running_p->cpu_burst_rem);
                    
                    tbl->running_p->state = 1;  // preempt  to ready queue
                    enqueue(tbl->ready_q, tbl->running_p, 0);
                    
                    tbl->running_p = out;
                    tbl->running_p->state = 2;  // running
                    dequeue(tbl->ready_q, tbl->running_p);  // remove `out` from ready queue

                    tbl->quantum = _quantum;    // reset quantum
                }
            }
            break;
        default:
            printf("Error: CPU() algo not implemented\n");
            exit(1);
    }


    if(tbl->running_p == NULL){
        gannt[tbl->clk] = -1;
        // log message: CPU IDLE
        printf("<@%d> IDLE: CPU is idle\n", tbl->clk);
        return -1;
    }

    // 2. compute
    if(algo == 5){tbl->quantum--;}  // if Round Robin 
    // compute CPU burst
    tbl->running_p->cpu_burst_rem--;
    gannt[tbl->clk] = tbl->running_p->pid;
    // check if running_p is finished

    if(tbl->running_p->cpu_burst_rem == 0){
        printf("<@%d> TERMINATE: [%d] to term queue \n", tbl->clk, tbl->running_p->pid);
        tbl->running_p->state = 4; // terminated
        tbl->running_p->finish_time = tbl->clk;
        tbl->running_p->turnaround_time =
        (tbl->running_p->finish_time - tbl->running_p->arrival_time);
        
        enqueue(tbl->term_q, tbl->running_p, 0);   // create Node at term
        
        tbl->running_p = NULL;
        return 0;
    }
    // I/O countdown
    tbl->running_p->io_burst_start--;
    // check if I/O must be serviced
    if(tbl->running_p->io_burst_start == 0){
        // log message: WAIT
        printf("<@%d> WAIT: [%d] (%d I/O clk) to wait queue\n", tbl->clk+1, tbl->running_p->pid, tbl->running_p->io_burst_rem);
        tbl->running_p->state = 3; // waiting
        tbl->running_p->io_burst_start = -1; //I/O only once
        enqueue(tbl->wait_q, tbl->running_p, 0);
        tbl->running_p = NULL;
        return -1;
    }     
    
    return tbl->running_p->cpu_burst_rem;
}


int io_service(Table* tbl, int algo){
    /* Service I/O burst for 1 clock cycle
    1. Schedule: If io_p is NULL, select a Process from wait_q to perform I/O,
        - if wait_q is also empty, return NULL.

    2. I/O service: decrement io_burst_rem by 1
        - if io_burst_rem == 0, move io_p to ready queue
        - if io_burst_rem > 0, return NULL

    Returns
    -------
    -1 if no process available to perform I/O
    0 if I/O burst is completed (tbl->io_p == NULL)
    else: remaining I/O burst time
     */
    // 1. Schedule
    if(tbl->io_p == NULL){
        if(tbl->wait_q->head == NULL){
            return -1;    // no process available to perform I/O
        }
        else{
            tbl->io_p = tbl->wait_q->head->p;
            printf("<@%d> I/O START: [%d] (%d I/O clock)\n", tbl->clk, tbl->io_p->pid, tbl->io_p->io_burst_rem);
            dequeue(tbl->wait_q, tbl->io_p);
            tbl->io_p->state = 3;   // waiting
        }
    }
    // 2. I/O service
    tbl->io_p->io_burst_rem--;
    return tbl->io_p->io_burst_rem;
}


Process* _SJF(Queue* q, int preemptive){
    /* 
    Returns the process with the shortest CPU burst time in the queue.
    If non-preemptive, return process with 0 I/O burst time, else return process with shortest CPU burst time.
    */

    // check if ready queue is empty
    if(q->head == NULL){
        return NULL;
    }

    Node* curr = q->head;   // not NULL
    Node* min_node = q->head;  // not NULL

    // seek ready queue
    while(curr != NULL){
        if(preemptive == 0 && curr->p->io_burst_rem == 0){
            return curr->p; // return process with 0 I/O burst
        }
        if(curr->p->cpu_burst_rem < min_node->p->cpu_burst_rem){    // schedule earlier process if same cpu_burst_rem
            min_node = curr;
        }
        curr = curr->right;
    } // curr == NULL

    return min_node->p;
}


Process* _PRIO(Queue* q, Process* running_p, int preemptive){
    /*
    Priority Scheduling: Returns Process* with the highest priority

    
    Process* running_p: currently running process
    */

    // check if ready queue is empty
    if(q->head == NULL){
        return running_p;   // empty ready queue. Keep running_p. IDLE if running_p == NULL
    }
    
    // max_priority = -1 if no running_p
    int running_priority = (running_p == NULL) ? -1 : running_p->priority;
    int max_priority = running_priority;
    Node* curr = q->head;
    Node* max_node = q->head;

    // seek ready queue
    while(curr != NULL){
        if(preemptive == 0 && curr->p->io_burst_rem == 0){
            return curr->p; // return process with 0 I/O burst
        }
        if(curr->p->priority > max_priority){
            max_node = curr;
            max_priority = curr->p->priority;
        }
        curr = curr->right;
    }

    // only preempt if ready queue has a Process with higher priority than running_p
    return (max_priority > running_priority) ? max_node->p : running_p;
}


void print_process_info(Process* p){
    // to be used after create_process()
    printf("\n[%d] Process Info\n==============\n", p->pid);
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
    printf("Arrival_time: %d\n", p->arrival_time);
    printf("Priority: %d\n", p->priority);
    printf("CPU Burst Time (Initial): %d\n", p->cpu_burst_init);
    printf("I/O Burst Time (Initial): %d\n", p->io_burst_rem);
    printf("I/O Burst Start Time: %d\n", p->io_burst_start);
}


void print_queue(Queue *q){
    // check if empty
    if(q->head == NULL){
        printf("Queue is empty\n");
        return;
    }
    // print queue info
    printf("Processes Count: %d\n", q->cnt);
    
    // print queue
    Node *curr = q->head;
    while (curr != NULL) {
        printf("[%d]-->", curr->p->pid);
        curr = curr->right;
    }
    printf("NULL\n\n");
}


void evaluate(Table* tbl, int algo, int size){
    /*
    Display evaluation info/metrics for the process in term_q
    with the provided `pid`
    if pid=0, it will display overview
    if pid=-1, it will exit evaluation mode
     */
    int pid;
    Queue* term_q = tbl->term_q;

    printf("\n\n====START EVALUATION====\n");
    // take user input for pid
    while(pid != -1){
        printf("<<Enter PID to evaluate (0: overview, -1: restart, -2: exit)>>\n");
        printf("PID: ");
        scanf(" %d", &pid); // consume newline
        printf("--------------------------------------\n");
        Node* curr = term_q->head;
        if(pid == -1){
            printf("\nExiting evaluation...\n");
            break;
        }
        else if(pid == -2){
            printf("\nExiting program...\n");
            exit(0);
        }
        if(pid==0){ // overview
            int ready_wait_time_sum = 0;
            int io_wait_time_sum = 0;
            int turnaround_time_sum = 0;
            int wait_time_sum = 0;

            int ready_wait_time_avg = 0;
            int io_wait_time_avg = 0;
            int turnaround_time_avg = 0;
            int wait_time_avg = 0;
            int num_process = term_q->cnt;

            while(curr != NULL){
                ready_wait_time_sum += curr->p->ready_wait_time;
                io_wait_time_sum += curr->p->io_wait_time;
                turnaround_time_sum += curr->p->turnaround_time;
                curr = curr->right;
            }
            wait_time_sum = ready_wait_time_sum + io_wait_time_sum;
            
            ready_wait_time_avg = ready_wait_time_sum / num_process;
            io_wait_time_avg = io_wait_time_sum / num_process;
            turnaround_time_avg = turnaround_time_sum / num_process;
            wait_time_avg = wait_time_sum / num_process;
            switch (algo) {
                case 0:
                    printf("Algorithm: FCFS\n");
                    break;
                case 1:
                    printf("Algorithm: SJF\n");
                    break;
                case 2:
                    printf("Algorithm: SRTF (SJF with preemption)\n");
                    break;
                case 3:
                    printf("Algorithm: Priority (no preemption)\n");
                    break;
                case 4:
                    printf("Algorithm: Preemptive Priority\n");
                    break;
                case 5:
                    printf("Algorithm: Round Robin\n");
                    break;
            }
            printf("Task Finished at %d\n\n", tbl->clk);
            printf("Terminated Queue:\n");
            print_queue(term_q);
            printf("\nWait time: total=%d, avg=%d\n", wait_time_sum, wait_time_avg);
            printf("Ready queue wait time: total=%d, avg=%d\n", ready_wait_time_sum, ready_wait_time_avg);
            printf("Wait queue wait time: total=%d, avg=%d\n", io_wait_time_sum, io_wait_time_avg);
            printf("Turnaround time: total=%d, avg=%d\n\n\n", turnaround_time_sum, turnaround_time_avg);

            print_gannt_chart(size);
        }
        else{ // pid != 0
            while(curr != NULL){    // search term_q for PID match
                if(curr->p->pid == pid){
                    printf("[%d] Evaluation\n", pid);
                    printf("--------------------\n");
                    printf("Wait time: %d (ready: %d, wait:%d)\n",
                    curr->p->ready_wait_time + curr->p->io_wait_time, curr->p->ready_wait_time, curr->p->io_wait_time);
                    printf("Turnaround time: %d (Arrive:%d, Terminate:%d)\n",
                    curr->p->turnaround_time, curr->p->arrival_time, curr->p->finish_time);
                    printf("Priority: %d\n\n\n", curr->p->priority);
                    break;  // break out of while(curr != NULL)
                }
                curr = curr->right;
            }
            if(curr == NULL){
                printf("Error: PID not found\n\n\n");
            }
        } // else: display evaluation info for process with pid
    }
}


void print_gannt_chart(int size){
    
    int curr_pid = 0;
    int next_pid = 0;   // when curr != next, process switched
    int i = 0;
    
    printf("====Gannt Chart====\n");
    printf("(CPU burst starts this clock)---[Process]---(next Process starts this clock)\n\n");

    // gannt chart
    printf("(0)");
    while(1){
        curr_pid = gannt[i];
        next_pid = gannt[i+1];
        if(curr_pid != next_pid){
            if(curr_pid == -1){
                printf("---CPU IDLE---(%d)", i+1);
            }
            else{
                printf("---[PID: %d]---(%d)", curr_pid, i+1);
            }
        }
        i++;
        if(i==size){
            break;
        }
    }
    // finish clock
    printf("---[PID: %d]---(%d)", gannt[i], i);
    printf("\n\n");
}


void display_config(Config* cfg){
    /* prints Config */
    printf("\n\n==============\n");
    printf("<<Config>>\n");
    printf("==============\n");
    printf("Number of processes: %d\n", cfg->num_process);
    printf("Scheduling algorithm: ");
    switch (cfg->algo) {
        case 0:
            printf("FCFS\n");
            break;
        case 1:
            printf("SJF\n");
            break;
        case 2:
            printf("SRTF (SJF with preemption)\n");
            break;
        case 3:
            printf("Priority (no preemption)\n");
            break;
        case 4:
            printf("Preemptive Priority\n");
            break;
        case 5:
            printf("Round Robin\n");
            printf("Time quantum: %d\n", cfg->quantum);
            break;
    }
    printf("\n\n");
}


void edit_config(Config* cfg){
    display_config(cfg);
    printf("\n\n<<Edit Config?>> (y/n) : ");
    char c;
    scanf(" %c", &c);

    if(c == 'y'){
        printf("\n<<Edit Config>>\n\n");
        printf("<<Enter number of processes>> (MAX=20): ");
        scanf(" %d", &cfg->num_process);
        printf("\n<<Enter scheduling algorithm>> (0~5)\n");
        printf("0: FCFS, 1: SJF, 2: SRTF, 3: Priority, 4: Preemptive Priority, 5: Round Robin\n");
        printf("Algorithm: ");
        scanf(" %d", &cfg->algo);
        if(cfg->algo == 5){
            printf("<<Enter time quantum>> (default=5): ");
            scanf(" %d", &cfg->quantum);
        }
        // use priority?
        if(cfg->algo == 3 || cfg->algo == 4){
            cfg->use_priority = true;
        }
        else{
            cfg->use_priority = false;
        }
        // random seed?
        printf("\n<<Use random seed?>> (y/n): ");
        scanf(" %c", &c);
        if(c == 'y'){
            printf("\n<<Enter random seed>> (int 1~99): ");
            int seed;
            scanf(" %d", &seed);
            srand(seed);
        }
        printf("\n<<Config updated>>\n");
        display_config(cfg);
    }
    else{
        printf("\n\n<<Using default config>>\n");
    }
}


int main(){
    while(1){
        // set default config
        srand(98);
        Config cfg = {
            .use_priority = false,
            .num_process = 5,
            .algo = 5,  // 0: FCFS, 1: SJF, 2: SRTF, 3: Priority, 4: Preemptive Priority, 5: RR
            .quantum = 5
        };
        
        // take user input for config
        edit_config(&cfg);

        // create an empty table. (empty new_pool, ready, wait, term queues are created. CLK <-- 0)
        Table *tbl = create_table(&cfg);
        
        // create processes, store them in new_pool (tbl->new_pool)
        tbl->new_pool = create_process(&cfg);

        printf("\n\n====TASK START====\n");
        // print process info
        printf("\n\n====Created Processes====\n");
        for(int i=0; i<cfg.num_process; i++){
            if(i%2 == 0){
                printf("\nDue to Ubuntu Server terminal's scroll constraint, only 2 processes will be displayed at a time\n");
                printf("Input any character to continue...\n");
                char c;
                scanf(" %c", &c);
                printf("\n");
            }
            print_process_info(tbl->new_pool[i]);
        }
        printf("\n====LAST PROCESS====\n");

        printf("\n\n====LOGS====\n");
        // loop
        while(tbl->clk < MAX_TIME){
            if(tbl->clk % 20 == 0){
                printf("\nDue to Ubuntu Server terminal's scroll constraint, only 20 cycles of log will be displayed at a time.\n");
                printf("Input any character to continue...\n");
                char c;
                scanf(" %c", &c);
                printf("\n\n");
            }
            // add processes that arrived to ready_queue
            arrived_to_ready(tbl, cfg.num_process);
            wait_to_ready(tbl, cfg.algo);
            // schedule, compute, enqueue, dequeue processes
            io_service(tbl, cfg.algo);
            CPU(tbl, cfg.algo, cfg.quantum);

            // check if all processes are terminated
            if(tbl->term_q->cnt == cfg.num_process){
                printf("<@%d> COMPLETE: All processes are terminated\n====LOG END====\n", tbl->clk);
                break;
            }

            // increment wait time for all processes in wait queue
            update_wait_time(tbl);

            tbl->clk++;
        }
        // test evalutate per pid
        evaluate(tbl, cfg.algo, tbl->clk);
    }
    return 0;
}

