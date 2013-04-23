CC = gcc


SRC = $(shell find . -name "*.c")
OBJ = $(SRC:.c=.o)

TARGET = ngp
LDFLAGS = -lncurses -lconfig -lpthread
CFLAGS = 
INCLUDE_DIR = 
CONFIG_FILE = ngprc

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIR) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
	
install:
	cp $(TARGET) /usr/bin
	cp $(CONFIG_FILE) /etc
