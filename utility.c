/******************************************************************************
 * Nazev projektu: IOS - projekt 2 (synchronizace) Skibus
 * Soubor: utility.c
 * Datum: 22.04.2024
 * Autor: Marek Pazúr
 * 
 * Popis: Pomocne funkce pro validaci vstupu, alokaci/dealokaci a inicializaci
 * 
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>

#include "utility.h"
#include "structs.h"

int convert_args(char **argv, int* buffer, int size) {
    char *endptr;

    for (int i = 0; i < size; ++i)
    {
        buffer[i] = strtol(argv[i+1], &endptr, 10);

        if (*endptr != '\0'){
            fprintf(stderr,"Cannot parse argument[%d]: %s\n", i, argv[i+1]);

            if(errno != 0)
            	perror(NULL);

            return CONVERSION_ERROR;
            }
    }

    return EXIT_SUCCESS;
}

void init_settings(settings_t *s,int *buffer) {
	//L = [0], Z = [1], K = [2], TL = [3], TB = [4]
	s->skier_count = buffer[0];
	s->stop_count = buffer[1];
	s->capacity = buffer[2];
	s->TL = buffer[3];
	s->TB = buffer[4];
}

int validate_input(settings_t *s) {

    /*Validate input arguments*/
    if(!(s->skier_count > 0 && s->skier_count < 20000)) {
        fprintf(stderr,"Neplatny pocet lyzaru: %d\n", s->skier_count);
        return INPUT_ERROR;
    }


    if(!(s->stop_count > 0 && s->stop_count <= 10)) {
        fprintf(stderr,"Neplatny pocet zastavek: %d\n", s->stop_count);
        return INPUT_ERROR;
    }

    if(!(s->capacity >= 10 && s->capacity <= 100)) {
        fprintf(stderr,"Neplatna kapacita: %d\n", s->capacity);
        return INPUT_ERROR;
    }


    if(!(s->TL >= 0 && s->TL <= 10000)) {
        fprintf(stderr,"Neplatny maximalni cas: %d\n", s->TL);
        return INPUT_ERROR;
    }

    if(!(s->TB >= 0 && s->TB <= 1000)) {
        fprintf(stderr,"Neplatna maximalni doba: %d\n", s->TB);
        return INPUT_ERROR;
    }

    return EXIT_SUCCESS;    
}

int allocate_shared_data(pcshd_t** sdata) {

    /*Mapped memory allocation*/
	if((*sdata = mmap(NULL, sizeof(pcshd_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		perror("Unable to map memory\n");
		return MEM_ALLOC_ERROR;
	}

	return EXIT_SUCCESS;
}

int init_shared_data(pcshd_t* sdata, settings_t *s) {

    /*Set memory to 0*/
    memset(sdata, 0, sizeof(pcshd_t));

    /*Open output file*/
	sdata->file = fopen(OUTPUT_FILE, "w+");

    if(sdata->file == FILE_OPEN_ERROR) {
        fprintf(stderr,"Unable to open file \'%s\'\n%s\n",OUTPUT_FILE, strerror(errno));
        return EXIT_FAILURE;
    }

    /*Get settings*/
    sdata->s = *s;

    /*Init counter variables*/
    sdata->skiers_remaining = sdata->s.skier_count;

    /*Init semaphores*/
    sem_init(&sdata->mutex,1,1);
    sem_init(&sdata->bus,1,0);

    for (int i = 0; i < sdata->s.stop_count; ++i)
        sem_init(&sdata->stop[i],1,0);

    sem_init(&sdata->exit,1,0);

	return EXIT_SUCCESS;
}

int free_shared_data(pcshd_t** sdata) {

    /*Closing output file descriptor*/
	fclose((*sdata)->file);

    /*Destroying semaphores*/
    sem_destroy(&(*sdata)->mutex);
    sem_destroy(&(*sdata)->bus);

    for (int i = 0; i < (*sdata)->s.stop_count; ++i)
        sem_destroy((*sdata)->stop);

    sem_destroy(&(*sdata)->exit);

    /*Unmapping allocated memory*/
	if(munmap(*sdata, sizeof(pcshd_t)) == UNMAP_FAILED) {
		perror("Memory unmap has failed\n");
		return MEM_FREE_ERROR; 
	}

	*sdata = NULL;

	return EXIT_SUCCESS;
}