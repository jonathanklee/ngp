CC = gcc


SRC = $(shell find -name "*.c")
OBJ = $(SRC:.c=.o)

TARGET = dos
LDFLAGS = -lncurses -lconfig
CFLAGS = 
INCLUDE_DIR = 

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIR) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
	
