vpath %.c ./src
CC=gcc-13
TIDY=clang-tidy-15
SRC_FILES=binjecter.c utils.c elfutils.c 
INCLUDE_DIR=src
OBJDIR=bin
SRCDIR=src
CFLAGS=-Wall -pedantic -Wextra -I$(INCLUDE_DIR)
LIBS=-lbfd -lm
EXEC=Binjecter

objfiles=$(SRC_FILES:%.c=$(OBJDIR)/%.o)


all : $(EXEC)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDE_DIR)/binjecter.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXEC): $(objfiles)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

debug: $(objfiles)
	$(CC) -o $(EXEC) $^ -g $(CFLAGS) $(LIBS)

build_dependencies : $(SRC_FILES:%.c=%.dep)
	@cat $^ > make.test
	@rm $^

%.dep: %.c
	@gcc -I$(INCLUDE_DIR) -MM -MF $@ $<

tidy:
	$(TIDY) $(SRCDIR)/*.c -checks=clang-analyzer-* -- -I./$(INCLUDE_DIR)

payload:
	nasm $(SRCDIR)/payload_inject.S
	nasm $(SRCDIR)/payload_goat.S
	mv $(SRCDIR)/payload_inject ./payload_inject
	mv $(SRCDIR)/payload_goat ./payload_got

restore:
	cp ./backup/date_backup ./date

clean:
	rm -f bin/*.o Binjecter a.out make.test date ./payload_inject ./payload_got ./date_tmp

stuff:
	make clean
	make restore
	make payload
	make debug