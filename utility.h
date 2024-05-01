/*Author: Marek Pazúr*/
#ifndef UTILITY_H
#define UTILITY_H

#include "structs.h"

#define ARG_COUNT 0x00000006
#define OUTPUT_FILE "proj2.out" 

/*Error flags*/
#define CONVERSION_ERROR 0x00000001
#define INPUT_ERROR 0x00000002
#define MEM_ALLOC_ERROR 0x00000004
#define MEM_FREE_ERROR 0x00000004
#define FILE_OPEN_ERROR NULL
#define UNMAP_FAILED -1

int convert_args(char **argv, int* buffer, int size);
int validate_input(settings_t *s);
void init_settings(settings_t *s, int *buffer);
int allocate_shared_data(pcshd_t** sdata);
int init_shared_data(pcshd_t* sdata,settings_t *s);
int free_shared_data(pcshd_t** sdata);

#endif