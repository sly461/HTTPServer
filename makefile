src = $(wildcard *.cpp) # ./src/test.cpp ...
obj = $(patsubst %.cpp, %, $(src))
myArgs = -Wall -g

all:$(obj)

$(obj):%:%.cpp
	g++ $< -o $@ $(myArgs)

clean:
	-rm -rf $(obj)

.PHONY:all clean
