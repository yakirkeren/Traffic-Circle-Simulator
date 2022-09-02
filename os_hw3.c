// Saar Ben Yochana - 313234155 |
// Eyal Zvi         - 319067732 |
// -----------------------------|

//// INCLUDES ////
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

//// DEFINES ////
#define N 5                                                         // Square dimensions
#define FIN_PROB 0.1                                                // Probability of sinks
#define MIN_INTER_ARRIVAL_IN_NS 8000000                             // Minimum time of car creation
#define MAX_INTER_ARRIVAL_IN_NS 9000000                             // Maximum time of car creation
#define INTER_MOVES_IN_NS 100000                                    // Movement delay
#define SIM_TIME 2                                                  // Simulation time [seconds]

#define SIM_TIME_IN_NS SIM_TIME*1000000000                          // Simulation time [nanoseconds]
#define SNAPSHOTS 10                                                // Number of snapshots
#define MAX_GEN_CAR (SIM_TIME_IN_NS/MIN_INTER_ARRIVAL_IN_NS)        // Maximal number of cars from any generator
#define MAX_TOTAL_CARS 4*MAX_GEN_CAR                                // Maximal number of total cars


//// Car Struct ////
typedef struct Car {
    int CarID;                                 // Unique identity number
    int Location;                              // Location index of the car [in the square / entry point]
    int isMoved;                               // Boolean flag which indicates if sink is enabled
} Car;

int   getCarID();                              // Encapsulate the id mutex counter increment
void  exit_simulation();
void  init_simulation();                       // Initialize simulation's board, mutexes and generators threads
void* print_snapshot();                        // Printing the square board <SNAPSHOTS> times during the simulation
void* generator(void* gen_num);                // Generator management thread function
void* car_progress(void* car);                 // Car management thread function


//// Global Variables ////
int sim_over = 0;                              // Global boolean flag which indicates end of simulation for all threads
int genIndex[4] = {0,1,2,3};                   // Addressing generators indices which should be passed as arguments

//// Shared Resources ////
int counter = 0;                               // Global counter which is used for generating unique carID's
int square[4*(N-1)] = {0};                     // Implement the board of the simulation with this array

//// Mutex's & Threads ////
pthread_mutex_t counter_mutex;                 // Counter mutex
pthread_mutex_t square_mutex[4*(N-1)];         // Array of board panels mutex
pthread_t carThreads[MAX_TOTAL_CARS] = {0};    // Array of cars threads
pthread_t genThreads[4];                       // Array of generator threads
pthread_t printingThread;                      // Printing simulation snapshots thread

int main() {
    /* -----------------------------------------------------------------------------------------------------------------
     * Description: This program simulates traffic circle in order to predict the congestion in a junction.
     * Output:      Determined number of uniform snapshots of the junction during the simulate
     * -----------------------------------------------------------------------------------------------------------------
    */

    init_simulation();
    sleep(SIM_TIME);
    sim_over = 1;
    exit_simulation();
    return EXIT_SUCCESS;
}

void init_simulation()
{
    /* -----------------------------------------------------------------------------------------------------------------
     * Description: Initialize simulation's board, mutexes and generators threads
     * Input:       None
     * Return:      None
     * -----------------------------------------------------------------------------------------------------------------
    */

    // Counter mutex init:
    if (pthread_mutex_init(&counter_mutex,NULL)) {
        perror("error occurred in counter mutex initialize");
        exit_simulation();
        exit(EXIT_FAILURE);
    }
    // Square mutex's initialization:
    for (int i=0;i<4*(N-1);i++)
    {
        if (pthread_mutex_init(&square_mutex[i],NULL)) {
            perror("error occurred in square mutex initialize");
            exit_simulation();
            exit(EXIT_FAILURE);
        }
    }
    // Create the printing thread:
    if (pthread_create(&printingThread,NULL,print_snapshot,NULL))
    {
        perror("Printing thread creation failed.");
        exit_simulation();
        exit(EXIT_FAILURE);
    }
    for (int i=0; i<4;i++) {
        // Create 4 generators threads and passing the index accordingly:
        if(pthread_create(&genThreads[i],NULL,generator, (void *)(&genIndex[i])))
        {
            perror("Thread creation (generator thread) failed.");
            exit_simulation();
            exit(EXIT_FAILURE);
        }
    }
}

