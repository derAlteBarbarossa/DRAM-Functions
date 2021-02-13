#include "util.h"

char* allocate_superpage()
{
	char* buffer = mmap(NULL, SUPERPAGE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS|MAP_HUGETLB, -1, 0);
	for (int i=0; i<SUPERPAGE; i++)
	    *(buffer+i) = 't';

	return buffer;
}

int compare( const void* a, const void* b)
{
	int int_a = * ( (int*) a );
	int int_b = * ( (int*) b );

	if ( int_a == int_b )
		return 0;
	else if ( int_a < int_b )
		return -1;
	else
		return 1;
}

int find_median(int times[ROUNDS])
{
	qsort(times, ROUNDS, sizeof(int), compare);
	return times[ROUNDS/2];
}

int time_access(char const *addr1, char const *addr2)
{
	int times[ROUNDS];
	int aux = 0;

	volatile char* a1 = (volatile char*) addr1;
	volatile char* a2 = (volatile char*) addr2;

	for (int i=0; i<ROUNDS; i++)
	{
		_mm_mfence();
		uint64_t start = __rdtscp(&aux);
		*a1;
		*a2;
		_mm_mfence();
		times[i] = (__rdtscp(&aux) - start);

		_mm_clflush((char*)a1);
		_mm_clflush((char*)a2);
		

	}

	int median = find_median(times);
	return median;
}



char** initialise_pool(char* buffer)
{
	uint64_t random;
	char** pool = (char**) malloc(sizeof(char*)*POOLSIZE);
	for (int i=0; i<POOLSIZE; i++)
	{
		random = rand()%(SUPERPAGE);
		*(pool + i) = buffer + (random);
	}
	return pool;
}

void dump_pool(char** pool)
{
	for (int i = 0; i < POOLSIZE; i++)
		printf("%p\n", *(pool + i));
}

int un_coloured(int colours[])
{
	int not_coloured = 0;

	for (int i=0; i<POOLSIZE; i++)
		if(colours[i] ==0)
			not_coloured++;

	return not_coloured;
}

void revert_colour(int* colours, int current_colour)
{
	for (int i = 0; i < POOLSIZE; i++)
	{
		if(colours[i] == current_colour)
			colours[i] = 0;
	}
}

void bank_conflict(char* buffer, char** pool)
{
	FILE* fptr;
	fptr = fopen("timing","w");


	int access_time;
	char* base = buffer;

	for (int i=0; i<POOLSIZE; i++)
	{
			access_time = time_access(base, *(pool+i));
			fprintf(fptr, "%p,%p,%d\n", base, *(pool+i), access_time);
	}
	fclose(fptr);
}

int* find_banks(char* buffer, char** pool, int threshold)
{
	int* colours = (int*) malloc(POOLSIZE*sizeof(int));
	memset(colours, 0, sizeof(int)*POOLSIZE);

	int access_time;
	int group_members;
	int current_colour = 1;


	for (int i=0; i<POOLSIZE; i++)
	{
		if (colours[i] == 0)
		{
			group_members = 0;
			colours[i] = current_colour;
			for (int j=i+1; j<POOLSIZE; j++)
			{
				if (colours[j] == 0)
				{
					access_time = time_access(*(pool+i), *(pool+j));
					if(access_time > threshold)
					{
						group_members++;
						colours[j] = current_colour;
					}
				}

				
			}
			if(group_members < GROUPINGTHRESHOLD)
			{
				revert_colour(colours, current_colour);
				current_colour--;
			}
			current_colour++;
		}
	}
	// printf("%d\n", --current_colour);
	return colours;
}
int number_of_banks(int colours[])
{
	int num_banks = 0;
	for (int i = 0; i < POOLSIZE; i++)
		if(colours[i] > num_banks)
			num_banks = colours[i];

	return num_banks;
}

int find_threshold(char* buffer)
{
	int times[POOLSIZE] = {0};
	char** pool = initialise_pool(buffer);

	char* base = buffer;

	double threshold = 0;
	for (int round = 0; round < THRESHOLDROUNDS; round++)
	{
		for (int i=0; i<POOLSIZE; i++)
		{
			times[i] = time_access(base, *(pool+i));
		}
		qsort(times, POOLSIZE, sizeof(int), compare);
		
		int threshold_pos = 0;
		int max_gap = 0;
		for (int i = 0; i < POOLSIZE-1; i++)
		{
			if(times[i+1]-times[i] > max_gap)
			{
				max_gap = times[i+1]-times[i];
				threshold_pos = i;
			}
		}
		threshold += (double)times[threshold_pos];
	}

	// printf("%d\t", (int) threshold/THRESHOLDROUNDS);
	return (int) threshold/THRESHOLDROUNDS;
}

