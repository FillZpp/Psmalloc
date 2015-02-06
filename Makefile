# Copyright (C) 2014 FillZpp


DIR_SRC = ./src
SRC = $(wildcard ${DIR_SRC}/*.c)
OBJ = $(patsubst %.c, %.o, $(notdir $(SRC)))
TEST = ./test/test.c

SYS_LIB_DIR = /usr/lib
CC = gcc
LD = ld
CFLAGS = -g -Wall -O2
TARGET = libpsmalloc.so
MIN_TARGET = libpsmalloc_minimal.so
MAJOR_V = 0
MINOR_V = 1
THREAD_NUM = 100

all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) -shared -lpthread -soname $(TARGET) -o $@ $^

$(OBJ) : $(SRC)
	$(CC) $(CFLAGS) -fpic -lpthread -c $^


.PHONY:install
install:
	strip $(TARGET) -S -o $(MIN_TARGET)
	cp $(TARGET) $(SYS_LIB_DIR)/$(TARGET).$(MAJOR_V).$(MINOR_V)
	ln -sf $(SYS_LIB_DIR)/$(TARGET).$(MAJOR_V).$(MINOR_V) $(SYS_LIB_DIR)/$(TARGET).$(MAJOR_V)
	ln -sf $(SYS_LIB_DIR)/$(TARGET).$(MAJOR_V) $(SYS_LIB_DIR)/$(TARGET)
	cp $(MIN_TARGET) $(SYS_LIB_DIR)/$(MIN_TARGET).$(MAJOR_V).$(MINOR_V)
	ln -sf $(SYS_LIB_DIR)/$(MIN_TARGET).$(MAJOR_V).$(MINOR_V) $(SYS_LIB_DIR)/$(MIN_TARGET).$(MAJOR_V)
	ln -sf $(SYS_LIB_DIR)/$(MIN_TARGET).$(MAJOR_V) $(SYS_LIB_DIR)/$(MIN_TARGET)


.PHONY:test
test:
	$(CC) -lpthread $(TEST) -o test_libc
	$(CC) -lpthread -ltcmalloc $(TEST) -o test_tcmalloc
	$(CC) -lpthread -lpsmalloc $(TEST) -o test_psmalloc
	@echo -e "\nlibc:"
	@./test_libc $(THREAD_NUM)
	@echo -e "\ntcmalloc:"
	@./test_tcmalloc $(THREAD_NUM)
	@echo -e "\npsmalloc:"
	@./test_psmalloc $(THREAD_NUM)
	@rm -f test_libc test_tcmalloc test_psmalloc


.PHONY:distclean
distclean:
	rm -f *.o


.PHONY:clean
clean:
	rm -f *.o *.so
