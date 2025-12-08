CC= g++
CUDA = nvccx
# CObj = main.cpp
CFLAGS = -Wall

make: main-make
	
make-main:
	$(CC) $(CFLAGS) main.cpp -o main 

clean: clean-main

clean-main:
	rm -rf main