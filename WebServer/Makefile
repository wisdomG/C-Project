CC := g++
OBJECT := main.o
TARGET := webserver
LIBS := -lpthread -lboost_system
CXXFLAGS := -std=c++11

$(TARGET) : $(OBJECT)
	$(CC) -o $(TARGET) $(OBJECT) $(LIBS)

%.o : %.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	-rm -f $(OBJECT) $(TARGET)
