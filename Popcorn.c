#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>

//constant control parameters to mess around with
#define KERNEL_DEPTH 4
#define PARTIAL_POS_SCORE_MAX 11    //max +score +1 for inclusivity= partialposscores*kerneldepth
#define PARTIAL_NEG_SCORE_MAX 21    //max -score +1 for inclusivity= partialnegscores*kerneldepth
#define COEFF_MAX 15     //limiting coefficient values to prevent absurd calculations.

#define OPERATION_TIME_COST_MS 10  //waiting period between each matrix operation

#define WAIT delay_ms(OPERATION_TIME_COST_MS);  //macro for cleanliness in matrix ops
#define TEST getkernelcontents(k); //matrix dump inside matrix ops

#define MICRO 1000000
#define NANO 1000000000
#define BURN_MIN 10
#define BURN_MAX 45

#define EARLY_CANCEL_PENALTY 100    //score penalty invoked for each second of delay
#define BURN_PENALTY 200

atomic_int SCORE=0;
int TIMEREMAINING=0;

struct {
    const char *label;
    const int value;
} bagsize[] = {
    //replaced all numbers to logical shifts to make it look cooler
    //can also be implemented using array of strings and then an array of nums as shifts
    //but im lazy sorry
    {"O", 1},
    {"XS",1<<3},
    {"S",1<<4},
    {"M",1<<5},
    {"L",1<<6},
    {"XL",1<<7},
    {"X2L",1<<8},
    {"X3L",1<<9},
    {"X4L",1<<10}
};

typedef struct{
    double A[KERNEL_DEPTH][KERNEL_DEPTH];
    double b[KERNEL_DEPTH];
} popkernel;

int convertmap(const char* key){
    int n = sizeof(bagsize)/sizeof(bagsize[0]);
    for (int i = 0; i < n; i++) {
        if (!strcmp(bagsize[i].label,key)){     //strcmp returns 0 on success so invert
            return bagsize[i].value;
        }
    }
    return -1;
}

//Ctrl-C = stop microwaving prematurely - cancel the microthread which in turns cancels every other thread
//score dump...
void end_game(int sig){
    if(sig==2){
        printf("Microwave Stopped. There were %d seconds remaining.\n", TIMEREMAINING);

        atomic_fetch_add(&SCORE,-EARLY_CANCEL_PENALTY*TIMEREMAINING);
        printf("Your score is: %d\n",SCORE);
        exit(0);
    }
}

//matrix ops
//every matrix step has been artificially delayed for the sake of gameplay using WAIT macro defined above

void rowswap(popkernel* k, int r1, int r2){
    double temp[KERNEL_DEPTH];
    memcpy(temp,k->A[r1],sizeof(double)*KERNEL_DEPTH);
    memcpy(k->A[r1],k->A[r2],sizeof(double)*KERNEL_DEPTH);
    WAIT
    memcpy(k->A[r2],temp,sizeof(double)*KERNEL_DEPTH);
    //TEST
}

//checkpivot gives us the index at which the target pivot is located, aka the highest cij for xj specified by varpos.
int checkpivot(popkernel* k, int varpos){
    int max=0; int index = 0;
    for(int i=0; i<KERNEL_DEPTH; i++){
        WAIT
        if(k->A[i][varpos]>max){
            max=k->A[i][varpos];
            index=i;
        }
    }
    return index;
}

//THE MATRIX FUNCTIONALITY ITSELF HAS SOME BUGS SO THIS FUNCTION IS STILL INCOMPLETE
int partialpivot(popkernel* k){
    //Ax=b
    int x[KERNEL_DEPTH]={0};

    //Use partial pivoting find solution vector
    //do row operations to bring the most significant x1 to the top
    for(int i=0;i<KERNEL_DEPTH-1; i++){
        //row swap pivot with current row and reduce every other row
        WAIT
        int pivot=checkpivot(k,i);
        rowswap(k,pivot,i);
        WAIT
        //now cii is the most significant coefficient for xi
        //reduce proceeding rows (preceding ones are reduced using backsub later)
        for(int j=i+1;j<KERNEL_DEPTH;j++){
            WAIT
            //mulltiplier for current row = cii/cji, then we subtract the cii row
            //to make current xi coefficient 0 and move on
            double magnitude = k->A[i][i]/k->A[j][i];
            WAIT
            k->b[j]*=magnitude;
            WAIT
            k->b[j]-=k->b[i];
            
            for(int c=0;c<KERNEL_DEPTH;c++){
                WAIT
                k->A[j][c]*=magnitude;
                WAIT
                k->A[j][c]-=k->A[i][c];
            }
        }

    }

    WAIT
    //do back substitution
    for(int i=KERNEL_DEPTH-1;i>0;i--){
        WAIT
        int sigma=0;
        for(int j=i+1;j<KERNEL_DEPTH;j++){
            WAIT
            sigma+=k->A[i][j]*x[j];
        }
        x[i]-=sigma;
        WAIT
        x[i]+=k->b[i]/k->A[i][i];  //add sol. vector itself div by coeff of xi
    }

    //return popcorn score 
    //return sum(x);
    return sum(k->b);  //placeholder for solution vector sum until I find the fix for incorrect matrix ops.
}

