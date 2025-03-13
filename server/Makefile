CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -g -fno-diagnostics-color
TARGET=aesdsocket

all: $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm -f $(TARGET) $(TARGET).o
