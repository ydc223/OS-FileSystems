all: mymirror 


mymirror: mymirror.o utility.o
	g++ mymirror.o utility.o -o mymirror 

mymirror.o: mymirror.cpp
	g++ -g -c -std=c++11 mymirror.cpp 

utility.o: utility.cpp
	g++ -c -Wall -O2 -std=c++11 -I. utility.cpp 

clean:
	rm -rf *o mymirror 

$(OBJECTS): utility.h tree.hh

