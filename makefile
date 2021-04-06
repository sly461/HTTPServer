src = $(wildcard ./*/*.cpp *.cpp)
obj = server
myArgs = -Wall -g -pthread

all:$(src)
	g++ $(src) -o $(obj) $(myArgs)

clean:
	-rm -rf $(obj)

.PHONY:all clean
