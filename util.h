#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <string.h>
#include <stdbool.h>

#define SUPERPAGE 1024*1024*1024
#define POOLSIZE 1024
#define ROUNDS 1000
#define GROUPINGTHRESHOLD	10
#define THRESHOLDROUNDS		10
#define ADDRESS_BITS		30
#define THIRTY_BISTS_MASK	0x3fffffff
#define	FN 					4

struct address_in_bank
{
	char* address;
	struct address_in_bank* next_address;
};

struct functions
{
	uint32_t function;
	struct functions* next_function;
};

char* allocate_superpage();
int compare( const void* a, const void* b);
int find_median(int times[]);
int time_access(char const *addr1, char const *addr2);
char** initialise_pool(char* buffer);
int un_coloured(int colours[]);
void revert_colour(int* colours, int current_colour);
void bank_conflict(char* buffer, char** pool);
int* find_banks(char* buffer, char** pool, int threshold);
int number_of_banks(int colours[]);
int find_threshold(char* buffer);
struct address_in_bank* cluster_addresses(char** pool, int colours[], int cluster_colour);
uint32_t find_significant_bits(struct address_in_bank* cluster_head, int threshold);
char* change_bit(char* addr, uint64_t bit);
uint32_t* find_functions(char** pool, int colours[], int total_banks);
void find_candidate_masks(struct address_in_bank* cluster_head, uint32_t* functions);
void print_functions(uint32_t* functions);
uint32_t find_row_mask(struct address_in_bank* cluster_head, uint32_t* functions, int threshold);
int switch_bank(char** new_address, uint32_t* functions, int bit);


void print_cluster_addresses(struct address_in_bank* cluster_head);


void print_colours(int colours[POOLSIZE]);
void dump_pool(char** pool);

#endif // UTIL_H