struct address_in_bank* cluster_addresses(char** pool, int colours[], int cluster_colour)
{
	struct address_in_bank* head = (struct address_in_bank*) malloc(sizeof(struct address_in_bank));
	head->next_address = NULL;

	for (int i=0; i < POOLSIZE; i++)
	{
		if(colours[i] == cluster_colour)
		{
			head->address = *(pool + i);
			
			struct address_in_bank* new_head = (struct address_in_bank*) malloc(sizeof(struct address_in_bank));
			new_head->next_address = head;
			head = new_head;
		}
	}

	return head;
}

uint32_t find_significant_bits(struct address_in_bank* cluster_head, int threshold)
{
	uint64_t significant_bit_mask = 0;
	int access_time;
	struct address_in_bank* head = (struct address_in_bank*) malloc(sizeof(struct address_in_bank*));
	head = cluster_head->next_address;

	char* base = head->address;

	head = head->next_address;

	while(head != NULL)
	{
		access_time = time_access(base, head->address);

		if(access_time >= threshold)
		{
			for(uint64_t i=0; i < ADDRESS_BITS; i++)
			{
				char* probing_address = change_bit(head->address, i);
				access_time = time_access(base, probing_address);
				if(access_time < threshold)
					significant_bit_mask |= 1 << i;
			}
		}
		head = head->next_address;
	}
	// printf("%lx\n", significant_bit_mask);
	return significant_bit_mask;
}

char* change_bit(char* addr, uint64_t bit)
{
	uint64_t mask = 1 << bit;
	return (char*)((uint64_t)addr^mask);
}


uint32_t* find_functions(char** pool, int colours[POOLSIZE], int total_banks)
{
	uint32_t* functions, *final_functions;

	functions = (uint32_t*) malloc (sizeof(uint32_t) * FN);
	memset(functions, 0, sizeof(uint32_t) * FN);

	final_functions = (uint32_t*) malloc (sizeof(uint32_t) * FN);
	memset(final_functions, 0, sizeof(uint32_t) * FN);

	struct address_in_bank* cluster_head = (struct address_in_bank*) malloc(sizeof(struct address_in_bank*));

	
	for(int bank = 1; bank <= total_banks; bank++)
	{
		cluster_head = cluster_addresses(pool, colours, bank);
		if (cluster_head == NULL)
		{
			printf("Empty Cluster[%d]\n", bank);
			continue;
		}
		memset(functions, 0, sizeof(uint32_t) * FN);
		find_candidate_masks(cluster_head, functions);

		for (int i = 0; i < FN; i++)
			if(*(final_functions + i) == 0)
			{
				*(final_functions + i) = *(functions + i);
			}
	}
	
	

	return final_functions;
}

void print_functions(uint32_t* functions)
{
	int num_funcs = 0;
	for (int i = 0; i < FN; i++)
		if(*(functions + i) != 0)
			num_funcs++;

	printf("%d\n", num_funcs);

	for (int i = 0; i < FN; i++)
		if(*(functions + i) != 0)
			printf("%x\n", *(functions + i));
}

