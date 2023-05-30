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

#define MAX_TIME 300
// global clock variable


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
    
    ... cpu_burst_time: (Config.rand_cpu_burst=true) 1 ~ MAX_CPU_BURST (20)
                        (Config.rand_cpu_burst=false) DEFAULT_CPU_BURST (10)

    ... io_burst_start: number of CPU burst cycles before I/O must be processed. Decrements by 1 every CPU burst.
                        I/O must be processed when io_burst_start == 0
        (Config.rand_io_burst=true) (1 ~ cpu_burst_init-1): 
        (Config.rand_io_burst=false) DEFAULT_IO_START (1)

    ... io_burst_rem: (Config.rand_io_burst=true) (1 ~ cpu_burst_init/2): number of I/O burst cycles remaining
                      (Config.rand_io_burst=false) DEFAULT_IO_BURST (2)

    ... state: 0=new {1=ready, 2=running, 3=waiting, 4=terminated}
    
    ... time related attributes are initialised to 0
     */
    
    Process *new_process = (Process*) malloc(sizeof(Process));
    

    new_process->pid = rand()%8999 + 1001; // 1001 ~ 9999
    new_process->arrival_time = cfg->rand_arrival ? rand()%MAX_ARRIVAL_TIME + 1 : 0;
    new_process->priority = cfg->use_priority ? rand()%MAX_PRIORITY + 1 : DEFAULT_PRIORITY;
    new_process->cpu_burst_init = cfg->rand_cpu_burst ? rand()%MAX_CPU_BURST + 1 : DEFAULT_CPU_BURST;
    new_process->cpu_burst_rem = new_process->cpu_burst_init;
    new_process->io_burst_start = cfg->rand_io_burst ? rand()%(new_process->cpu_burst_init-1) + 1 : DEFAULT_IO_START;
    new_process->io_burst_rem = cfg->rand_io_burst ? rand()%(new_process->cpu_burst_init/2) + 1 : DEFAULT_IO_BURST;
    
    new_process->state = 0; // new

    // time related attributes are initialised to -1
    new_process->ready_wait_time = 0;
    new_process->io_wait_time = 0;
    new_process->total_wait_time = 0;
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
            if(tbl->ready_q->head != NULL){
                printf("<@%d> ARRIVE: [%d] to ready queue\n", tbl->clk, new_pool[i]->pid);
            }
            enqueue(ready_q, new_pool[i]);
            new_pool[i]->state = 1; // ready
        }
    }
}


