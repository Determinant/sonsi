main: main.o parser.o builtin.o model.o eval.o
	g++ -o main $^ -pg

.cpp.o:
	g++ $< -c -g -pg -DDEBUG

clean:
	rm -f *.o
	rm -f main

db:
	gdb main

cdb:
	cgdb main
