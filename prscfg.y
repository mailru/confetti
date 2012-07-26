%{

#undef yylval
#undef yylloc

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>

static int prscfg_yyerror(prscfg_yyscan_t yyscanner, const char *msg);
extern int prscfg_yylex (YYSTYPE * yylval_param, prscfg_yyscan_t yyscanner);
static NameAtom* prependName(NameAtom *prep, NameAtom *name);
static void freeName(NameAtom *atom);
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

#define MakeScalarParam(r, t, n, v, p)	do {  	\
	(r) = malloc(sizeof(OptDef));				\
	if (!(r)) {									\
		prscfg_yyerror(yyscanner, "No memory");	\
		YYERROR;								\
	}											\
	(r)->paramType = t##Type;					\
	(r)->optional = p;							\
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

#define SetSection(out, in, sec)	do {			\
	OptDef	*opt;									\
	opt = (out) = (in); 							\
													\
	while(opt) {									\
		opt->name = prependName((sec), opt->name);	\
													\
		opt = opt->next;							\
	}												\
	freeName(sec);									\
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
	OptDef		*node;
	NameAtom	*atom;
	int			flag;
}

%type	<node>		cfg section_list section named_section

%type	<node>		param param_list struct_list

%type	<atom>		identifier elem_identifier keyname array_keyname
%type	<atom>		section_name

%type	<flag>		opt;

%type   <str>		comma_opt

%token	<str>		NULL_P OPT_P KEY_P NATURAL_P STRING_P

%%

cfg:
	section_list 	{ output = $$ = $1; }
	;

section_list:
	section					{ $$ = $1; }
	| section_list named_section	{ MakeList($$, $2, $1); }
	;
	
section:
	/* empty */		{ $$ = NULL; }
	| param_list		{ $$ = $1; }

named_section:
	'[' section_name ']' section	{ SetSection($$, $4, $2); }
	;
	
section_name:
	keyname			{ $$ = $1; }
	| array_keyname	{ $$ = $1; }

param_list:
	param							{ $$ = $1; }
	| param_list comma_opt param	{ MakeList($$, $3, $1); /* plainOptDef will revert the list */ }
	;

identifier:
	KEY_P			{ MakeAtom($$, $1); }
	| NULL_P		{ MakeAtom($$, $1); }
	;

elem_identifier:
	identifier						{ $$ = $1; }
	| identifier '[' NATURAL_P ']'	{ 
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

array_keyname:
	identifier '[' NATURAL_P ']'		{ 
			$$ = $1;
			$$->index = atoi($3);
			/* XXX check !*/
			free($3);
		}
	| elem_identifier '.' array_keyname { MakeList($$, $1, $3); }
	;

param:
	opt keyname '=' NULL_P								{ MakeScalarParam($$, scalar, $2, NULL, $1); free($4); }
	| opt keyname '=' OPT_P								{ MakeScalarParam($$, scalar, $2, $4, $1); }
	| opt keyname '=' KEY_P								{ MakeScalarParam($$, scalar, $2, $4, $1); }
	| opt keyname '=' NATURAL_P							{ MakeScalarParam($$, scalar, $2, $4, $1); }
	| opt keyname '=' STRING_P							{ MakeScalarParam($$, scalar, $2, $4, $1); }
	| opt keyname '=' '{' param_list comma_opt '}'		{ MakeScalarParam($$, struct, $2, $5, $1); SetParent( $$, $5 ); }
	| opt keyname '=' '[' struct_list comma_opt ']' 	{ $5->name = $2; $5->optional = $1; $$ = $5; }
	| opt keyname '=' '[' ']' 							{ MakeScalarParam($$, array, $2, NULL, $1); }
	| opt array_keyname '=' '{' param_list comma_opt '}' { MakeScalarParam($$, struct, $2, $5, $1); SetParent( $$, $5 ); }
	;
	
opt: /* empty */	{ $$ = 0; }
	| OPT_P			{ $$ = 1; free($1); }

comma_opt:
	','				{ $$=NULL; }
	| /* EMPTY */	{ $$=NULL; }
	;

struct_list:
	'{' param_list comma_opt '}' {
			OptDef		*str;
			NameAtom	*idx;

			MakeAtom(idx, NULL);
			MakeScalarParam(str, struct, idx, $2, 0); 
			SetParent( str, $2 );
			SetIndex( str, 0 );
			MakeScalarParam($$, array, NULL, str, 0);
			SetParent( $$, str );
		}
	| struct_list comma_opt '{' param_list comma_opt '}' {
			OptDef		*str;
			NameAtom	*idx;

			MakeAtom(idx, NULL);
			MakeScalarParam(str, struct, idx, $4, 0);
			SetParent(str, $4);
			SetIndex(str, $1->paramValue.arrayval->name->index + 1);
			MakeList($1->paramValue.arrayval, str, $1->paramValue.arrayval); 
			SetParent($1, str);
			$$ = $1;
		}
	;

%%

static int
prscfg_yyerror(prscfg_yyscan_t yyscanner, const char *msg) {
	out_warning(CNF_SYNTAXERROR, "gram_yyerror: %s at line %d", msg, prscfgGetLineNo(yyscanner));
	return 0;
}

static NameAtom*
cloneName(NameAtom *list, NameAtom **end) {
	NameAtom	*newList = NULL, *ptr, *endptr = NULL;

	while(list) {
		ptr = *end = malloc(sizeof(*ptr));
		if (!ptr) {
			out_warning(CNF_NOMEMORY, "No memory");
			return NULL;
		}
		*ptr = *list;
		if (ptr->name) {
			ptr->name = strdup(ptr->name);
			if (!ptr->name) {
				out_warning(CNF_NOMEMORY, "No memory");
				free(ptr);
				return NULL;
			}
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

static NameAtom* 
prependName(NameAtom *prep, NameAtom *name) {
	NameAtom	*b, *e;

	b = cloneName(prep, &e);

	if (!b) {
		out_warning(CNF_NOMEMORY, "No memory");
		return NULL;
	}

	e->next = name;

	return b;
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
				endPtr->index = index;
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
			case scalarType:
				ptr = malloc(sizeof(*ptr));
				if (!ptr) {
					out_warning(CNF_NOMEMORY, "No memory");
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
				if (def->paramValue.arrayval == NULL) {
					ptr = malloc(sizeof(*ptr));
					if (!ptr) {
						out_warning(CNF_NOMEMORY, "No memory");
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
				} else {
					list = plainOptDef(def->paramValue.arrayval, list);
				}
				break;
			default:
				out_warning(CNF_INTERNALERROR, "Unkown paramType: %d", def->paramType);
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
			case scalarType:
				free(def->paramValue.scalarval);
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


