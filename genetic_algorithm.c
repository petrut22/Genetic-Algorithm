#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"
#include <pthread.h>

struct my_arg {
	int id;
	individual **current_generation;
	individual **next_generation;
	int object_count;
	int generations_count;
	int sack_capacity;
	int thread_count;
	const sack_object *objects;
	pthread_barrier_t *barrier;

};


int min(int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}


int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *thread_count, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 4) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	*thread_count = (int) strtol(argv[3], NULL, 10);

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int sack_capacity, int thread_id, int thread_count)
{
	int weight;
	int profit;

	int start_index = thread_id * (double) object_count / thread_count;
	int end_index = min((thread_id + 1) * (double) object_count / thread_count, object_count);

	for (int i = start_index; i < end_index; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}
//functia utilizata pentru sortare conform criteriilor din enunt
int compare(individual *first, individual *second)
{
	int i;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}
//functie merge
void merge(individual *source, int start, int mid, int end, individual *destination) {
	int iA = start;
	int iB = mid;
	int i;
	//destinatia va cuprinde cele doua secventa ordonate 
	for (i = start; i < end; i++) {
		if (end == iB || (iA < mid && compare(&source[iA], &source[iB]) <=0  )) {
			destination[i] = source[iA];
			iA++;
		} else {
			destination[i] = source[iB];
			iB++;
		}
	}
}
//functia de sortare
void mergeSort(individual *generation, int object_count, int thread_id, int thread_count, pthread_barrier_t *barrier)
{

    int i, j;
	//calculam indecsii pentru fiecare thread
	int start_index = thread_id * (double) object_count / thread_count;
	int end_index = min((thread_id + 1) * (double) object_count / thread_count, object_count);
	//in vectorul acesta vor adauga merge-ul facut pe cele doua secvente ordonate din vector
	individual *generationNew = (individual*) calloc(object_count, sizeof(individual));
	//realizez sortarea pe fiecare secventa destinata unui anumite thread
	qsort(generation + start_index, end_index - start_index, sizeof(individual), cmpfunc);

	//asteptam toate thread-urile sa isi finalizeze sortarea
    pthread_barrier_wait(barrier);
	//facem merge-ul ca sa obtinem noul vector sortat
    if(thread_id == 0) {
		// i reprezinta indexul de delimitare ditre secventa curenta si cea urmatoare
        i = (double) object_count / thread_count; 
		// j reprezinta indexul de final pentru urmatoarea secventa
        j = (double) object_count / thread_count * 2;
        while(i < j) {
			//realizez merge-ul si obtin noua secventa
			merge(generation, 0, i, j, generationNew);
			//ne asiguram sa sa adaugam secventa rezultata inapoi in vector
			memmove(generation, generationNew, j * sizeof(individual));
			//actualizam indecsii, astfel pentru pasul urmator
			i = j;
            j = min( (j + (double) object_count / thread_count), object_count);
        }
    }

	 pthread_barrier_wait(barrier);

}


// iterate for each generation
void *thread_function(void *arg)
{



 	struct my_arg* data = (struct my_arg*) arg;
	individual *tmp = NULL;
	int count;
	int cursor;

	//implementati aici merge sort paralel

	int start_index;
	int end_index;

	for (int k = 0; k < data->generations_count; ++k) {
		cursor = 0;
		// compute fitness and sort by it
		pthread_barrier_wait(data->barrier);
		compute_fitness_function(data->objects, *data->current_generation, data->object_count, data->sack_capacity, data->id, data->thread_count);


		//realizez sortarea cu ajutorul threadurior
		pthread_barrier_wait(data->barrier);
		mergeSort(*data->current_generation, data->object_count, data->id, data->thread_count, data->barrier);
		pthread_barrier_wait(data->barrier);


		//keep first 30% children (elite children selection)

		count = data->object_count * 3 / 10;
		start_index = data->id * (double)count/ data->thread_count;
		end_index = min((data->id + 1) * (double)count/ data->thread_count, count);
		
		pthread_barrier_wait(data->barrier);

		for (int i = start_index; i < end_index; ++i) {
			copy_individual(*data->current_generation + i, *data->next_generation + i);
		}

		pthread_barrier_wait(data->barrier);
		

		cursor = count;
		// // // mutate first 20% children with the first version of bit string mutation
		count = data->object_count * 2 / 10;
		start_index = data->id * (double)count/ data->thread_count;
		end_index = min((data->id + 1) * (double)count/ data->thread_count, count);

		pthread_barrier_wait(data->barrier);

		for (int i = start_index; i < end_index; ++i) {
			copy_individual(*data->current_generation + i, *data->next_generation + cursor + i);
			mutate_bit_string_1(*data->next_generation + cursor + i, k);
		}

		pthread_barrier_wait(data->barrier);

	

		cursor += count;

		// // // mutate next 20% children with the second version of bit string mutation
		count = data->object_count * 2 / 10;
		start_index = data->id * (double)count/ data->thread_count;
		end_index = min((data->id + 1) * (double)count/ data->thread_count, count);

		for (int i = start_index; i < end_index; ++i) {
			copy_individual(*data->current_generation + i + count, *data->next_generation + cursor + i);
			mutate_bit_string_2(*data->next_generation + cursor + i, k);
		}
		pthread_barrier_wait(data->barrier);

		cursor += count;
		// // // crossover first 30% parents with one-point crossover
		// // // (if there is an odd number of parents, the last one is kept as such)
		count = data->object_count * 3 / 10;
		start_index = data->id * (double)count/ data->thread_count;
		end_index = min((data->id + 1) * (double)count/ data->thread_count, count);

		pthread_barrier_wait(data->barrier);
		if (data->id == 0 && count % 2 == 1) {
			copy_individual(*data->current_generation + data->object_count - 1, *data->next_generation + cursor + count - 1);
			count--;
		}
		pthread_barrier_wait(data->barrier);

		if(start_index % 2 == 1) {
			start_index++;
		}

		
		for (int i = start_index; i < end_index && i < count - 1; i += 2) {
			crossover(*data->current_generation + i, *data->next_generation + cursor + i, k);
		}

		pthread_barrier_wait(data->barrier);

		// // switch to new generation
		if(data->id == 0) {
			tmp = *data->current_generation;
			*data->current_generation = *data->next_generation;
			*data->next_generation = tmp;
		}

		start_index = data->id * (double)data->object_count/ data->thread_count;
		end_index = min((data->id + 1) * (double)data->object_count/ data->thread_count, count);
		pthread_barrier_wait(data->barrier);

		for (int i = start_index; i < end_index; ++i) {
			(*data->current_generation)[i].index = i;
		}
		//se face printarea pentru fiecare generatie buna pe un singur thread
		pthread_barrier_wait(data->barrier);
		if (data->id == 0 && k % 5 == 0) {
			print_best_fitness(*data->current_generation);
		}

		pthread_barrier_wait(data->barrier);

	}

	pthread_barrier_wait(data->barrier);
	compute_fitness_function(data->objects, *data->current_generation, data->object_count, data->sack_capacity, data->id, data->thread_count);

	//realizez sortarea cu ajutorul threadurior
	pthread_barrier_wait(data->barrier);
	mergeSort(*data->current_generation, data->object_count, data->id, data->thread_count, data->barrier);
	pthread_barrier_wait(data->barrier);

	//se face printarea pentru fiecare generatie buna pe un singur thread
	 if(data->id == 0) {
	 	print_best_fitness(*data->current_generation);
	 }

	pthread_exit(NULL);
	
}

void run_genetic_algorithm(const sack_object *objects, int object_count, int generations_count, int sack_capacity, int thread_count)
{
	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));
	pthread_t *threads = (pthread_t*) malloc(thread_count * sizeof(pthread_t));
	int r;
	void *status;
	pthread_barrier_t barrier;
	struct my_arg *arguments = (struct my_arg*) malloc(thread_count * sizeof(struct my_arg));
	pthread_barrier_init(&barrier, NULL, thread_count);

	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = 0; i < object_count; ++i) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = object_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = object_count;
	}
	
	//realizarea algoritmului cu ajutorul threadurilor
	//in arguments retin toate datele legate de vectorii de generatii, inclusiv
	//bariera, nr de thread-uri si id-ul thread-ului
	for (int i = 0; i < thread_count; i++) {
		arguments[i].id = i;
		arguments[i].current_generation = &current_generation;
		arguments[i].next_generation = &next_generation;
		arguments[i].object_count = object_count;
		arguments[i].generations_count = generations_count;
		arguments[i].sack_capacity = sack_capacity;
		arguments[i].thread_count = thread_count;
		arguments[i].objects = objects;
		arguments[i].barrier = &barrier;

		r = pthread_create(&threads[i], NULL, thread_function, &arguments[i]);

		if (r) {
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}
	
	for (int i = 0; i < thread_count; i++) {
		r = pthread_join(threads[i], &status);

		if (r) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	
	}
	//free barrier
	pthread_barrier_destroy(&barrier);

	// free resources for old generation
	free_generation(current_generation);
	free_generation(next_generation);

	// // free resources
	free(current_generation);
	free(next_generation);
	//free threads
	free(threads);
}