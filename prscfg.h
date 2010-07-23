#ifndef PRSCFG_H
#define PRSCFG_H

#include <sys/types.h>

typedef struct prscfg_yy_extra_type {
	/*
	 * string
	 */
	char *strbuf;
	int length;
	int total;

	int	commentCounter;
} prscfg_yy_extra_type;

/*
 * The type of yyscanner is opaque outside scan.l.
 */

typedef void *prscfg_yyscan_t;
prscfg_yyscan_t prscfgScannerInit(FILE *fh, prscfg_yy_extra_type *yyext);
void prscfgScannerFinish(prscfg_yyscan_t scanner);


typedef struct NameAtom {
	char			*name;
	int				index;
	struct NameAtom *next;
} NameAtom;

typedef struct OptDef {

	enum {
		numberType 	= 0,
		stringType 	= 1,
		structType 	= 2,
		arrayType	= 3 
	} paramType;

	union {
		char			*numberval;
		char			*stringval;
		struct OptDef 	*structval;
		struct OptDef 	*arrayval;
	} paramValue;

	NameAtom		*name;

	struct OptDef	*parent;
	struct OptDef	*next;
} OptDef;


#endif
