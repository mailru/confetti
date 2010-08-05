%{

#undef yylval
#undef yylloc

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>

static int prscfg_yyerror(prscfg_yyscan_t yyscanner, char *msg);
extern int prscfg_yylex (YYSTYPE * yylval_param, prscfg_yyscan_t yyscanner);
static OptDef	*output;

#define MakeAtom(r, n)				do {		\
	(r) = malloc(sizeof(NameAtom));				\
	if (!(r)) {									\
		prscfg_yyerror(yyscanner, "No memory");	\
		YYERROR;								\
	}											\
	(r)->name = (n);							\
	(r)->index = -1;							\
	(r)->next = NULL;							\
} while(0)

#define MakeScalarParam(r, t, n, v)	do {  		\
	(r) = malloc(sizeof(OptDef));				\
	if (!(r)) {									\
		prscfg_yyerror(yyscanner, "No memory");	\
		YYERROR;								\
	}											\
	(r)->paramType = t##Type;					\
	(r)->paramValue.t##val = (v);				\
	(r)->name = (n);							\
	(r)->parent = NULL;							\
	(r)->next = NULL;							\
} while(0)

#define MakeList(r, f, l)						\
	if (f) {									\
		(f)->next = (l);						\
		(r) = (f);								\
	} else {									\
		(r) = (l);								\
	}

#define SetParent(p, l) do {                	\
    OptDef *i = (l);                      		\
	while(i) {                              	\
		i->parent = (p);                    	\
		i = i->next;                        	\
	}                                       	\
} while(0)

#define SetIndex(l, in) do {                	\
    OptDef *i = (l);                      		\
	while(i) {                              	\
		i->name->index = (in);              	\
		i = i->next;                        	\
	}                                       	\
} while(0)

%}

%pure-parser
%expect 0
%name-prefix="prscfg_yy"
%error-verbose

%parse-param {prscfg_yyscan_t yyscanner}
%lex-param   {prscfg_yyscan_t yyscanner}

%union		 {
	char		*str;

	OptDef	*node;
	NameAtom	*atom;
}

%type	<atom>		identifier elem_identifier keyname 
%type	<node>		param param_list struct_list
%type	<node>		cfg

%token	<str>		KEY_P NULL_P STRING_P NUMBER_P

%%

cfg:
	param_list		{ output = $$ = $1; }
	;

identifier:
	KEY_P			{ MakeAtom($$, $1); }
	| NULL_P		{ MakeAtom($$, $1); }
	;

elem_identifier:
	identifier						{ $$ = $1; }
	| identifier '[' NUMBER_P ']'	{ 
			$$ = $1; 
			$$->index = atoi($3);
			/* XXX check !*/
			free($3);
		}
	;

keyname:
	identifier						{ $$ = $1; }
	| elem_identifier '.' keyname	{ MakeList($$, $1, $3); }
	;


param_list:
	param				{ $$ = $1; }
	| param param_list	{ MakeList($$, $1, $2); }
	;

param:
	keyname '=' NUMBER_P			{ MakeScalarParam($$, number, $1, $3); }
	| keyname '=' STRING_P			{ MakeScalarParam($$, string, $1, $3); }
	| keyname '=' KEY_P				{ MakeScalarParam($$, string, $1, $3); }
	| keyname '=' NULL_P			{ MakeScalarParam($$, string, $1, NULL); free($3); }
	| keyname '=' '{' param_list '}' { MakeScalarParam($$, struct, $1, $4); SetParent( $$, $4 ); }
	| keyname '=' '[' struct_list ']' { $4->name = $1; $$ = $4; }
	;

struct_list:
	'{'	param_list '}'							{
			OptDef	*str;
			NameAtom	*idx;

			MakeAtom(idx, NULL);
			MakeScalarParam(str, struct, idx, $2); 
			SetParent( str, $2 );
			SetIndex( str, 0 );
			MakeScalarParam($$, array, NULL, str);
			SetParent( $$, str );
		}
	| struct_list ',' '{' param_list '}' {
			OptDef	*str;
			NameAtom	*idx;

			MakeAtom(idx, NULL);
			MakeScalarParam(str, struct, idx, $4); 
			SetParent(str, $4);
			SetIndex(str, $1->paramValue.arrayval->name->index + 1);
			MakeList($1->paramValue.arrayval, str, $1->paramValue.arrayval); 
			SetParent($1, str);
			$$ = $1;
		}
	;

