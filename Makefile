main: parser.o builtin.o model.o main.o
	g++ -o main $^

.cpp.o:
	g++ $< -c -g -DDEBUG

clean:
	rm -f *.o
	rm -f main