void* generator(void* gen_num) {
    /* -----------------------------------------------------------------------------------------------------------------
     * Description: Generator management thread function - generate new car in a given time interval
     * Input:       gen_num - memory address pointer to the generator index in the global scope
     * Return:      None
     * -----------------------------------------------------------------------------------------------------------------
    */

    // Generator variables declaration
    int genID = *(int *) gen_num;
    double random_wait;
    int index = genID*(N-1);
    int id_validation;
    Car *New;

    while (!sim_over) {
        // This section will be executed only during simulation time

        // Generate random time from pre-defined range and wait till it over
        random_wait = rand()%(MAX_INTER_ARRIVAL_IN_NS-MIN_INTER_ARRIVAL_IN_NS) + MIN_INTER_ARRIVAL_IN_NS;
        usleep((useconds_t)(random_wait/(double)1000));
        // Allocate new car object & initialize it fields.
        New = (Car *)malloc(sizeof(Car));
        if (!New) {
            perror("Memory allocation failed");
            exit_simulation();
            exit(EXIT_FAILURE);
        }
        // Checking return value of getCarID to decide if it is required to free car object memory allocation
        id_validation = getCarID();

        switch(id_validation) {
            case -1: {
                // Unlocking mutex failed is interpreted as return code = '-1'
                free(New);
                perror("Locking counter mutex failed.");
                exit_simulation();
                exit(EXIT_FAILURE);
            }
            case -2: {
                // Unlocking mutex failed is interpreted as return code = '-2'
                free(New);
                perror("Unlocking counter mutex failed.");
                exit_simulation();
                exit(EXIT_FAILURE);
            }
            default: {
                // Otherwise, the id is a positive unique number therefore it is valid to continue
                New->CarID=id_validation;
                break;
            }
        }
        // Init car object other fields
        New->Location=index;
        New->isMoved=0;

        // Create new thread and connect it to the allocated car, in case of error -> memory allocation will free
        if (pthread_create((carThreads+New->CarID), NULL,car_progress, New))
        {
            perror("New car thread creation failed.");
            free(New);
            exit_simulation();
            exit(EXIT_FAILURE);
        }
    }
    // Performing thread exit once simulation over
    pthread_exit(NULL);
}

int getCarID() {
    /* -----------------------------------------------------------------------------------------------------------------
     * Description: Verify only one thread increment the counter and therefore each car gets a unique id number.
     * Input:       None
     * Return:      newID - sampled counter value when thread successfully lock the mutex and increment the counter
     * -----------------------------------------------------------------------------------------------------------------
    */

    int newID;
    if (pthread_mutex_lock(&counter_mutex)) {
        return -1;
    }
    // Protecting the counter increment and return value:
    counter++;
    newID=counter;
    // --------------------------------------------------
    if (pthread_mutex_unlock(&counter_mutex)) {
        return -2;
    }
    return newID;
}