%%

static int
prscfg_yyerror(prscfg_yyscan_t yyscanner, char *msg) {
	out_warning("gram_yyerror: %s at line %d\n", msg, prscfgGetLineNo(yyscanner));
	return 0;
}

static NameAtom*
cloneName(NameAtom *list, NameAtom **end) {
	NameAtom	*newList = NULL, *ptr, *endptr = NULL;

	while(list) {
		ptr = *end = malloc(sizeof(*ptr));
		if (!ptr) {
			out_warning("No memory");
			return NULL;
		}
		*ptr = *list;
		ptr->name = strdup(ptr->name);
		if (!ptr->name) {
			out_warning("No memory");
			free(ptr);
			return NULL;
		}

		if (newList) {
			endptr->next = ptr;
			endptr = ptr;
		} else {
			newList = endptr = ptr;
		}

		list = list->next;
	}

	return newList;
}

static void
freeName(NameAtom *atom) {
	NameAtom	*p;
	
	while(atom) {
		free(atom->name);
		p = atom->next;
		free(atom);
		atom = p;
	}
}

static int
compileName(OptDef	*def) {
	NameAtom	*beginPtr = NULL, *endPtr, *list;
	OptDef	*c = def;
	int		index = -1;

	list = NULL;

	while(c) {
		if (c->name->name) {
			beginPtr = cloneName(c->name, &endPtr);
			if (!beginPtr)
				return 1;

			if (index >= 0) {
				beginPtr->index = index;
				index = -1;
			}

			endPtr->next = list;
			list = beginPtr;
		} else {
			index = c->name->index;
		}

		c = c->parent;
	}

	def->name = list;

	return 0;
}

static OptDef*
plainOptDef(OptDef *def, OptDef *list) {
	OptDef	*ptr;

	while(def) {
		switch(def->paramType) {
			case numberType:
			case stringType:
				ptr = malloc(sizeof(*ptr));
				if (!ptr) {
					out_warning("No memory");
					freeCfgDef(def);
					freeCfgDef(list);
					return NULL;
				}
				*ptr = *def;
				if (compileName(ptr)) {
					freeName(ptr->name);
					free(ptr);
					freeCfgDef(def);
					freeCfgDef(list);
					return NULL;
				}
				ptr->parent = NULL;
				ptr->next = list;
				list = ptr;
				break;
			case structType:
				list = plainOptDef(def->paramValue.structval, list);
				break;
			case arrayType:
				list = plainOptDef(def->paramValue.arrayval, list);
				break;
			default:
				out_warning("Unkown paramType: %d", def->paramType);
		}

		ptr = def->next;
		freeName(def->name);
		free(def);
		def = ptr;
	}

	return list;
}

void
freeCfgDef(OptDef *def) {
	OptDef	*ptr;

	while(def) {
		switch(def->paramType) {
			case numberType:
				free(def->paramValue.numberval);
				break;
			case stringType:
				free(def->paramValue.stringval);
				break;
			case structType:
				freeCfgDef(def->paramValue.structval);
				break;
			case arrayType:
				freeCfgDef(def->paramValue.arrayval);
				break;
			default:
				break;
		}

		ptr = def->next;
		freeName(def->name);
		free(def);
		def = ptr;
	}
}

OptDef*
parseCfgDef(FILE *fh) {
	prscfg_yyscan_t			yyscanner;
	prscfg_yy_extra_type	yyextra;
	int						yyresult;

	yyscanner = prscfgScannerInit(fh, &yyextra);

	output = NULL;
	yyresult = prscfg_yyparse(yyscanner);
	prscfgScannerFinish(yyscanner);

	if (yyresult != 0) 
		return NULL;

	return plainOptDef(output, NULL);
}

OptDef*
parseCfgDefBuffer(char *buffer) {
	prscfg_yyscan_t			yyscanner;
	prscfg_yy_extra_type	yyextra;
	int						yyresult;

	yyscanner = prscfgScannerInitBuffer(buffer, &yyextra);

	output = NULL;
	yyresult = prscfg_yyparse(yyscanner);
	prscfgScannerFinish(yyscanner);

	if (yyresult != 0) 
		return NULL;

	return plainOptDef(output, NULL);
}


