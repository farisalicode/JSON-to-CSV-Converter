CXX = g++
CXXFLAGS = -std=c++11
FLEX = flex
BISON = bison
YFLAGS = -d

TARGET = json2relcsv
OBJS = main.o scanner.o parser.tab.o ast.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

ast.o: ast.cpp ast.hpp
	$(CXX) $(CXXFLAGS) -c ast.cpp

parser.tab.c parser.tab.h: parser.y
	$(BISON) $(YFLAGS) parser.y

parser.tab.o: parser.tab.c ast.hpp
	$(CXX) $(CXXFLAGS) -c parser.tab.c

scanner.c: scanner.l parser.tab.h
	$(FLEX) -o scanner.c scanner.l

scanner.o: scanner.c parser.tab.h
	$(CXX) $(CXXFLAGS) -c scanner.c -o scanner.o

main.o: main.cpp ast.hpp parser.tab.h
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f $(TARGET) $(OBJS) scanner.c parser.tab.c parser.tab.h parser.output

