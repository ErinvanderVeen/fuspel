override CFLAGS+=\
	-Werror -Wall -Wextra\
	-Wno-missing-field-initializers\
	-fPIC\
	-O3\
	-DFUSPEL_CLI\
	-DFUSPEL_DEBUG

DEPS=fuspel.h lex.h syntax.h print.h parse.h eval.h mem.h code.h graphs.h
OBJ=fuspel.o lex.o syntax.o print.o parse.o eval.o mem.o code.o graphs.o
EXE=fuspel

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(RM) $(OBJ) $(EXE)