int max(int a, int b){
    return a>b?a:b;
}

void delay_ms(int ms){
    struct timespec t1;
    t1.tv_sec=ms/1000;   //carry over to seconds if >1000
    t1.tv_nsec=(ms%1000)*MICRO;
    nanosleep(&t1,NULL);
}

//abs val function for doubles just in case
double abs_dbl(double val){
    if(val>=0) {return val;}
    else{return val*-1;}
}

int sum(double* vals){
    int tot = 0;
    int l = sizeof(vals)/sizeof(double);
    for(int i=0; i<l; i++){
        tot+=vals[i];
    }
    return tot;
}

//create each popcorn kernel with seeded initial conditions.
popkernel createkernel(){
    popkernel pk;
    //generate solution vector x first and then construct a matrix for it
    //double *vars = malloc(sizeof(double)*KERNEL_DEPTH);
    double vars[KERNEL_DEPTH];
    
    for(int i=0; i<KERNEL_DEPTH; i++){
        //solution is win cond. so x1...xk are constrained by partialposmax
        vars[i]=rand()%PARTIAL_POS_SCORE_MAX;
    }

    //matrix of coeffs init and populate
    double coeffs[KERNEL_DEPTH][KERNEL_DEPTH];
    
    for(int i=0; i<KERNEL_DEPTH; i++){
        for(int j=0; j<KERNEL_DEPTH; j++){
            coeffs[i][j]=rand()%COEFF_MAX;
        }
    }

    //generate solution vector by using k eqs.
    //a11*x1 + ... + a1k*xk = b1
    // ...     ...    ...   = ...
    //ak1*x1 + ... + akk*xk = bk

    //store A and b, disregard x
    double sols[KERNEL_DEPTH];
    double b;
    for(int i=0; i<KERNEL_DEPTH; i++){
        b=0;
        for(int j=0; j<KERNEL_DEPTH; j++){
            //sum up all the pieces for one eq
            b+=coeffs[i][j]*vars[j];
        }
        //limit each element of b to being below partial neg score or shit will blow up fr
        //Im doing whole row/2 each iter of this loop
        while(b>PARTIAL_NEG_SCORE_MAX){
            for(int j=0; j<KERNEL_DEPTH; j++){
                coeffs[i][j]/=2;
            }
            b/=2;
        }
        sols[i]=b;
    }

    memcpy(pk.A, coeffs, sizeof(pk.A));
    memcpy(pk.b, sols, sizeof(pk.b));
    return pk;
}

//each kernel is a nxn matrix to be solved against an n dimensional vector
popkernel* generatebag(int size){

    if(size==-1){perror("Invalid bag size specified."); exit(-1);}
    popkernel *bag = malloc(sizeof(popkernel)*size);

    for(int i=0; i<size; i++){
        popkernel popcorn_inst = createkernel();
        memcpy(bag[i].A,popcorn_inst.A,sizeof(bag[i].A));
        memcpy(bag[i].b,popcorn_inst.b,sizeof(bag[i].b));
    }

    return bag;
}

//debug (shits out a matrix)
void getkernelcontents(popkernel* korn){
    printf("Kerneltest:\n");
    for(int i = 0; i<KERNEL_DEPTH; i++){
        printf("| ");
        for(int j=0; j<KERNEL_DEPTH; j++){
            printf("%lf ",korn->A[i][j]);
        }
        printf("| %lf\n",korn->b[i]);
    }
}

//timer thread function
void* microwave_wait(void* arg){
    int *seconds = (int*) arg;
    for(int i=0;i<*seconds;i++){
        sleep(1);
        TIMEREMAINING-=1;   //safely modifiable beacuse only one thread runs microwave wait
    }
    
    printf("DING!\n");
}

//kernel pop thread cancel
void* popfail(void* arg){
    popkernel* pk = (popkernel*) arg;
    int tot=-1*sum(pk->b);
    atomic_fetch_add(&SCORE,tot);
}

