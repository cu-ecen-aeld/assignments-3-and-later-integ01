
writer: writer.o
		"$(CROSS_COMPILE)"gcc writer.c -o writer

writer.o: writer.c
	"$(CROSS_COMPILE)"gcc -c writer.c -o writer.o

clean:
	rm -f writer.o
	rm -f writer

