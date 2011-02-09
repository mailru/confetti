CC=gcc
AR=ar
RANLIB=ranlib
LD=ld
BISON=bison -y -d 
FLEX=flex

ARFLAGS=rcv
LDFLAGS=-r
CFLAGS=-Wall -g -O0 -Werror -pedantic -std=gnu99
INCLUDE=-I.
LIB=

OBJS=confetti.o h_dump.o f_dump.o c_dump.o p_dump.o d_dump.o
PRSSRC = prscfl

all: confetti

ifdef PRSSRC
OBJS += $(PRSSRC:%=%_scan.o)
OBJS += $(PRSSRC:%=%_gram.o)

$(PRSSRC:%=%_scan.o): $(PRSSRC:%=%_gram.c)

$(PRSSRC:%=%_scan.c): $(PRSSRC:%=%.l)
	$(FLEX) -o$@ $<

$(PRSSRC:%=%_gram.c): $(PRSSRC:%=%.y)
	$(BISON) $<
	mv -f y.tab.c $@
	mv -f y.tab.h $(@:%.c=%.h)
endif

.SUFFIXES: .o.c

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

confetti: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIB)

example: all
	$(MAKE) -C example -f Makefile all


p_dump.c: parse_source.c header_source.c

p_dump.o: p_dump.c parse_source.c header_source.c

parse_source.c: prscfg_gram.c prscfg_gram.h prscfg_scan.c
	cat prscfg_gram.h prscfg_gram.c prscfg_scan.c | \
		perl ./makec.pl \
	> $@

header_source.c: prscfg.h
	perl ./makec.pl < $< > $@

prscfg_scan.c: prscfg.l
	$(FLEX) -o$@ $<

prscfg_gram.c: prscfg.y
	$(BISON) $<
	mv -f y.tab.c $@
	mv -f y.tab.h $(@:%.c=%.h)

test: example
	@[ -d results ] || mkdir results
	@[ -d diffs ] || mkdir diffs
	@for FILE in dump default defcfg custom buffer ; do \
		echo -n $$FILE	"	........ " ; \
		if sh tests/$$FILE > results/$$FILE 2>results/$$FILE.errout && diff -c expected/$$FILE results/$$FILE > diffs/$$FILE ; then \
			echo ok ; \
		else \
			echo FAILED ; \
		fi ; \
	done

clean:
	rm -rf confetti $(OBJS)
	rm -rf *core y.tab.*
	rm -rf results diffs
ifdef PRSSRC
	for prs in $(PRSSRC); do rm -rf $${prs}_gram.c $${prs}_scan.c $${prs}_gram.h ; done
endif
	rm -f prscfg_gram.c prscfg_gram.h prscfg_scan.c parse_source.c header_source.c
	$(MAKE) -C example -f Makefile clean

