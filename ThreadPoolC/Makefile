CC := g++
TARGET := threadpool
LIBS := -lpthread

CXXFLAGS := -std=c++11 

OBJECTS := ThreadPool.o main.o

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LIBS)

%.o: %.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(TARGET)
