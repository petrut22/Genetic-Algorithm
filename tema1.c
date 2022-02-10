#include <stdlib.h>
#include <stdio.h>
#include "genetic_algorithm.h"

int main(int argc, char *argv[]) {
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	int thread_count = 0;

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, &thread_count, argc, argv)) {
		return 0;
	}


	//printf("%d %d %d %d\n", object_count, sack_capacity, generations_count, thread_count);
	run_genetic_algorithm(objects, object_count, generations_count, sack_capacity, thread_count);

	free(objects);

	return 0;
}