all: aesdsocket
default: aesdsocket

#CFLAGS ?= -g Wall -Wextra
CFLAGS ?= -g -Wextra
aesdsocket: aesdsocket.o
		$(CC) $(CFLAGS) aesdsocket.o -o aesdsocket $(LDFLAGS) -lm -lpthread
#		"$(CROSS_COMPILE)"gcc -g server.c -o aesdsocket

aesdsocket.o: server_threads.c
	$(CC) $(CFLAGS) -c server_threads.c -o aesdsocket.o
#	"$(CROSS_COMPILE)"gcc -g -c server.c -o aesdsocket.o

clean:
	rm -f aesdsocket.o
	rm -f aesdsocket

