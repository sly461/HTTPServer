src = $(wildcard ./http/*.cpp ./httpserver/*.cpp *.cpp)
obj = server
myArgs = -Wall -g

all:$(src)
	g++ $(src) -o $(obj) $(myArgs)

clean:
	-rm -rf $(obj)

.PHONY:all clean
