CC = g++
EXEC = dpll

all:
	@echo "Building dpll..."
	$(CC) $(FLAGS) dpll.cpp -o $(EXEC)
	@echo "Done."