//kernel burn thread function
void* burner(void* arg){
    //a kernel can take anywhere between BURN_MIN to BURN_MAX seconds to burn after being popped
    int s = max(rand()%(BURN_MAX*1000),BURN_MIN*1000);
    delay_ms(s);

    printf("BURNED!\n");
    fflush(stdout);
    atomic_fetch_add(&SCORE,-200);
}

//kernel pop thread function
void* popper(void* arg){
    pthread_t burnthread;
    popkernel* pk = (popkernel*) arg;
    //cleanup = pop failed output negative score
    pthread_cleanup_push(popfail,pk);
    int s = max(rand()%10000,3000);
    //initial random delay
    delay_ms(s);
    int runningtot = partialpivot(pk);
    atomic_fetch_add(&SCORE,runningtot);
    pthread_cleanup_pop(0);
    printf("Pop!\n");
    fflush(stdout);
    //time 2 burn :)
    pthread_create(&burnthread, NULL, burner, NULL);
    pthread_detach(burnthread);
}


int main(int argc, char* argv[]){

    if(argc!=3){
        printf("Correct Argument format: ./popcorn <microwave time (secs)> <bagsize: O|XS|S|M|L|XL|X(2...4)L>\n"
            "Press 'h' for documentation, any other key to exit: ");
        char c;
        scanf("%c",&c);
        if(c=='h'){
            printf("Welcome to Pop The Kernels! A multi-threaded minigame where you control the microwave timer.\n"
            "The game starts with a bag of popcorn kernels being generated according to your specified bag size.\n"
            "Inputs: <Microwave time (in seconds)> <Bag Size (Specify one of these flags): O|XS|S|M|L|XL|X(2...4)L>\n\n"
            " O: One Kernel Only.\n XS: Extra Small - 8 Kernels.\n S: Small - 16 Kernels.\n"
            " M: Medium - 32 Kernels.\n L: Large - 64 Kernels. \n XL: Extra Large - 128 Kernels... and so on.\n\n"
            "Example Input (Command Line): ./popkernels 120 X3L - Runs for 120 seconds with a XXXL bag (512 kernels!).\n"
            "(*Note that custom bag sizes are not supported.)\n\n"
            "So now then... How does this work?\n "
            "All popcorn kernels use a fixed value N and store two arrays based on this value.\n"
            " 1- An NxN matrix (A)\n 2- An N-D vector (b)\n"
            "Then a thread performs calculations to reverse engineer a solution to this matrix vector pair \n"
            "and all the elements of the solution vector are summed together to generate a score, and once this is\n"
            " done the thread returns a pop sound.\n"
            "When the timer runs out, all the unpopped kernels will negatively affect your score using the unsolved vector b.\n"
            "So.... just spam the biggest value for the microwave and call it a day perhaps...\n"
            "(Do note that burnt popcorn penalize you even more than what you earn per piece :D)\n"
            "Now that you know the rules, go ahead and try it out!\n"
            "./popkernels <time> <bagsize>\n"
            );
            exit(0);
        }
        else{
            exit(-1);
        }
    }

    struct sigaction sa = {.sa_handler=end_game};
    sigaction(2,&sa,NULL); 

    int t=atoi(argv[1]);
    char *s=argv[2];
    int s_int = convertmap(s);
    //printf("#kernels = %d\n",convertmap(size));
    //printf("%ld\n",time(NULL));
    srand(time(NULL));

    //timing test for kernel gen
    struct timespec t1,t2;

    printf("Generating %d Kernels:\n", s_int);
    clock_gettime(CLOCK_MONOTONIC,&t1);

    //generate <bagsize> kernels
    popkernel* popbag = generatebag(s_int);

    //single test
    //popkernel korn = createkernel();

    clock_gettime(CLOCK_MONOTONIC,&t2);
    long delta = t2.tv_nsec-t1.tv_nsec;
    printf("Kernels Generated! Took %ld ns\n",delta);

    //kerneltest: dump out kernel matrix | solution pairs
    getkernelcontents(&popbag[0]);

    //start timer now and sum all the scores upon finishing.
    TIMEREMAINING = t;
    pthread_t microtimer;
    pthread_create(&microtimer,NULL,microwave_wait,&t);

    //start individual kernel threads
    pthread_t popthreads[s_int]; pthread_t *cancelled;
    for(int i=0; i<s_int; i++){
        pthread_create(&popthreads[i],NULL,popper,&popbag[i]);
    }

    //wait for timer to stop.
    pthread_join(microtimer,NULL);
    
    //cancel all running threads
    for(int i=0; i<s_int; i++){
        pthread_cancel(popthreads[i]);
    }
    
    sleep(1);

    printf("Your final score is: %d\n",SCORE);

    free(popbag);

    return 0;
}