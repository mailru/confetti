#ifndef PRSCFL_H
#define PRSCFL_H

#include <sys/types.h>

typedef struct prscfl_yy_extra_type {
	/*
	 * string
	 */
	char *strbuf;
	int length;
	int total;

	int lineno;

} prscfl_yy_extra_type;

/*
 * The type of yyscanner is opaque outside scan.l.
 */

typedef void *prscfl_yyscan_t;
int prscflGetLineNo(prscfl_yyscan_t scanner);
prscfl_yyscan_t prscflScannerInit(FILE *fh, prscfl_yy_extra_type *yyext);
void prscflScannerFinish(prscfl_yyscan_t scanner);

typedef struct ParamDef {

	enum {
		int32Type 	= 0,
		int64Type 	= 1,
		uint32Type 	= 2,
		uint64Type 	= 3,
		doubleType 	= 4,
		stringType 	= 5,
		commentType = 6,
		structType 	= 7,
		arrayType	= 8,
		builtinType = 9
	} paramType;

	union {
		int32_t			int32val;
		int64_t			int64val;
		u_int32_t		uint32val;
		u_int64_t		uint64val;
		double			doubleval;
		char			*stringval;
		char			*commentval;
		struct ParamDef *structval;
		struct ParamDef *arrayval;
		char			*builtinval;
	} paramValue;

	char	*name;

	int		rdonly;

	struct ParamDef	*comment;
	struct ParamDef	*parent;
	struct ParamDef	*next;
} ParamDef;

ParamDef* parseCfgDef(FILE *fh);
void hDump(FILE *fh, char *name, ParamDef *def);
void dumpStructName(FILE *fh, ParamDef *def, char *delim);
void cDump(FILE *fh, char *name, ParamDef *def);
void fDump(FILE *fh, ParamDef *def);
void pDump(FILE *fh, ParamDef *def);
void HDump(FILE *fh);
void dDump(ParamDef *def);

#endif
