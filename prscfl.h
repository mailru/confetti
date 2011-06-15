#ifndef PRSCFL_H
#define PRSCFL_H

#include <stdbool.h>
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
		boolType 	= 6,
		commentType = 7,
		structType 	= 8,
		arrayType	= 9,
		builtinType = 10
	} paramType;

	union {
		int32_t			int32val;
		int64_t			int64val;
		u_int32_t		uint32val;
		u_int64_t		uint64val;
		double			doubleval;
		bool			boolval;
		char			*stringval;
		char			*commentval;
		struct ParamDef *structval;
		struct ParamDef *arrayval;
		char			*builtinval;
	} paramValue;

	char	*name;

	int		flags;

	struct ParamDef	*comment;
	struct ParamDef	*parent;
	struct ParamDef	*next;
} ParamDef;

#define PARAMDEF_RDONLY			(0x01)
#define PARAMDEF_REQUIRED		(0x02)

ParamDef* parseCfgDef(FILE *fh);
void hDump(FILE *fh, char *name, ParamDef *def);
void dumpStructName(FILE *fh, ParamDef *def, char *delim);
void cDump(FILE *fh, char *name, ParamDef *def);
void fDump(FILE *fh, ParamDef *def);
void pDump(FILE *fh, ParamDef *def);
void HDump(FILE *fh);
void dDump(ParamDef *def);

#endif
