#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"

// global constants
#define MAX_PROCESS 5
#define MAX_ARRIVAL_TIME 20
#define MAX_PRIORITY 4
#define DEFAULT_PRIORITY 3
#define MAX_CPU_BURST 20
#define DEFAULT_CPU_BURST 10
#define MAX_IO_BURST 5
#define DEFAULT_IO_BURST 2


// global time variables
int TICK = 0; // current time

/* STRUCTS
Status: keeps track of processes and their states
Process: holds information about a process
Config: keeps track of current configuration
 */


typedef struct {
    int process_cnt;        // initialised to 0
    int pids[MAX_PROCESS];  // initialised to -1
}Status;


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


// global instance of Status
Status status = {0 , {-1}}; // {process_cnt, pids[]}
// global instance of Config
Config cfg={true, true, false, true, true}; // {rand_pid,  rand_arrival, rand_priority, rand_cpu_burst, rand_io_burst}


// FUNCTIONS //
Process* _create_process(Config *cfg){
    Process *new_process = (Process*) malloc(sizeof(Process));
    
    // set pid
    new_process->pid = cfg->rand_pid ? rand()%8999 + 1001 : 1001 + status.process_cnt;
    // set arrival time
    new_process->arrival_time = cfg->rand_arrival ? rand()%MAX_ARRIVAL_TIME + 1 : 0;
    // set priority
    new_process->priority = cfg->rand_priority ? rand()%MAX_PRIORITY + 1 : DEFAULT_PRIORITY;
    // set cpu burst (total)
    new_process->cpu_burst_init = cfg->rand_cpu_burst ? rand()%MAX_CPU_BURST + 1 : DEFAULT_CPU_BURST;
    // set I/O burst (total)
    new_process->io_burst_init = cfg->rand_io_burst ? rand()%MAX_IO_BURST + 1 : DEFAULT_IO_BURST;
    // add to ready queue if arrival_time <= TICK
    if (new_process->arrival_time <= TICK){
        new_process->state = 1;
        to_ready(); // add to ready queue
    } else {
        new_process->state = 0;
        to_new(); // add to new queue
    }

    // update status
    status.process_cnt++;
    status.pids[status.process_cnt-1] = new_process->pid;

    return new_process; // return pointer to new process
}

// prints process attributes (except time related attributes)
void process_info(Process* p){

    // equivalent to python print(f"[{pid}]", '\n', '='*10, sep='')
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




int main(){
    // test _create_process()
    Process *p = _create_process(&cfg);
    process_info(p);

    Process *p2 = _create_process(&cfg);
    process_info(p2);

    return 0;
}