void* car_progress(void* car) {
    /* -----------------------------------------------------------------------------------------------------------------
     * Description: Car management thread function - generate new car in a given time interval
     * Input:       None
     * Return:      newID - sampled counter value while succeed to lock and increment
     * -----------------------------------------------------------------------------------------------------------------
    */

    // Car's movement variables declaration
    Car *currCar = (Car *) car;
    int CurrIndex = currCar->Location;
    int PrevIndex, NextIndex;
    int inSquare = 0;

    // Implementing a circular movement
    if (!CurrIndex)
        // If current location is index 0 then the previous one is 4*(N-1)-1;
        PrevIndex = 4*(N-1)-1;
    else
        // For each other location previous location will be at index current index minus one
        PrevIndex = CurrIndex - 1;

    while (!sim_over) {
        // This section will be executed only during simulation time
        if (!inSquare) {
            // Trying to capture both mutexes of previous and current panels in the board
            if (!pthread_mutex_trylock(&square_mutex[PrevIndex])) {
                if (!pthread_mutex_trylock(&square_mutex[CurrIndex])) {
                    // If both panels are locked successfully then it is valid to insert the car to the board
                    square[CurrIndex] = currCar->CarID;
                    inSquare = 1;
                    if (pthread_mutex_unlock(&square_mutex[PrevIndex])) {
                        perror("Unlocking square mutex failed.");
                        exit_simulation();
                        exit(EXIT_FAILURE);
                    }
                    continue;
                }
                if (pthread_mutex_unlock(&square_mutex[PrevIndex])) {
                    perror("Unlocking square mutex failed.");
                    exit_simulation();
                    exit(EXIT_FAILURE);
                }
            }
            // Previous panel locking try has failed -> repeat and try to capture it again
            continue;
        }
        // This section will be executed only when the car successfully entered the board

        // Update movement variables accordingly
        CurrIndex = currCar->Location;
        NextIndex = (CurrIndex+1)%(4*(N-1));
        // Trying to capture the mutex of the next panel in order to let the car move forward
        if (!pthread_mutex_trylock(&square_mutex[NextIndex])) {
            // If the next panel has locked successfully then car can move forward and release previous panel mutex
            square[NextIndex] = square[CurrIndex];
            square[CurrIndex] = 0;                               // Resets the current panel before movement
            currCar->Location = NextIndex;                       // Updates the current location inside the car struct
            currCar->isMoved = 1;                                // Sets the sink flag to enable removing
            if (pthread_mutex_unlock(&square_mutex[CurrIndex])) {
                perror("Unlocking mutex failed.");
                exit_simulation();
                exit(EXIT_FAILURE);
            }
        }
        // Verifying required delay between any two different car movements
        usleep((useconds_t)(INTER_MOVES_IN_NS/1000));
        // If car reached one of the sinks (and already moved) then it will be removed in probability of FIN_PROB
        if ((currCar->Location)%(N-1)==0 && currCar->isMoved==1) {
            int random = rand()%101;
            if (random < FIN_PROB*100) {
                break;
            }
        }
        // Verifying we try and delete a car once per sink
        currCar->isMoved=0;
    }
    // Once simulation is over, do panel mutex unlock (if required), free car allocated memory and exit the thread
    square[currCar->Location]=0;
    if (inSquare == 1) {
        if (pthread_mutex_unlock(&square_mutex[currCar->Location])) {
            perror("Unlocking mutex failed.");
            exit_simulation();
            exit(EXIT_FAILURE);
        }
    }
    free(currCar);
    pthread_exit(NULL);
}

void* print_snapshot() {
    /* -----------------------------------------------------------------------------------------------------------------
     * Description: Print board state in pre-defined number of snapshots during the simulation time
     * Input:       None
     * Output:      Board state according to the next notation: '*' - represents captured panel (by a car)
     *                                                          '@' - represents middle panel
     *                                                          ' ' - represents blank panel
     * -----------------------------------------------------------------------------------------------------------------
    */

    while (!sim_over) {
        for (int i=0;i<N;i++) {
            for (int j=0;j<N;j++) {
                if (i == 0) {
                    if (square[(N-1)-j] > 0)
                        printf("*");
                    else
                        printf(" ");
                }
                if (i == N-1) {
                    if (square[2*(N-1)+j] > 0)
                        printf("*");
                    else
                        printf(" ");
                }
                if (i!=0 && i!=N-1) {
                    if (j == 0) {
                        if (square[N-1+i] > 0)
                            printf("*");
                        else
                            printf(" ");
                    } else if (j==N-1) {
                        if (square[4*(N-1)-i] > 0)
                            printf("*");
                        else
                            printf(" ");
                    } else
                        printf("@");
                }
            }
            printf("\n");
        }
        printf("\n");
        // Verifying there will be exactly [SNAPSHOTS] snapshots during the simulation
        usleep((useconds_t)(((double)SIM_TIME/(double)(SNAPSHOTS))*1000000));
    }
    // Do thread exit once simulation over
    pthread_exit(NULL);
}


void exit_simulation() {
    /* -----------------------------------------------------------------------------------------------------------------
     * Description: Waiting for all threads to do complete exit and free all allocated memory and resources.
     * Input:       None
     * Return:      None
     * -----------------------------------------------------------------------------------------------------------------
    */

    // Forcing main thread to wait for complete exit of printing thread and all cars & generators threads
    pthread_join(printingThread,NULL);
    for (int i=0;i<MAX_TOTAL_CARS;i++) {
        pthread_join(carThreads[i],NULL);
    }
    for (int i=0;i<4;i++) {
        pthread_join(genThreads[i],NULL);
    }
    // Destroy all initialized mutex's.
    if (pthread_mutex_destroy(&counter_mutex)) {
        perror("Destroying counter mutex failed");
        exit(EXIT_FAILURE);
    }
    for (int i=0;i<4*(N-1);i++) {
        if (pthread_mutex_destroy(&square_mutex[i])) {
            perror("Destroying square mutex failed");
            exit(EXIT_FAILURE);
        }
    }
}