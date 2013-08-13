sonsi: main.o parser.o builtin.o model.o eval.o exc.o consts.o types.o gc.o
	g++ -o sonsi $^ -pg -lgmp

.cpp.o:
	g++ $< -c -O2 -DGMP_SUPPORT -Wall

clean:
	rm -f *.o
	rm -f sonsi

db:
	gdb sonsi

cdb:
	cgdb sonsi

run:
	./sonsi
