output: main.o Order.o Limit.o
	g++ main.o Order.o Limit.o -o tester

main.o: main.cpp
	g++ -c main.cpp

Order.o: Order/Order.cpp Order/Order.h
	g++ -c Order/Order.cpp

Limit.o: Limit/Limit.cpp Limit/Limit.h
	g++ -c Limit/Limit.cpp

clean:
	rm *.o 
