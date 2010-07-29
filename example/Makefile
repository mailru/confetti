CONFETTI=../confetti
NAME=my_product
CFG=example.cfgtmpl

CC=gcc

CFLAGS=-Wall -g -O0 -Werror -pedantic -std=c99
INCLUDE=-I.
LIB=

OBJS=example.o cfg.o 

all: $(NAME).cfg example 

example.c: $(NAME)_cfg.h

cfg.c: $(NAME)_cfg.c $(NAME)_cfg.h prscfg.c prscfg.h

cfg.o: $(NAME)_cfg.c $(NAME)_cfg.h prscfg.c prscfg.h

.SUFFIXES: .o.c

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

example: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIB)

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
	rm -f $(OBJS)
	rm -f example


