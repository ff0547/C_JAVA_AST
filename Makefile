# Makefile for Java AST Parser

CC = gcc
CFLAGS = -Wall -std=c99 -g
YACC = bison

# Source files
YACC_SRC = parser.y
AST_SRC = java_ast.c
MAIN_SRC = main.c
LEX_SRC = lex.c
NEW_SRC = new.c

# Generated files
YACC_C = parser.tab.c
YACC_H = parser.tab.h

# Object files
OBJ = $(LEX_SRC:.c=.o) $(NEW_SRC:.c=.o) $(YACC_C:.c=.o) $(AST_SRC:.c=.o) $(MAIN_SRC:.c=.o)

# Executable (Windows)
TARGET = java_parser

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

YACC_FLAGS = -d -v

$(YACC_C) $(YACC_H): $(YACC_SRC)
	$(YACC) -d -v -o $(YACC_C) $(YACC_SRC)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /f *.o $(TARGET) 2>nul || rm -f *.o $(TARGET)

test: $(TARGET)
	./$(TARGET) HelloWorld.java

.PHONY: all clean test
