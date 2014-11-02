#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <strings.h>
#include <math.h>



/*  ================== process struct ================= */

typedef struct {
	int A; /* arrival time */
	int B; /* burst time */
	int C; /* total CPU time */
	int IO; /* IO burst time */
    int pid; /* process id number (based on loc in input file) */
	int waitTime; /* time in ready state */
	int IOtime; /* time in blocked state */
	int finishTime; /* finishing time */
	int state; /* -1:unstarted, 0:ready, 1:running, 2:blocked, 3:finished */
	int blockedTimer; /* time remaining in blocked state */
	int runningTimer; /* time remaining in running state */
    int CPUleft; /* how much time left until finished */
    int timeIntoRQ; /* time when last got placed in ready Q */
    int justBlocked; /* flag whether or not JUST got out of blocked state */
    int Qtimer; /* for RR, current quantum timer */
} process;

/* ================= helper functions declarations ================= */

int randomOS(FILE *file, int U, int CPUleft);
int getBurstTime();
void printProcess(process p);
void printProcessSummary(process p);
void printState(process processes[], int currTime);
void printFinalSummary(process processes[]);
void printQ(int *q);
void printT(int *q);
void enqueue(int *q, int p);
int dequeue(int *q);
int qIsEmpty(int *q);
void sortQSJF(int *temp, int n, process processes[]);
int QSize(int *q);
int newPtoTemp(process processes[], int currTime, int *temp);
void tempToReady(int *temp, int *q, int c, int currTime, process processes[]);
process createProcess(int a, int b, int c, int io, int id );
void sortProcByArrival(process processes[]);
void readFile(FILE *file, process processes[]);
void tieBreak(int *temp, int n, process processes[]);
int updateBlocked(process processes[], int *temp, int c);
int updateRun(process processes[], int currTime, int *temp, int c);
int somethingRunning(process processes[]);
void updateQ(process processes[], int *q, int currTime, int *temp, int c);
void moveProcToRunning(process processes[], int *q, int currTime);
int allDone(process processes[]);
void zeroArr(int *arr);

/* ================= schedulers ================= */

void FCFS(process processes[], int *readyQ, int *temp);
void uniprogrammed(process processes[], int *readyQ, int *temp);
void RR( process processes[], int *readyQ, int *temp );
void SJF( process processes[], int *readyQ, int *temp );



/* ================= global variables ================= */

int numProcs = 0;
FILE *randomNums;
int Q = -1; /* gets set in RR */
int finalFinish;
int totCPU;
int totIO;
int verbose = 0;


/* ================= main program ================= */

int main( int argc, char *argv[] ) {


    /* deal with command line arguments */
    if(strcmp(argv[1], "--verbose") == 0) {
        verbose = 1;
    }

    int loc = 1;
    if (verbose) {
        loc++;
    }

	FILE *file = fopen( argv[loc], "r" );

	if ( file == 0 ) {
        printf( "Could not open file\n" );
        exit(1);
    }
    
    /* check how many processes there are total */
    fscanf(file,"%d",&numProcs);
    
    /* create an array of numProcesses processes */
    process processes[numProcs]; 

    /* populate array of processes */
    readFile(file, processes);

    /* sort processes by arrival time */
    sortProcByArrival(processes);

    /* create a ready queue of size number of processes */
    int readyQ[numProcs];
    zeroArr(readyQ);

    /* create temp array */
    int temp[numProcs];
    zeroArr(temp);
    
    /* open random numbers file */
    randomNums = fopen( argv[loc+1], "r" );

	if ( randomNums == 0 ) {
        printf( "Could not open file\n" );
        exit(1);
    }

    /* determine which type of scheduler to run */
    char scheduler = *argv[loc+2];

    switch(scheduler) {
        case('f'):
            FCFS(processes, readyQ, temp);
            printf("\nThe scheduling process used was FCFS\n");
            break;
        case('u'):
            uniprogrammed(processes, readyQ, temp);
            printf("\nThe scheduling process used was uniprogrammed\n");
            break;
        case('r'):
            RR(processes, readyQ, temp);
            printf("\nThe scheduling process used was Round Robin\n");
            break;
        case('s'):
            SJF(processes, readyQ, temp);
            printf("\nThe scheduling process used was Shortest Job First\n");
            break;

        default:
            printf("Not a valid scheduler. Exiting.\n");
            exit(1);
    }

    /* print process summaries */
    printf("\n");
    for(int i = 0; i < numProcs; i++) {
        printf("Process %d:\n", i);
        printProcessSummary(processes[i]);
        printf("\n");
    }

    printFinalSummary(processes);
    

}

