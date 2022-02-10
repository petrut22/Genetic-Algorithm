
build:
	@echo "Building..."
	@gcc -o tema1_par tema1.c genetic_algorithm.c -lpthread -lm -Wall -Werror
	@echo "Done"

build_debug:
	@echo "Building debug..."
	@gcc -o tema1_par tema1.c genetic_algorithm.c -lpthread -lm -Wall -Werror -O0 -g3 -DDEBUG
	@echo "Done"

clean:
	@echo "Cleaning..."
	@rm -rf tema1_par
	@echo "Done"
