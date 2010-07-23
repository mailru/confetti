#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <prscfl.h>

static void
usage() {
	fputs(
		"Copyright (c) 2010 Teodor Sigaev <teodor@sigaev.ru>. All rights reserved.\n"
		"Usage:\n"
		"confetti -i INPUTFILE [-c COUTFILE -n NAME | -h HOUTFILE -n NAME | -f CFGOUTFILE | -p PARSEROUTFILE | -H HPARSEOUTFILE]\n",
		stdout
	);

	exit(1);
}

extern char *optarg;
extern int opterr;

int
main(int argc, char* argv[]) {
	char		*ifn = NULL, *cfn = NULL, *hfn = NULL, *cfgfn = NULL, *pfn = NULL, *Hfn = NULL;
	FILE		*fh;
	char		*name = NULL;
	int			i;
	ParamDef	*def;

	opterr=0;
	while((i=getopt(argc,argv,"i:c:h:f:n:p:H:")) != EOF) {
		switch(i) {
			case 'i':
				ifn = strdup(optarg);
				break;
			case 'c':
				cfn = strdup(optarg);
				break;
			case 'h':
				hfn = strdup(optarg);
				break;
			case 'f':
				cfgfn = strdup(optarg);
				break;
			case 'p':
				pfn = strdup(optarg);
				break;
			case 'H':
				Hfn = strdup(optarg);
				break;
			case 'n':
				name = strdup(optarg);
				break;
			default:
				usage();
		}
	}

	if ( !ifn )
		usage();

	if ( (fh = fopen(ifn, "r")) == NULL ) {
		fprintf(stderr, "Could not open file '%s': %s\n", ifn, strerror(errno));
		exit(1);
	}

	if ( (def = parseCfgDef(fh)) == NULL ) {
		fprintf(stderr, "Could not parse file '%s'\n", ifn);
		exit(1);
	}

	fclose(fh);

	if ( hfn ) {
		if (!name)
			usage();

		if (strcmp(hfn, "-") == 0) {
			fh = stdout;
		} else if ((fh = fopen(hfn, "w")) == NULL ) {
			fprintf(stderr, "Could not open file '%s': %s\n", hfn, strerror(errno));
			exit(1);
		}
	
		hDump(fh, name, def);

		if (fh != stdout)
			fclose(fh);
	}

	if ( cfn ) {
		if (!name)
			usage();

		if (strcmp(cfn, "-") == 0) {
			fh = stdout;
		} else if ((fh = fopen(cfn, "w")) == NULL ) {
			fprintf(stderr, "Could not open file '%s': %s\n", cfn, strerror(errno));
			exit(1);
		}
	
		cDump(fh, name, def);

		if (fh != stdout)
			fclose(fh);
	}

	if ( cfgfn ) {
		if (strcmp(cfgfn, "-") == 0) {
			fh = stdout;
		} else if ((fh = fopen(cfgfn, "w")) == NULL ) {
			fprintf(stderr, "Could not open file '%s': %s\n", cfgfn, strerror(errno));
			exit(1);
		}
	
		fDump(fh, def);

		if (fh != stdout)
			fclose(fh);
	}

	if ( pfn ) {
		if (strcmp(pfn, "-") == 0) {
			fh = stdout;
		} else if ((fh = fopen(pfn, "w")) == NULL ) {
			fprintf(stderr, "Could not open file '%s': %s\n", pfn, strerror(errno));
			exit(1);
		}
	
		pDump(fh);

		if (fh != stdout)
			fclose(fh);
	}

	if ( Hfn ) {
		if (strcmp(Hfn, "-") == 0) {
			fh = stdout;
		} else if ((fh = fopen(Hfn, "w")) == NULL ) {
			fprintf(stderr, "Could not open file '%s': %s\n", Hfn, strerror(errno));
			exit(1);
		}
	
		HDump(fh);

		if (fh != stdout)
			fclose(fh);
	}

	exit(0);
}
