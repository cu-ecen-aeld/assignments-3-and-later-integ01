all: writer

writer: writer.o
		"$(CC)" writer.c -o writer
#		"$(CROSS_COMPILE)"gcc writer.c -o writer

writer.o: writer.c
	"$(CC)" -c writer.c -o writer.o
#	"$(CROSS_COMPILE)"gcc -c writer.c -o writer.o

clean:
	rm -f writer.o
	rm -f writer

