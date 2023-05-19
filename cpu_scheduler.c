#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cpu_scheduler.h"

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

// global struct instances TODO: function to initialise structs with custom/default values
Config cfg={true, true, true, true, true}; // {rand_pid,  rand_arrival, rand_priority, rand_cpu_burst, rand_io_burst}
// rand_pid=false is currently not implemented

// FUNCTIONS //

Process* _create_process(Config *cfg){
    Process *new_process = (Process*) malloc(sizeof(Process));
    
    // set pid
    new_process->pid = rand()%8999 + 1001;   // rand_pid=false is currently not implemented
    // set arrival time
    new_process->arrival_time = cfg->rand_arrival ? rand()%MAX_ARRIVAL_TIME + 1 : 0;
    // set priority
    new_process->priority = cfg->rand_priority ? rand()%MAX_PRIORITY + 1 : DEFAULT_PRIORITY;
    // set cpu burst (total)
    new_process->cpu_burst_init = cfg->rand_cpu_burst ? rand()%MAX_CPU_BURST + 1 : DEFAULT_CPU_BURST;
    // set I/O burst (total)
    new_process->io_burst_init = cfg->rand_io_burst ? rand()%MAX_IO_BURST + 1 : DEFAULT_IO_BURST;
    // set state
    new_process->state = 0;


    // time related attributes are initialised to -1
    new_process->ready_wait_time = -1;
    new_process->io_wait_time = -1;
    new_process->total_wait_time = -1;
    new_process->turnaround_time = -1;
    new_process->finish_time = -1;

    return new_process; // return pointer to new process
}


void _enqueue(Queue *q, Process *p, int priority){

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


int main(){
    srand(99);
    
    int p_cnt = 0; // number of processes in job pool

    Process* job_pool[MAX_PROCESS];

    for(int i=0; i<MAX_PROCESS; i++){
        job_pool[i] = _create_process(&cfg);
        print_process_info(job_pool[i]);
        p_cnt++;
    }




    return p_cnt;
}