/* ================= helper functions declarations ================= */

void printProcess(process p) {

    printf("process id: %d\n", p.pid);
    printf("arrival time: %d\n", p.A);
    printf("burst time: %d\n", p.B);
    printf("total cpu time: %d\n", p.C);
    printf("io burst time: %d\n", p.IO);
    printf("current state: %d\n", p.state);
    printf("time left in blocked state: %d\n", p.blockedTimer);
    printf("time left in running state: %d\n", p.runningTimer);
    printf("CPU time left: %d\n", p.CPUleft);
    printf("total time in ready state so far: %d\n", p.waitTime);
    printf("total time in blocked state so far: %d\n", p.IOtime);
    printf("finish time: %d\n", p.finishTime);
}

void printProcessSummary(process p) {
    printf("\t(A,B,C,IO) = (%d,%d,%d,%d)\n", p.A, p.B, p.C, p.IO);
    printf("\tFinishing time: %d\n", p.finishTime);
    printf("\tTurnaround time: %d\n", (p.finishTime - p.A));
    printf("\tI/O time: %d\n", p.IOtime);
    printf("\tWaiting time: %d\n", p.waitTime);
}

void printState(process processes[], int currTime) {
    printf("Before cycle %5d: ", currTime);
    for(int i = 0; i < numProcs; i++) {
        if(processes[i].state == -1) {
            printf("%15s %3d ", "unstarted", 0);
        }
        else if(processes[i].state == 0) {
            printf("%15s %3d ","ready", 0);
        }
        else if(processes[i].state == 1) {
            printf("%15s %3d ", "running", processes[i].runningTimer);
        }
        else if(processes[i].state == 2) {
            printf("%15s %3d ", "blocked", processes[i].blockedTimer);
        }
        else {
            printf("%15s %3d ", "finished", 0);
        }
    }
    printf("\n");
}

void printFinalSummary(process processes[]) {

    double cpuU = (double) totCPU / (double) finalFinish;
    double ioU = (double) totIO / (double) finalFinish;
    double thru; 

    int turn = 0;
    int wait = 0;
    for(int i = 0; i < numProcs; i++) {
        turn += (processes[i].finishTime - processes[i].A);
        wait += (processes[i].waitTime);
        totCPU += processes[i].C;
    }

    double avgTurn = (double)turn / (double) numProcs;
    double avgWait = (double) wait / (double) numProcs;
    thru = 100 / ((double)(finalFinish) / (double) numProcs);


    printf("Summary Data: \n");
    printf("\tFinishing Time: %d\n", finalFinish);
    printf("\tCPU Utilization: %f\n", cpuU);
    printf("\tI/O Utilization: %f\n", ioU);
    printf("\tThroughput: %f processes per hundred cycles\n", thru);
    printf("\tAverage turnaround Time: %f\n", avgTurn);
    printf("\tAverage waiting Time: %f\n", avgWait);
}

void printQ(int *q) {
    printf("Ready Queue");
    for (int i = 0; i < numProcs; i++) {
        printf(" : %d", q[i]);
    }
    printf("\n");
}

void printT(int *q) {
    printf("Temp Array");
    for (int i = 0; i < numProcs; i++) {
        printf(" : %d", q[i]);
    }
    printf("\n");
}

process createProcess(int a, int b, int c, int io, int id ) {
    process newProcess = {
        a,
        b,
        c,
        io,
        id,
        0,
        0,
        0,
        -1,
        0,
        0,
        c,
        0,
        0,
        0
    };

    return newProcess;        
}

void sortProcByArrival(process processes[]) {
    int c,d;
    process t;
    int n = numProcs;

    for (c = 1 ; c <= n - 1; c++) {
        d = c;
     
        while ( d > 0 && processes[d].A < processes[d-1].A) {
          t = processes[d];
          processes[d] = processes[d-1];
          processes[d-1] = t;
          d--;
        }
    }

    for(int i = 0; i < numProcs; i++) {
        processes[i].pid = i;
    }
}

void enqueue(int *q, int p) {
    //place a process at back of array
	//printf("*****ENQUEUEING NOW!*****\n");

    for(int i = 0; i < numProcs; i++) {
        if(q[i] == -1) {
            q[i] = p;
            break;
        }
    }
}

int dequeue(int *q) {
    //remove first item from array and shift all others up

    //printf("*****DEQUEUEING NOW!*****\n");

    int pid = q[0]; //if array is empty, q[0] = -1 which is good
    
    //shift all elements up
    for (int i = 0; i < numProcs-1; i++) {
        q[i] = q[i+1];
    }
    q[numProcs-1] = -1; //to preserve integrity of queue 

    return pid;
}

