sonsi: main.o parser.o builtin.o model.o eval.o exc.o consts.o types.o
	g++ -o sonsi $^ -pg -lgmp

.cpp.o:
	g++ $< -c -g -pg -DGMP_SUPPORT -Wall -O2

clean:
	rm -f *.o
	rm -f sonsi

db:
	gdb sonsi

cdb:
	cgdb sonsi
