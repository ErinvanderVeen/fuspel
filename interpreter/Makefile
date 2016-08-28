CFLAGS=-Wall -Werror -g

DEPS=lex.h syntax.h print.h parse.h log.h eval.h mem.h code.h
OBJ=fuspel.o lex.o syntax.o print.o parse.o log.o eval.o mem.o code.o
EXE=fuspel

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(RM) $(OBJ) $(EXE)
