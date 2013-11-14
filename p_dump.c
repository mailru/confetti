#include <stdio.h>
#include <prscfl.h>

static char* parserSource[] =	{
	"#include <stdio.h>\n\n"
	"typedef struct prscfg_yy_extra_type {\n"
	"	char *strbuf;\n"
	"	int length;\n"
	"	int total;\n"
	"	int lineno;\n"
	"	int commentCounter;\n"
	"	int ostate;\n"
	"} prscfg_yy_extra_type;\n"
	"typedef void *prscfg_yyscan_t;\n"
	"static prscfg_yyscan_t prscfgScannerInit(FILE *fh, prscfg_yy_extra_type *yyext);\n"
	"static prscfg_yyscan_t prscfgScannerInitBuffer(char *buffer, prscfg_yy_extra_type *yyext);\n"
	"static void prscfgScannerFinish(prscfg_yyscan_t scanner);\n"
	"static void prscfgScannerStartValue(prscfg_yyscan_t scanner);\n"
	"static void prscfgScannerEndValue(prscfg_yyscan_t scanner);\n"
	"static int prscfgGetLineNo(prscfg_yyscan_t yyscanner);\n\n",
#include "parse_source.c"
	NULL
};

static char* headerSource[] =	{ 
#include "header_source.c"
	NULL
};

void 
pDump(FILE *fh, ParamDef *def) {
	char **source = parserSource;

	if (def->paramType == builtinType)
		fputs(def->paramValue.stringval, fh);

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