int qIsEmpty(int *q) {
    for(int i = 0; i < numProcs; i++) {
        if (q[i] != -1) return 0;
    }
    return 1;
}

int newPtoTemp(process processes[], int currTime, int *temp) {
    //puts processes created at currTime in temp array
    //returns number of processes just added
    int c = 0;
    for(int i = 0; i < numProcs; i++) {
        if(processes[i].A == currTime) {
            temp[c] = processes[i].pid;
            c++;
        }
    }
    return c;
}

void tempToReady(int *temp, int *q, int c, int currTime, process processes[]) {
    //moves all c process from temp array to readyQ with ties taken care of
    //sets these processes arrival to Q time as current time
    tieBreak(temp,c,processes);
    
    for(int i = 0; i < c; i++) {
        enqueue(q, temp[i]);
        processes[temp[i]].timeIntoRQ = currTime;
        processes[temp[i]].state = 0;
    }
}

void updateQ(process processes[], int *q, int currTime, int *temp, int c) {

	//printf("***** UPDATING QUEUE NOW!*****\n");
    // increments wait counter for all processes that are in ready Q
    // adds elements that have just arrived to temporary buffer
    for(int i = 0; i < numProcs; i++) {
        //check if in ready state
        if(processes[i].state == 0) {
            processes[i].waitTime += 1;
        }
        if(processes[i].state == -1 && processes[i].A == currTime) {
            //put these in temporary buffer
            temp[c] = processes[i].pid;
            c++;
        }
        //at this point we should have all the elements
        //that may need to get into the actual ready q in 
        //the temp buffer. add them in proper order (tie 
        //breakers) to readyQ 
        if(c > 0) {
            tieBreak(temp, c, processes);
            for(int i = 0; i < c; i++) {
                enqueue(q, temp[i]);
            }
        } 
    }
}

int updateRun(process processes[], int currTime , int *temp, int c) {

    /* this function decrements the current running timer for the
       running process. if the timer reaches 0, state moves to 
       finished with finish time = curr time. otherwise moves to 
       blocked or gets preempted
    */

    // returns 1 if it has finished running, 0 otherwise
    for(int i = 0; i < numProcs; i++) {
        //check if in blocked state
        if(processes[i].state == 1 && !(processes[i].justBlocked)) {
            totCPU += 1;
            processes[i].runningTimer -= 1;
            processes[i].CPUleft -= 1;
            processes[i].Qtimer -= 1;

            //if this causes their timer to end,
            //check if need to move to blocked state with new blockedTimer
            //or to finished state
            if(processes[i].CPUleft == 0) { //terminated
                processes[i].state = 3;
                processes[i].finishTime = currTime;
                finalFinish = currTime;
            }
            else if(processes[i].runningTimer == 0) { //block
                processes[i].state = 2;
                processes[i].blockedTimer = randomOS(randomNums, processes[i].IO, processes[i].IO);
            }

            else if(Q == 2 && processes[i].Qtimer == 0) {
                temp[c] = processes[i].pid;
                c++;
            }
        }
        else if(processes[i].justBlocked == 1) {
            processes[i].justBlocked = 0;
        }
    }
    return c;
}

int somethingRunning(process processes[]) {

	//printf("***** CHECKING IF SOMETHING IS RUNNING NOW!*****\n");
    //return 1 if something is running, otherwise 0
    for(int i = 0; i < numProcs; i++) {
        if(processes[i].state == 1) return 1;
    }
    return 0;
}

void moveProcToRunning(process processes[], int *q, int currTime) {
    //need to move first process off queue to running state
    //calculate how long it's been in Q this time and add to
    //total running wait time in Q
    int p = dequeue(q);
    processes[p].state = 1;
    if(processes[p].runningTimer == 0) {
        processes[p].runningTimer = randomOS(randomNums, processes[p].B, processes[p].CPUleft);
    }
    //processes[p].runningTimer = randomOS(randomNums, processes[p].B, processes[p].CPUleft);
    //printf("difference is: %d\n", (currTime - processes[p].timeIntoRQ) );
    processes[p].waitTime += (currTime - processes[p].timeIntoRQ);
    if(Q == 2) {
        processes[p].Qtimer = 2;
    }
}

int updateBlocked(process processes[], int *temp, int c) {

    //add processes that have finished blocking to temp array
    int flag = 0;
    for(int i = 0; i < numProcs; i++) {
        //check if in blocked state
        if(processes[i].state == 2) {
            flag = 1;
            processes[i].blockedTimer -= 1;
            processes[i].IOtime += 1;
            //if this causes their timer to end add to temp array
            if(processes[i].blockedTimer == 0) {
                //processes[i].state = 0;
                temp[c] = processes[i].pid;
                c++;
                processes[i].justBlocked = 1;

            }
        } 
    }
    if (flag) { totIO += 1; }
    return c;
}

