#include <stdio.h>
#include <prscfl.h>

static char* parserSource[] =	{ 
#include "parse_source.c"
	NULL
};

static char* headerSource[] =	{ 
#include "header_source.c"
	NULL
};

void 
pDump(FILE *fh) {
	char **source = parserSource;

	while(*source) {
		fputs(*source, fh);
		source++;
	}
}

void 
HDump(FILE *fh) {
	char **source = headerSource;

	while(*source) {
		fputs(*source, fh);
		source++;
	}
}