void find_candidate_masks(struct address_in_bank* cluster_head, uint32_t* functions)
{
	bool xor_head, xor_address_in_cluster, not_finished;
	uint32_t lo;
	uint32_t mask;
	int outlier;
	struct address_in_bank* head = (struct address_in_bank*) malloc (sizeof(struct address_in_bank*));

	int index = 0;

	for (int i = 0; i < ADDRESS_BITS; i++)
	{
		for (int j = i+1; j < ADDRESS_BITS; j++)
		{
			outlier = 0;
			head = cluster_head; 
			head = head->next_address;

			lo = ((uint64_t)head->address) & THIRTY_BISTS_MASK;
			xor_head = ((lo & (1 << i)) >> i) ^ ((lo & (1 << j)) >> j);

			while(head != NULL)
			{
				lo = ((uint64_t)head->address) & THIRTY_BISTS_MASK;
				xor_address_in_cluster = ((lo & (1 << i)) >> i) ^ ((lo & (1 << j)) >> j);

				if (xor_head != xor_address_in_cluster)
				{
					outlier++;
				}

				if (outlier == 5)
					break;

				head = head->next_address;
			}

			if(head == NULL)
			{
				*(functions + index) = (1 << i) | (1 << j);
				index++;
			}
		}
	}

	/*
	for (int i = 0; i < ADDRESS_BITS; i++)
	{
		for (int j = i+1; j < ADDRESS_BITS; j++)
		{
			for (int k = j+1; k < ADDRESS_BITS; k++)
			{
				outlier = 0;
				head = cluster_head; 
				head = head->next_address;

				lo = ((uint64_t)head->address) & THIRTY_BISTS_MASK;
				xor_head = ((lo & (1 << i)) >> i) ^ ((lo & (1 << j)) >> j) ^ ((lo & (1 << k)) >> k);

				while(head != NULL)
				{
					lo = ((uint64_t)head->address) & THIRTY_BISTS_MASK;
					xor_address_in_cluster = ((lo & (1 << i)) >> i) ^ ((lo & (1 << j)) >> j) ^ ((lo & (1 << k)) >> k);

					if (xor_head != xor_address_in_cluster)
					{
						outlier++;
					}

					if (outlier == 5)
						break;

					head = head->next_address;
				}

				if(head == NULL)
				{
					*(functions + index) = (1 << i) | (1 << j) | (1 << k);
					index++;
				}	
				
			}
			
		}
	}
	*/
	/*
	for (int i = 0; i < ADDRESS_BITS; i++)
	{
		for (int j = i+1; j < ADDRESS_BITS; j++)
		{
			for (int k = j+1; k < ADDRESS_BITS; k++)
			{
				for (int l = k + 1; l < ADDRESS_BITS; l++)
				{
					outlier = 0;
					head = cluster_head; 
					head = head->next_address;

					lo = ((uint64_t)head->address) & THIRTY_BISTS_MASK;
					xor_head = ((lo & (1 << i)) >> i) ^ ((lo & (1 << j)) >> j) ^ ((lo & (1 << k)) >> k) ^ ((lo & (1 << l)) >> l);

					while(head != NULL)
					{
						lo = ((uint64_t)head->address) & THIRTY_BISTS_MASK;
						xor_address_in_cluster = ((lo & (1 << i)) >> i) ^ ((lo & (1 << j)) >> j) ^ ((lo & (1 << k)) >> k) ^ ((lo & (1 << l)) >> l);

						if (xor_head != xor_address_in_cluster)
						{
							outlier++;
						}

						if (outlier == 5)
							break;

						head = head->next_address;
					}

					if(head == NULL)
					{
						*(functions + index) = (1 << i) | (1 << j) | (1 << k) | (1 << l);
						index++;
					}	
				}
			}
			
		}
	}
	*/
	return;
}

uint32_t find_row_mask(struct address_in_bank* cluster_head, uint32_t* functions, int threshold)
{
	uint32_t row_mask = 0;
	char* new_address;
	int time;

	struct address_in_bank* head = (struct address_in_bank*) malloc (sizeof(struct address_in_bank*));
	head = cluster_head->next_address;

	for (int bit = 0; bit < ADDRESS_BITS; bit++)
	{
		new_address = change_bit(head->address, bit);
		new_address = switch_bank(new_address, functions, bit);

		time = time_access(new_address, head->address);

		if (time > threshold)
		{
			// printf("Bit[%d] is a part of row_maks\n", bit);
			row_mask |= (1 << bit);
		}

	}
	return row_mask;
}

char* switch_bank(char* new_address, uint32_t* functions, int bit)
{			
	char* same_bank_address = new_address;

	for (int i = 0; i < FN; i++)
	{
		if ( (*(functions + i) >> bit) & 1 )
		{
			for (int shift = 0; shift < ADDRESS_BITS; shift++)
			{
				if ( (*(functions + i) >> shift) & 1 )
					if ( shift != bit)
						same_bank_address = change_bit(same_bank_address, shift);
			}
		}

		else
			continue;
	}

	return same_bank_address;
}

void print_cluster_addresses(struct address_in_bank* cluster_head)
{
	struct address_in_bank* head = (struct address_in_bank*) malloc (sizeof(struct address_in_bank*));
	head = cluster_head; 
	head = head->next_address;

	while(head != NULL)
	{
		printf("%p\n", (void*)head->address);
		head = head->next_address;
	}
}

void print_colours(int colours[POOLSIZE])
{
	for (int i=0; i<POOLSIZE; i++)
		printf("%d\n", colours[i]);
}

