sonsi: main.o parser.o builtin.o model.o eval.o exc.o consts.o types.o gc.o
	g++ -o sonsi $^ -pg -lgmp

.cpp.o:
	g++ $< -c -g -DGMP_SUPPORT -Wall -DGC_INFO

clean:
	rm -f *.o
	rm -f sonsi

db:
	gdb sonsi

cdb:
	cgdb sonsi

run:
	./sonsi
