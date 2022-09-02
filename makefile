CC = gcc

CFLAGS = -g -Wall -pthread

TARGET = os_hw3
TARGET1 = OS

all:	clean	$(TARGET)

$(TARGET):	$(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET1) $(TARGET).c

clean:
	$(RM) $(TARGET)