void enqueue(Queue *q, Process *p){
    /*
    Create/allocate a new Node and assign a process to it
    */

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

    Queue* ready_q = tbl->ready_q;
    
    if(ready_q == NULL){
        printf("<@%d> ERROR: update_wait_time() ready queue is empty but task not over\n", tbl->clk);
        exit(1);
    }

    Node* curr = ready_q->head;

    while(curr != NULL){
        curr->p->ready_wait_time++;
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
                    // log message: IDLE
                    printf("<@%d> IDLE: CPU and I/O are idle\n", tbl->clk);
                    return -1;  // CPU and I/O IDLE: running_p == NULL
                }
                tbl->running_p = tbl->ready_q->head->p;
                // log message: DISPATCH
                printf("<@%d> DISPATCH: [%d] to CPU\n", tbl->clk, tbl->running_p->pid);
                tbl->running_p->state = 2;  // running
                dequeue(tbl->ready_q, tbl->running_p);   
            }
            break;           
        case 1: // SJF (non-preemptive)
            if(tbl->running_p == NULL && tbl->io_p == NULL){
                out = _SJF(tbl->ready_q);
                if(out == NULL){
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
            out = _SJF(tbl->ready_q);   // NULL if ready_q is empty, else returns a Process
            if(out != NULL){
                if(tbl->running_p == NULL){
                    tbl->running_p = out;
                    printf("<@%d> DISPATCH: [%d] to CPU\n", tbl->clk, tbl->running_p->pid);
                    tbl->running_p->state = 2; // running
                    dequeue(tbl->ready_q, tbl->running_p);
                }
                else if(tbl->running_p->cpu_burst_rem > out->cpu_burst_rem){   // preempt running_p with out
                    printf("<@%d> PREEMPT: [%d] (%d clk) to CPU, [%d] (%d clk) to ready queue",
                           tbl->clk, out->pid, out->cpu_burst_rem, tbl->running_p->pid, tbl->running_p->cpu_burst_rem);
                    tbl->running_p->state = 1;  // preempt  to ready
                    enqueue(tbl->ready_q, tbl->running_p);
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
                out = _PRIO(tbl->ready_q, NULL);
                if(out == NULL){
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
            out = _PRIO(tbl->ready_q, tbl->running_p);
            if(out == NULL){
                break; // CPU is IDLE
            }
            // out != NULL       
            if(tbl->running_p == NULL){
                tbl->running_p = out;
                printf("<@%d> DISPATCH: [%d] to CPU\n", tbl->clk, tbl->running_p->pid);
                tbl->running_p->state = 2; // running
                dequeue(tbl->ready_q, tbl->running_p);
            }
            if(out != tbl->running_p){  // Premption: `out` replaces running_p
                printf("<@%d> PREEMPT: [%d] (%d clk) to CPU, [%d] (%d clk) to ready queue\n",
                       tbl->clk, out->pid, out->cpu_burst_rem, tbl->running_p->pid, tbl->running_p->cpu_burst_rem);
                tbl->running_p->state = 1;  // preempt  to ready queue
                enqueue(tbl->ready_q, tbl->running_p);
                tbl->running_p = out;
                tbl->running_p->state = 2;  // running
                dequeue(tbl->ready_q, tbl->running_p);  // remove `out` from ready queue
            }    
            break;
        case 5: // Round Robin: identical time quantum, no priority, always preempt
            if(tbl->quantum == 0){
                printf("<@%d> RR-EXPIRE: [%d] (%d clk) to ready queue\n",
                       tbl->clk, tbl->running_p->pid, tbl->running_p->cpu_burst_rem);
                tbl->running_p->state = 1; // preempt to ready queue
                enqueue(tbl->ready_q, tbl->running_p);
                tbl->running_p = NULL;
            }
            if(tbl->running_p == NULL){
                tbl->running_p = _PRIO(tbl->ready_q, NULL);
                if(tbl->running_p == NULL){
                    break;  // CPU is IDLE
                }
                else{
                    printf("<@%d> RR-DISPATCH: [%d] to CPU\n", tbl->clk, tbl->running_p->pid);
                    tbl->quantum = _quantum;    // reset quantum
                    tbl->running_p->state = 2; // running
                    dequeue(tbl->ready_q, tbl->running_p);
                }
            }
            break;
        default:
            printf("Error: CPU() algo not implemented\n");
            exit(1);
    }   // tbl->running_p is either NULL or a Process to execute


    if(tbl->running_p == NULL){
        // waiting for I/O
        return -1;
    }

    // 2. compute
    if(algo == 5){tbl->quantum--;}  // if Round Robin 
    // compute CPU burst
    tbl->running_p->cpu_burst_rem--;
    if(tbl->running_p->cpu_burst_rem == 0){
        printf("<@%d> TERMINATE: [%d] to term queue \n", tbl->clk, tbl->running_p->pid);
        tbl->running_p->state = 4; // terminated
        tbl->running_p->finish_time = tbl->clk;
        tbl->running_p->turnaround_time =
        (tbl->running_p->finish_time - tbl->running_p->arrival_time);
        
        enqueue(tbl->term_q, tbl->running_p);   // create Node at term
        
        tbl->running_p = NULL;
        return 0;
    }
    // I/O countdown
    tbl->running_p->io_burst_start--;
    if(tbl->running_p->io_burst_start == 0){
        // log message
        if(tbl->wait_q->head != NULL){
            printf("<@%d> TO WAIT QUEUE: [%d] (%d I/O clk)\n", tbl->clk, tbl->running_p->pid, tbl->running_p->io_burst_rem);
        }   // else: io_service() will log I/O START message
        tbl->running_p->state = 3; // waiting
        enqueue(tbl->wait_q, tbl->running_p);
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
    if(tbl->io_p->io_burst_rem == 0){
        if(algo == 2 || algo==4 || algo==5){    // if preemptive, move io_p to ready queue
            printf("<@%d> READY: [%d] I/O Complete\n", tbl->clk, tbl->io_p->pid);
            tbl->io_p->io_burst_start = -1; // only perform I/O once
            tbl->io_p->state = 1;   // ready
            enqueue(tbl->ready_q, tbl->io_p);
            tbl->io_p = NULL;
            return 0;
        }
        else{   // if non-preemptive, running_p = io_p
            printf("<@%d> DISPATCH: [%d] I/O Complete\n", tbl->clk, tbl->io_p->pid);
            tbl->io_p->state = 2;   // running
            tbl->running_p = tbl->io_p;
            tbl->io_p = NULL;
            return 0;
        }
    }
    return tbl->io_p->io_burst_rem;
}


Process* _SJF(Queue* q){
    /* 
    Returns the process with the shortest CPU burst time in the queue.
    */

    // check if ready queue is empty
    if(q->head == NULL){
        return NULL;
    }

    Node* curr = q->head;   // not NULL
    Node* min_node = q->head;  // not NULL

    // seek ready queue
    while(curr != NULL){
        if(curr->p->cpu_burst_rem < min_node->p->cpu_burst_rem){    // schedule earlier process if same cpu_burst_rem
            min_node = curr;
        }
        curr = curr->right;
    } // curr == NULL

    return min_node->p;
}


Process* _PRIO(Queue* q, Process* running_p){
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
    // prints process attributes (except time related attributes for evaluation)
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
    printf("Priority: %d\n", p->priority);
    printf("CPU Burst Time (Initial): %d\n", p->cpu_burst_init);
    printf("I/O Burst Initial Time: %d\n", p->io_burst_rem);
    printf("I/O Burst Start Time: %d\n", p->io_burst_start);
    printf("Arrival_time: %d\n\n", p->arrival_time);
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


void evaluate(Table* tbl){
    /*
    Display evaluation info/metrics for the process in term_q
    with the provided `pid`
    if pid=0, it will display average values for all processes
    in the term_q
    
    TODO: modify to return an eval struct type which contain
    time related attributes for all processes + avg + sums,
    which can be searched up with user input.

     */
    int pid;    // -1: exit, 0: avg, 1~: PID
    Queue* term_q = tbl->term_q;

    // take user input for pid
    while(pid != -1){
        printf("<<Enter PID to evaluate (0: average, -1: exit)>>\n");
        printf("PID: ");
        scanf(" %d", &pid); // consume newline
        printf("--------------------------------------\n");
        Node* curr = term_q->head;
        if(pid == -1){break;}
        if(pid==0){
            int ready_wait_time_sum = 0;
            int turnaround_time_sum = 0;
            // int io_wait_time_sum = 0;
            // int total_wait_time_sum = 0;

            while(curr != NULL){
                ready_wait_time_sum += curr->p->ready_wait_time;
                turnaround_time_sum += curr->p->turnaround_time;
                // io_wait_time_sum += curr->p->io_wait_time;
                // total_wait_time_sum += curr->p->total_wait_time;
                curr = curr->right;
            }

            int num_process = term_q->cnt;
            int ready_wait_time_avg = ready_wait_time_sum / num_process;
            int turnaround_time_avg = turnaround_time_sum / num_process;

            printf("Avg wait time: %d\n", ready_wait_time_avg);
            printf("Avg turnaround time: %d\n\n\n", turnaround_time_avg);
        } // display average values for all processes in term_q
        else{ // pid != 0
            while(curr != NULL){    // search term_q for PID match
                if(curr->p->pid == pid){
                    print_process_info(curr->p);
                    printf("[%d] Evaluation\n", pid);
                    printf("--------------------\n");
                    printf("Wait time in ready queue: %d\n", curr->p->ready_wait_time);
                    printf("Turnaround time: %d\n\n\n", curr->p->turnaround_time);
                    printf("Arrival time: %d\n", curr->p->arrival_time);
                    printf("Finish time: %d\n", curr->p->finish_time);
                    printf("CPU burst time: %d\n", curr->p->cpu_burst_init);
                    printf("Priority: %d\n", curr->p->priority);
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


int main(){
    // set test init
    Config cfg = {
        .rand_pid = true,
        .rand_arrival = true,
        .use_priority = false,
        .rand_cpu_burst = true,
        .rand_io_burst = true,
        .num_process = 10,
        .algo = 0,
        .quantum = 5
    };
    srand(98);
    

    // create an empty table. (empty new_pool, ready, wait, term queues are created. CLK <-- 0)
    Table *tbl = create_table(&cfg);
    
    // create processes, store them in new_pool (tbl->new_pool)
    tbl->new_pool = create_process(&cfg);

    // print process info
    for(int i=0; i<cfg.num_process; i++){
        print_process_info(tbl->new_pool[i]);
    }

    // loop
    while(tbl->clk < 150){
        // add processes that arrived to ready_queue
        arrived_to_ready(tbl, cfg.num_process);

        // schedule, compute, enqueue, dequeue processes
        io_service(tbl, cfg.algo);
        CPU(tbl, cfg.algo, cfg.quantum);
        
        // check if all processes are terminated
        if(tbl->term_q->cnt == cfg.num_process){
            printf("<@%d> COMPLETE: All processes are terminated\n", tbl->clk);
            break;
        }
        
        // increment wait time for all processes in wait queue
        update_wait_time(tbl);

        tbl->clk++;
    }

    printf("\nReady Queue at %d\n", tbl->clk);
    print_queue(tbl->ready_q);

    printf("\nTerm Queue at %d\n", tbl->clk);
    print_queue(tbl->term_q);

    // test evalutate per pid
    evaluate(tbl);

    return 0;
}

