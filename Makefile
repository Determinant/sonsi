main: main.o parser.o builtin.o model.o eval.o
	g++ -o main $^ -g

.cpp.o:
	g++ $< -c -g -DDEBUG

clean:
	rm -f *.o
	rm -f main

db:
	gdb main