void tieBreak(int *temp, int n, process processes[]) {

    //use insertion sort twice to sort the elements in 
    //the temp array to be placed in the readyQ. first 
    //sort by arrival time and then by pid. since insertion
    //sort is stable, this will correctly break all ties
    //n is number of elements in temp array

    int c,d,t;

    for (c = 1 ; c <= n - 1; c++) {
        d = c;
     
        while ( d > 0 && processes[temp[d]].A < processes[temp[d-1]].A) {
          t = temp[d];
          temp[d] = temp[d-1];
          temp[d-1] = t;
          d--;
        }
    }

    for (c = 1 ; c <= n - 1; c++) {
        d = c;
     
        while ( d > 0 && processes[temp[d]].pid < processes[temp[d-1]].pid) {
          t = temp[d];
          temp[d] = temp[d-1];
          temp[d-1] = t;
          d--;
        }
    } 
}

void sortQSJF(int *temp, int n, process processes[]) {
    int c,d,t;

    for (c = 1 ; c <= n - 1; c++) {
        d = c;
     
        while ( d > 0 && processes[temp[d]].CPUleft < processes[temp[d-1]].CPUleft) {
          t = temp[d];
          temp[d] = temp[d-1];
          temp[d-1] = t;
          d--;
        }
    } 
}

int QSize(int *q) {
    int n = 0;
    for(int i = 0; i < numProcs; i++) {
        if(q[i] != -1) {
            n++;
        }
    }
    return n;
}

int allDone(process processes[]) {
    //return 0 when at least one process not finished, 1 otherwise
    for(int i = 0; i < numProcs; i++) {
        if(processes[i].state != 3) {
            return 0;
        }
    }
    return 1;
}

void zeroArr(int *arr) {
    for(int i = 0; i < numProcs; i++) {
        arr[i] = -1;
    }
}

void readFile(FILE *file, process processes[]) {
    
    char paren1;
    int a;
    int b;
    int c;
    int io;
    char paren2;

    for(int i = 0; i < numProcs; i++) {
        fscanf(file, " %c %d %d %d %d %c", &paren1, &a, &b, &c, &io, &paren2);
        process newP = createProcess(a,b,c,io,i);
        processes[i] = newP;
    }
}

int randomOS(FILE *file, int U, int CPUleft) {

    /* gets reference to file, grabs next number, 
        mods it with burst time and adds 1.
        if it's greater than cpu left returns cpu left */
	int r;

    fscanf(file,"%d",&r);
   
    r = 1 + (r % U);

    if (r < CPUleft) {
    	return r;
    }
    else {
    	return CPUleft;
    }
} 



/* ============= INDIVIDUAL SCHEDULERS ================ */

void FCFS(process processes[], int *readyQ, int *temp) {

    /* 
        FCFS algorithm. Initializes states before entering while loop.
        Continues so long as some processes still running. Not preemptive.
        global vars keep track of system usage. 
    */
    int currTime = 0;
    int c = 0;

    if(verbose) {
        printState(processes, currTime);
    }

    c = newPtoTemp(processes, currTime, temp);
    tempToReady(temp, readyQ, c, currTime, processes);


    if(!somethingRunning(processes) && !qIsEmpty(readyQ)) {
        moveProcToRunning(processes, readyQ, currTime);
    }

    currTime++;
    c = 0;
    zeroArr(temp);


    //printState(processes, currTime);
    while(!allDone(processes)) {

        if(verbose) {
            printState(processes, currTime);
        }  

        //put all newly created processes in temp array
        c = newPtoTemp(processes, currTime, temp);

        //update all blocked and put newly unblocked processes in temp array
        c = updateBlocked(processes, temp, c);

        //put all processes from temp array onto readyQ
        tempToReady(temp, readyQ, c, currTime, processes);

        updateRun(processes, currTime, temp, c);

        //if nothing is running and our queue isn't empty, 
        //put first proc from readyQ in running position
        if(!somethingRunning(processes) && !qIsEmpty(readyQ)) {
            moveProcToRunning(processes, readyQ, currTime);
        }

        currTime++;
        zeroArr(temp);
        c = 0;

    }
}

