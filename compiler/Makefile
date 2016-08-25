CFLAGS=-Wall -s

DEPS=lex.h syntax.h print.h parse.h log.h eval.h mem.h
OBJ=fuspelc.o lex.o syntax.o print.o parse.o log.o eval.o mem.o
EXE=fuspelc

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(RM) -v $(OBJ) $(EXE)

.PHONY: all clean
