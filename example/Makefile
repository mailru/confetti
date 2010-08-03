CONFETTI=../confetti
NAME=my_product
CFG=example.cfgtmpl

CC=gcc

CFLAGS=-Wall -g -O0 -Werror -pedantic -std=c99
INCLUDE=-I.
LIB=

example_OBJS=example.o cfg.o 

example_buffer_OBJS=example_buffer.o cfg.o 

all: $(NAME).cfg example example_buffer 

example.c: $(NAME)_cfg.h

example_buffer.c: $(NAME)_cfg.h

cfg.c: $(NAME)_cfg.c $(NAME)_cfg.h prscfg.c prscfg.h

cfg.o: $(NAME)_cfg.c $(NAME)_cfg.h prscfg.c prscfg.h

.SUFFIXES: .o.c

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

example: $(example_OBJS)
	$(CC) -o $@ $(example_OBJS) $(LIB)

example_buffer: $(example_buffer_OBJS)
	$(CC) -o $@ $(example_buffer_OBJS) $(LIB)

$(NAME).cfg: $(CFG)
	$(CONFETTI) -i $< -n $(NAME) -f $@

$(NAME)_cfg.c: $(CFG)
	$(CONFETTI) -i $< -n $(NAME) -c $@

$(NAME)_cfg.h: $(CFG)
	$(CONFETTI) -i $< -n $(NAME) -h $@

prscfg.c: $(CFG)
	$(CONFETTI) -i $< -p $@

prscfg.h: $(CFG)
	$(CONFETTI) -i $< -H $@

clean:
	rm -f $(NAME).cfg $(NAME)_cfg.c $(NAME)_cfg.h
	rm -f prscfg.c prscfg.h
	rm -f $(example_OBJS) $(example_buffer_OBJS)
	rm -f example example_buffer


