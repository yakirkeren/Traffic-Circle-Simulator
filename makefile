CC = gcc

CFLAGS = -g -Wall -pthread

TARGET = TrafficCircle
TARGET1 = Traffic_Circle

all:	clean	$(TARGET)

$(TARGET):	$(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET1) $(TARGET).c

clean:
	$(RM) $(TARGET)
