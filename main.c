/****************************************************************
 * Nazev projektu: IOS - projekt 2 (synchronizace) Skibus
 * Soubor: main.c
 * Datum: 22.04.2024
 * Autor: Marek Pazúr
 * 
 * Popis: Synchronizace procesu pomoci semaforu a sdilene pameti
 * 
 ****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

/*Helper functions*/
#include "utility.h"
#include "structs.h"

/*Helper macros for better accessibility*/
#define ACTION ++sdata->action
#define FILE_P sdata->file
#define SKIERS sdata->s.skier_count
#define CAPACITY sdata->s.capacity

#define FINISH -1

int main(int argc, char **argv) {
    /*MAIN PROCESS*/

    /*Number of args*/
    if (argc != ARG_COUNT) {
        fprintf(stderr,"Invalid number of arguments: %d\n", argc);
        return EXIT_FAILURE;
    }

    /*Parse arguments*/
    int input_buffer[ARG_COUNT-1]; 
    if(convert_args(argv, input_buffer, ARG_COUNT-1) == CONVERSION_ERROR) {
        fprintf(stderr,"Conversion error\n");
        return EXIT_FAILURE;
    }

    /*Initialize settings of the program*/
    settings_t s;
    init_settings(&s, input_buffer);

    /*Validate input arguments*/
    if(validate_input(&s) == INPUT_ERROR) {
        fprintf(stderr, "Invalid input data\n");
        return EXIT_FAILURE;
    }

    /*Allocate and initialize process-shared-data */
    pcshd_t *sdata = NULL;

    if(allocate_shared_data(&sdata) == MEM_ALLOC_ERROR)
        return EXIT_FAILURE;

    if(init_shared_data(sdata,&s) == 1) {
        fprintf(stderr, "Unable to initialize shared data structure\n");
        free_shared_data(&sdata);
        return EXIT_FAILURE;
    }

/*******************************************************************/
/*---------------------------MAIN TASK-----------------------------*/
/*******************************************************************/
    /*Processes: MAIN + BUS + i*SKIERS*/
    pid_t skier[SKIERS];
    pid_t bus;

    /*FORK BUS*/
    bus = fork();

    /*BUS PROCESS*/
    if(bus == 0) {
        /*Random generated wait time*/
        srand(time(NULL) ^ getpid());

        /*BUS START*/
        sem_wait(&sdata->mutex);
        fprintf(FILE_P,"%d: BUS: started\n", ACTION);
        fflush(FILE_P);
        sem_post(&sdata->mutex);

        /*(#) Set Stop id to 1*/
        int idS = 1;

        /*Loop until every skier is transported to final destination*/
        while(idS != FINISH)
        {
            /*Time spent traveling between skibus stops*/
            usleep(rand() % sdata->s.TB + 1);
            
            /*(*)Arrival of the skibus*/
            sem_wait(&sdata->mutex);
            fprintf(FILE_P,"%d: BUS: arrived to %d\n", ACTION, idS);
            fflush(FILE_P);
            sem_post(&sdata->mutex);

            /*Skip current stop if no one is waiting*/
            if(sdata->skiers_onstop[idS-1] > 0) {
            sem_post(&sdata->stop[idS-1]);
            sem_wait(&sdata->bus);
            }

            /*Departure of the skibus*/
            sem_wait(&sdata->mutex);            
            fprintf(FILE_P,"%d: BUS: leaving %d\n", ACTION, idS);
            fflush(FILE_P);
            sem_post(&sdata->mutex);

            /*Last stop passed*/
            if(idS == sdata->s.stop_count) {
                /*Arrival to final destination*/
                sem_wait(&sdata->mutex);
                fprintf(FILE_P,"%d: BUS: arrived to final\n", ACTION);
                fflush(FILE_P);
                sem_post(&sdata->mutex);

                /*If there are skiers onboard signal them to deboard*/
                if(sdata->skiers_onboard > 0) {
                    sem_post(&sdata->exit);
                    sem_wait(&sdata->bus);
                }

                /*Leave final destination*/
                sem_wait(&sdata->mutex);
                fprintf(FILE_P,"%d: BUS: leaving final\n", ACTION);
                fflush(FILE_P);
                sem_post(&sdata->mutex);

                /*If every skier has been transported break, else go back to the first stop*/
                if(sdata->skiers_remaining < 1) {
                    idS = FINISH;
                }
                else {
                    idS = 1;
                    //sem_wait(&sdata->exit);
                }
            } else ++idS; //going to next stop
        }

        /*Bus has finished its job*/
        sem_wait(&sdata->mutex);
        fprintf(FILE_P,"%d: BUS: finish\n", ACTION);
        fflush(FILE_P);
        sem_post(&sdata->mutex);

        exit(0);
    } else if(bus == -1) {
        perror("Process BUS fork error\n");

        free_shared_data(&sdata);

        return EXIT_FAILURE;
    }
    
    /*MAIN PROCESS - FORKING SKIERS*/
    for (int i = 0; i < SKIERS; ++i) {
        /*FORK SKIER[i]*/
        skier[i] = fork();

        /*SKIER PROCESS*/
        if(skier[i] == 0) {
            /*ASSIGN SKIBUS STOP*/
            srand(time(NULL) ^ getpid());
            int my_stop = rand () % sdata->s.stop_count + 1;

            /*SKIER START*/
            sem_wait(&sdata->mutex);
            fprintf(FILE_P,"%d: L %d: started\n", ACTION, i+1);
            fflush(FILE_P);
            sem_post(&sdata->mutex);

            /*BREAKFAST*/
            if(sdata->s.TL > 0)
                usleep(sdata->s.TL);
                
            /*Arrival to skibus stop*/
            sem_wait(&sdata->mutex);
                fprintf(FILE_P,"%d: L %d: arrived to %d\n",ACTION,i+1,my_stop);
                fflush(FILE_P);
                ++sdata->skiers_onstop[my_stop-1]; //Increment the counter of skiers waiting at my stop
            sem_post(&sdata->mutex);

            /*Board on bus if its at my stop*/;
            sem_wait(&sdata->stop[my_stop-1]);

            sem_wait(&sdata->mutex);
                fprintf(FILE_P,"%d: L %d: boarding\n",ACTION,i+1);
                fflush(FILE_P);
                ++sdata->skiers_onboard; //Skier has boarded the bus
                --sdata->skiers_onstop[my_stop-1]; //Skier has left the stop
            sem_post(&sdata->mutex);

            /*IF there is enough capacity onboard AND someone is still waiting at the stop, let them ABOARD*/
            /*ELSE give the bus signal to depart*/
            if(sdata->skiers_onboard < CAPACITY && sdata->skiers_onstop[my_stop-1] > 0)
                sem_post(&sdata->stop[my_stop-1]);
            else sem_post(&sdata->bus);

            /*Exit the bus if at final stop*/
            sem_wait(&sdata->exit);

            sem_wait(&sdata->mutex);
                fprintf(FILE_P,"%d: L %d: going to ski\n",ACTION,i+1);
                fflush(FILE_P);
                --sdata->skiers_remaining; //Skiers that are still waiting to be transported
                --sdata->skiers_onboard;
            if(sdata->skiers_onboard == 0) //If everyone left the bus, signal the bus to depart
                sem_post(&sdata->bus);
            sem_post(&sdata->mutex);

            if(sdata->skiers_onboard > 0) //If there is still someone in the bus, signal them to deboard
            sem_post(&sdata->exit);

            exit(0);

        } else if(skier[i] == -1) {
            perror("Process SKIER fork error\n");

            for(int j = 0; j < i; ++j)
                kill(skier[j], SIGKILL);

            kill(bus, SIGKILL);

            free_shared_data(&sdata);

            return EXIT_FAILURE;
        } 
    }

    /*******************************************************************/
    /*MAIN PROCESS - end*/

    int status;

    /*Wait for SKIER processes to end*/
    for (int j = 0; j < SKIERS; ++j)
    waitpid(skier[j], &status, 0);
    
    /*Wait for BUS process to end*/
    waitpid(bus, &status, 0);

    /*Free memory*/
    if(free_shared_data(&sdata) == MEM_FREE_ERROR)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}