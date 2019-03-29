LDFLAGS="-1dl"

all: server

clean:
	@rm -rf *.o
	@rm -rf server

server: main.o
	gcc -o server $^

httpd.o: main.c
	gcc -c -o main.o main.c $LDFLAGS
