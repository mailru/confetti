%{
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <prscfl.h>
#include <prscfl_gram.h>

static int prscfl_yyerror(prscfl_yyscan_t yyscanner, char *msg);
extern int prscfl_yylex (YYSTYPE * yylval_param, prscfl_yyscan_t yyscanner);
static ParamDef	*output;

#define MakeScalarParam(r, t, n, v)	do {  	\
	(r) = malloc(sizeof(ParamDef));			\
	(r)->paramType = t##Type;				\
	(r)->paramValue.t##val = (v);			\
	(r)->name = (n);						\
	(r)->parent = NULL;						\
	(r)->next = NULL;						\
} while(0)

#define MakeList(r, f, l)					\
	if (f) {								\
		(r) = (f);							\
		(r)->next = (l);					\
	} else {								\
		(r) = (l);							\
	}

#define SetParent(p, l)	do {				\
	ParamDef *i = (l);						\
	while(i) {								\
		i->parent = (p);					\
		i = i->next;						\
	}										\
} while(0)

%}

%pure-parser
%expect 0
%name-prefix="prscfl_yy"

%parse-param {prscfl_yyscan_t yyscanner}
%lex-param   {prscfl_yyscan_t yyscanner}

%union		 {
	int32_t		int32val;	
	u_int32_t	uint32val;	
	int64_t		int64val;	
	u_int64_t	uint64val;	
	double		doubleval;
	char		*str;

	ParamDef	*node;
}

%type	<str>		identifier
%type	<node>		param param_list
%type	<node>		commented_param
%type	<node>		comment
%type	<node>		cfg

%token	<str>		KEY_P NULL_P STRING_P COMMENT_P
%token	<int32val>	INT32_P
%token	<uint32val>	UINT32_P
%token	<int64val>	INT64_P
%token	<uint64val>	UINT64_P
%token	<doubleval>	DOUBLE_P

%%

cfg:
	param_list		{ output = $$ = $1; }
	;

identifier:
	KEY_P			{ $$ = $1; }
	| NULL_P		{ $$ = $1; }
	;

param_list:
	commented_param					{ $$ = $1; }
	| commented_param param_list	{ MakeList($$, $1, $2); }
	;

comment:
	COMMENT_P							{ MakeScalarParam($$, comment, NULL, $1); }
	| COMMENT_P comment					{
			ParamDef	*comment;
			MakeScalarParam(comment, comment, NULL, $1);
			MakeList($$, comment, $2);
		}
	;

commented_param:
	param						{ $$ = $1; }
	| comment param 			{ $$ = $2; $$->comment = $1; }
	;

param:
	identifier '=' INT32_P				{ MakeScalarParam($$, int32, $1, $3); }
	| identifier '=' UINT32_P			{ MakeScalarParam($$, uint32, $1, $3); }
	| identifier '=' INT64_P			{ MakeScalarParam($$, int64, $1, $3); }
	| identifier '=' UINT64_P			{ MakeScalarParam($$, uint64, $1, $3); }
	| identifier '=' DOUBLE_P			{ MakeScalarParam($$, double, $1, $3); }
	| identifier '=' STRING_P			{ MakeScalarParam($$, string, $1, $3); }
	| identifier '=' NULL_P				{ MakeScalarParam($$, string, $1, NULL); }
	| identifier '=' '{' param_list '}' { MakeScalarParam($$, struct, $1, $4); SetParent( $$, $4 ); }
	| identifier '=' '[' param_list ']' { MakeScalarParam($$, array, $1, $4); SetParent( $$, $4 ); }
	;

%%

static int
prscfl_yyerror(prscfl_yyscan_t yyscanner, char *msg) {
    fprintf(stderr, "gram_yyerror: %s\n", msg);
	return 0;
}

ParamDef*
parseCfgDef(FILE *fh) {
	prscfl_yyscan_t			yyscanner;
	prscfl_yy_extra_type	yyextra;
	int						yyresult;

	yyscanner = prscflScannerInit(fh, &yyextra);

	output = NULL;
	yyresult = prscfl_yyparse(yyscanner);
	prscflScannerFinish(yyscanner);

	if (yyresult != 0) 
		return NULL;

	return output;
}

