all: aesdsocket
default: aesdsocket

aesdsocket: aesdsocket.o
#		"$(CC)" -g server.c -o aesdsocket
		"$(CROSS_COMPILE)"gcc -g server.c -o aesdsocket

aesdsocket.o: server.c
#	"$(CC)" -g -c server.c -o aesdsocket.o
	"$(CROSS_COMPILE)"gcc -g -c server.c -o aesdsocket.o

clean:
	rm -f aesdsocket.o
	rm -f aesdsocket

