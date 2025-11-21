# variable definitions
# we can use these to simplify our recipes
CC = gcc

# default recipe
# typing make with no arguments will run this recipe
all: mysh

mysh: mysh.o
	$(CC) $(CFLAGS) mysh.o -o mysh

mysh.o: mysh.c
	$(CC) $(CFLAGS) -c mysh.c

# this recipe removes the executable and any .o files

clean:
	rm -f *.o mysh
