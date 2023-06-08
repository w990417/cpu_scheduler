# OS Term Project - CPU Scheduling Simulator (COSE231)

## 1. Components
<br>

### `Create_Process()` : Creates and initialises processes.

    Each process will be assigned the following attributes:  
    
    - PID (Process ID)
    - Arrival Time
    - CPU burst time (randomly generated)
    - I/O burst time (randomly generated)
    - Priority (randomly generated)

    Following attributes will be used for scheduling algorithms:

    - State (Ready, Running, Waiting etc.)
    - Consumed/remaining CPU burst time
    - Consumed/remaining I/O burst time
    - Time spent in various queues

<br>

### `Config()` : System configuration
    Config() may be used to specify the following:

    - Number of processes to be created
    - Ready/waiting queue implementation/selection

<br>

### `Schedule()` : Scheduling algorithms

    Scheduling algorithms to be implemented:
    - FCFS (First Come First Served)
    - SJF (Shortest Job First)
    - SJF with preemption
    - Priority Scheduling (w/ preemption)
    - Priority Scheduling (w/o preemption)
    - RR (Round Robin)

<br>

### `Evaluation()` : Evaluate and compare the performance of scheduling algorithms

    Following metrics will be used to evaluate the performance of scheduling algorithms:
    - Average waiting time
    - Average turnaround time

<br>

## 2. Marking Criteria

1. method for data creation (-5)

       PID, arrival time, CPU burst time, I/O burst time, priority
        
2. implementation of I/O operation (-5)

        I/O interrupt may be dynamic (random) or static
        1 or more I/O burst

3. Scheduling Algorithms Implementation (-5 each)

        FCFS, 
        SJF (preemptive/non-preemptive),
        Priority (preemptive/non-preemptive),
        Round Robin

9. Gantt Chart (-5)

        for successful display of Gantt chart

10. Evaluation (-5)

        successful evaluation of waiting/turnaround time

11. Additional Functions

        2-4 points per additional function/functionality

12. QnA

        Must be able to demonstrate/explain the workings of the written codes

13. Report (-20)

        refer to "3.Report"

## 3. Report