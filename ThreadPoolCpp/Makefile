objects = main.cpp

build : $(objects)
	g++ -o main $(objects) -std=c++11 -lpthread

.PHONY : clean
clean:
	-rm main $(objects)
