all: aesdsocket
default: aesdsocket

aesdsocket: aesdsocket.o
		"$(CC)" -g server_threads.c -o aesdsocket -lm
#		"$(CROSS_COMPILE)"gcc -g server.c -o aesdsocket

aesdsocket.o: server_threads.c
	"$(CC)" -g -c server_threads.c -o aesdsocket.o
#	"$(CROSS_COMPILE)"gcc -g -c server.c -o aesdsocket.o

clean:
	rm -f aesdsocket.o
	rm -f aesdsocket

