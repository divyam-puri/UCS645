CXX = /opt/homebrew/opt/llvm/bin/clang++
CXXFLAGS = -O2 -std=c++17 -fopenmp
LDFLAGS = -fopenmp

TARGET = corr
OBJ = main.o correlate.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.cpp correlate.h
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o $(TARGET)
