/****************************************************************
 * Nazev projektu: IOS - projekt 2 (synchronizace) Skibus
 * Soubor: structs.h
 * Datum: 22.04.2024
 * Autor: Marek Pazúr
 * 
 * Popis: Pomocne struktury pro sdilena data
 * 
 ****************************************************************/
#ifndef STRUCTS_H
#define STRUCTS_H

/*Structure that holds input parameters*/
typedef struct settings {
	int skier_count;
	int stop_count;
	int capacity;
	int TL;
	int TB;
} settings_t;

/*Structure used as a shared memory among forked processes*/
typedef struct process_shared_data {
	/*Program settings & output file descriptor*/
	settings_t s;
	FILE* file;

	/*Shared variables*/
	int action; //Counter of actions
	int skiers_remaining; //Counter of skiers waiting to be transported
	int skiers_onboard; //Counter of skiers aboard the bus
	int skiers_onstop[10]; //Counter of skiers waiting at individual stops

	/*Semaphores*/
	sem_t mutex; //Critical section
	sem_t bus; //Signal the bus
	sem_t stop[10]; //Signal skiers to board
	sem_t exit; //Signal skiers to leave the bus
} pcshd_t;

#endif