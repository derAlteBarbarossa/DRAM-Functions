#include "util.h"


int main(int argc, char* argv[])
{
	char* buffer;
	char** pool;
	struct address_in_bank* cluster_head = (struct address_in_bank*) malloc(sizeof(struct address_in_bank*));
	uint32_t* functions = (uint32_t*) malloc (sizeof(uint32_t) * FN);
	uint32_t significant_bits;
	uint32_t row_mask;
	int num_banks;
	int threshold;
	int* colours = (int*) malloc(sizeof(int)*POOLSIZE);

	int c = getopt (argc, argv, "bfm");

	switch(c)
	{
		case 'b':
			buffer = allocate_superpage();
			pool = initialise_pool(buffer);
			threshold = find_threshold(buffer);
			colours = find_banks(buffer, pool, threshold);
			cluster_head = cluster_addresses(pool, colours, 1);
			significant_bits = find_significant_bits(cluster_head, threshold);
			printf("%x\n", significant_bits);

			break;

		case 'f':
			buffer = allocate_superpage();
			pool = initialise_pool(buffer);
			threshold = find_threshold(buffer);
			colours = find_banks(buffer, pool, threshold);
			functions = (uint32_t*) malloc (sizeof(uint32_t) * FN);
			num_banks = number_of_banks(colours);
			functions = find_functions(pool, colours, num_banks);
			print_functions(functions);

			break;

		case 'm':
			buffer = allocate_superpage();
			pool = initialise_pool(buffer);
			threshold = find_threshold(buffer);
			colours = find_banks(buffer, pool, threshold);
			cluster_head = cluster_addresses(pool, colours, 1);
			functions = (uint32_t*) malloc (sizeof(uint32_t) * FN);
			num_banks = number_of_banks(colours);
			functions = find_functions(pool, colours, num_banks);
			row_mask = find_row_mask(cluster_head, functions, threshold);
			printf("%x\n", row_mask);

			break;

		case -1:
			printf("Please choose one of the following options:\n");
			printf("\t -b : prints significant bits in hexadecimal\n");
			printf("\t -f : prints total number of function masks\n");
			printf("\t      and each one of them in hexadecimal\n");
			printf("\t -m : prints the row mask in hexadecimal\n");
			break;

	}

    return 0;
}
