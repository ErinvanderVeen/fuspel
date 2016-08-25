CFLAGS=-g

DEPS=lex.h syntax.h error.h print.h parse.h log.h eval.h
OBJ=fuspelc.o lex.o syntax.o error.o print.o parse.o log.o eval.o
EXE=fuspelc

all: $(EXE)

$(EXE): $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(RM) -v $(OBJ) $(EXE)

.PHONY: all clean
