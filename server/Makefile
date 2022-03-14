#make all or make => then gcc will be used
#make CROSS_COMPILE=aarch64-none-linux-gnu-

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -pthread 
SRCS = aesdsocket.c 
OBJS = $(SRCS:.c=.o) #replace .c file 
TARGET = $(SRCS:.c=) #replace .c with nothing"" for eg. aesdsocket.c => writer

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

.c.o:
	$(CC) -c -g $(SRCS)

clean:
	rm -f $(TARGET) *.o