void uniprogrammed(process processes[], int *readyQ, int *temp) {
    int currTime = 0;
    int c = 0;

    if(verbose) {
        printState(processes, currTime);
    }    

    /* put all processes on readyQ in their correct order */
    for(int i = 0; i < numProcs; i++) {
        if(processes[i].A == 0) {
            enqueue(readyQ, processes[i].pid); 
            processes[i].state = 0;
            processes[i].timeIntoRQ = 0;
            //printf("processes[%d].timeIntoRQ = %d\n", i, processes[i].timeIntoRQ);
        }
    }

    if(!somethingRunning(processes) && !qIsEmpty(readyQ)) {
        moveProcToRunning(processes, readyQ, currTime);
    }

    currTime++;
    c = 0;
    zeroArr(temp);

    int currProc = 0;

    while (currProc < numProcs) {
        processes[currProc].state = 1;
        processes[currProc].waitTime += (currTime - processes[currProc].timeIntoRQ - 1);
        processes[currProc].runningTimer = randomOS(randomNums, processes[currProc].B, processes[currProc].CPUleft);
        while(processes[currProc].state != 3) {

            if(verbose) {
                printState(processes, currTime);
            }

            c = updateBlocked(processes, temp, c);
            if(c > 0) {
                //if it got unblocked, put it in my temporary RQ
                processes[currProc].state = 0;
            }
            updateRun(processes, currTime, temp, c);

            if(processes[currProc].state == 0) {
                processes[currProc].state = 1;
                processes[currProc].runningTimer = randomOS(randomNums, processes[currProc].B, processes[currProc].CPUleft);

            }

            currTime++;
            c = 0;

            for(int i = 0; i < numProcs; i++) {
                if(processes[i].A == currTime) {
                    enqueue(readyQ, processes[i].pid); 
                    processes[i].state = 0;
                    processes[i].timeIntoRQ = currTime;
                }
            }

        }
        currProc++;

    }
}

void RR( process processes[], int *readyQ, int *temp ) {

    //set Q to quantum
    Q = 2;

    /* initialization steps */
    int currTime = 0;
    int c = 0;

    if(verbose) {
        printState(processes, currTime);
    }   
    c = newPtoTemp(processes, currTime, temp);
    tempToReady(temp, readyQ, c, currTime, processes);


    if(!somethingRunning(processes) && !qIsEmpty(readyQ)) {
        moveProcToRunning(processes, readyQ, currTime);
    }

    currTime++;
    c = 0;
    zeroArr(temp);


    //printState(processes, currTime);
    while(!allDone(processes)) {

        if(verbose) {
            printState(processes, currTime);
        }

        //put all newly created processes in temp array
        c = newPtoTemp(processes, currTime, temp);

        //update all blocked and put newly unblocked processes in temp array
        c = updateBlocked(processes, temp, c);

        c = updateRun(processes, currTime, temp, c);

        //put all processes from temp array onto readyQ
        tempToReady(temp, readyQ, c, currTime, processes);


        //if nothing is running and our queue isn't empty, 
        //put first proc from readyQ in running position
        if(!somethingRunning(processes) && !qIsEmpty(readyQ)) {
            moveProcToRunning(processes, readyQ, currTime);
        }

        currTime++;
        zeroArr(temp);
        c = 0;
    }
}

void SJF(process processes[], int *readyQ, int *temp) {
    int currTime = 0;
    int c = 0;
    int n = 0;

    if(verbose) {
        printState(processes, currTime);
    }    
    c = newPtoTemp(processes, currTime, temp);
    tempToReady(temp, readyQ, c, currTime, processes);

    n = QSize(readyQ);
    sortQSJF(readyQ, n, processes);


    if(!somethingRunning(processes) && !qIsEmpty(readyQ)) {
        moveProcToRunning(processes, readyQ, currTime);
    }

    currTime++;
    c = 0;
    zeroArr(temp);


    //printState(processes, currTime);
    while(!allDone(processes)) {

        if(verbose) {
            printState(processes, currTime);
        }

        //put all newly created processes in temp array
        c = newPtoTemp(processes, currTime, temp);

        //update all blocked and put newly unblocked processes in temp array
        c = updateBlocked(processes, temp, c);

        //put all processes from temp array onto readyQ
        tempToReady(temp, readyQ, c, currTime, processes);

        n = QSize(readyQ);
        sortQSJF(readyQ, n, processes);

        updateRun(processes, currTime, temp, c);

        //if nothing is running and our queue isn't empty, 
        //put first proc from readyQ in running position
        if(!somethingRunning(processes) && !qIsEmpty(readyQ)) {
            moveProcToRunning(processes, readyQ, currTime);
        }

        currTime++;
        zeroArr(temp);
        c = 0;

    }
}





