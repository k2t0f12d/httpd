all: server

clean:
	@rm -rf *.o
	@rm -rf server

server: httpd.o
	gcc -o server $^

httpd.o: httpd.c
	gcc -c -o httpd.o httpd.c
