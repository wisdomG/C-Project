CC := g++
TARGET := LeakDetector
LIBS := 

CXXFLAGS := -std=c++11

OBJECTS := LeakDetector.o main.o

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LIBS)

%.o : %.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(TARGET)
