main: main.o parser.o builtin.o model.o eval.o exc.o consts.o
	g++ -o main $^ -pg -lgmp

.cpp.o:
	g++ $< -c -O2 -g -pg -DGMP_SUPPORT

clean:
	rm -f *.o
	rm -f main

db:
	gdb main

cdb:
	cgdb main
