%{

#undef yylval
#undef yylloc

#include <stdlib.h>
#include <string.h>

#define YY_EXTRA_TYPE prscfg_yy_extra_type *

static int scan_yyerror(char *s, int lineno);
static int addstring(prscfg_yyscan_t yyscanner, char *s, int l);
static int addchar(prscfg_yyscan_t yyscanner, char s);
static char * strdupn(char *src, size_t size);

static YY_BUFFER_STATE buf = NULL;

%}


%option 8bit
%option never-interactive
%option nodefault
%option noyywrap
%option prefix="prscfg_yy"
%option reentrant
%option noinput
%option noyy_top_state
%option bison-bridge
%option stack
%option warn

%x VALUE
%x QUOTED
%x UNQUOTED
%x CCOMMENT

KEYCHARSTART	[a-zA-Z_]
KEYCHAR			[a-zA-Z0-9_]
DIGIT			[0-9]
WS				[ \t\f\r]

/* Default separators */
DEFSEP			[\[\]{},.=]

/* Value separators */
VALSEP			[\[\]{},]

%%

<INITIAL,VALUE>\n		{ yyextra->lineno++; }

<INITIAL,VALUE>{WS}+	{ /* ignore whitespace */ }

<INITIAL,VALUE>#[^\n]*	{ /* ignore single-line comment */ }

<INITIAL,VALUE>\/\*		{
			/* scan a C-style comment */
			yyextra->commentCounter = 1;
			yyextra->ostate = YYSTATE;
			BEGIN CCOMMENT;
		}

<INITIAL>[Oo][Pp][Tt]	{
			yylval->str = strdup("OPT");
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);	
			fprintf(stderr, "opt: %s\n", yylval->str);
			return OPT_P;
		}

<INITIAL>{KEYCHARSTART}{KEYCHAR}*	{
			yylval->str = strdupn(yytext, yyleng);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);	
			fprintf(stderr, "key: %s\n", yylval->str);
			return KEY_P;
		}

<INITIAL>{DIGIT}+		{
			yylval->str = strdupn(yytext, yyleng);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);	
			fprintf(stderr, "index: %s\n", yylval->str);
			return INDEX_P;
		}

<INITIAL>{DEFSEP}		{ return *yytext; }

<INITIAL>. 				{ return scan_yyerror("syntax error: Unknown character", yyextra->lineno); }

<INITIAL><<EOF>> 		{ yyterminate(); }

<VALUE>[Nn][Uu][Ll][Ll]	{
			yylval->str = strdup("NULL");
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);
			fprintf(stderr, "null: %s\n", yylval->str);
			BEGIN INITIAL;
			return NULL_P;
		}

<VALUE>\"				{
			/* Accept a quoted string. */
			yyextra->total = 256;
			yyextra->strbuf = malloc(yyextra->total);
			if (!yyextra->strbuf)
				scan_yyerror("No memory", yyextra->lineno);	
			yyextra->length = 0;
			BEGIN QUOTED;
		}

<VALUE>{VALSEP}			{
			BEGIN INITIAL;
			return *yytext;
		}

<VALUE>. 				{
			/* Accept an unquoted string. */
			yymore();
			BEGIN UNQUOTED;
		}

<VALUE><<EOF>> 			{
			return scan_yyerror("Unexpected end of string (expecting value)", yyextra->lineno);
		}

<UNQUOTED>#				|
<UNQUOTED>\n			|
<UNQUOTED>{WS}			|
<UNQUOTED>{VALSEP}		{
			int next = yytext[yyleng - 1];
			yylval->str = strdupn(yytext, yyleng - 1);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);
			fprintf(stderr, "unquoted: %s\n", yylval->str);
			unput(next);
			BEGIN INITIAL;
			return STRING_P;
		}

<UNQUOTED>\/\*			{
			yylval->str = strdupn(yytext, yyleng - 2);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);
			fprintf(stderr, "unquoted: %s\n", yylval->str);
			unput('*');
			unput('/');
			BEGIN INITIAL;
			return STRING_P;
		}

<UNQUOTED>. 			{ yymore(); }

<UNQUOTED><<EOF>>		{
			yylval->str = strdupn(yytext, yyleng - 1);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);
			fprintf(stderr, "unquoted: %s\n", yylval->str);
			BEGIN INITIAL;
			return STRING_P;
		}

<QUOTED>\\.  {
			if (addchar(yyscanner, yytext[1]))
				scan_yyerror("No memory", yyextra->lineno);
		}

<QUOTED>\\\n  {
			yyextra->lineno++;
		}

<QUOTED>\"			{
			yyextra->strbuf[yyextra->length] = '\0';
			yylval->str = yyextra->strbuf;
			fprintf(stderr, "quoted: %s\n", yylval->str);
			BEGIN INITIAL;
			yyextra->strbuf = NULL;
			return STRING_P;
		}

<QUOTED>\\ {
			/* This is only needed for \ just before EOF */
		}

<QUOTED>[^\\\"\n]+   {
			if (addstring(yyscanner, yytext, yyleng))
				scan_yyerror("No memory", yyextra->lineno);
		}

<QUOTED>\n   {
			if (addchar(yyscanner, yytext[0]))
				scan_yyerror("No memory", yyextra->lineno);
			yyextra->lineno++;
		}

<QUOTED><<EOF>> {
			return scan_yyerror("Unexpected end of string (expecting closing quote)", yyextra->lineno);
		}

<CCOMMENT>\/\*  {
			yyextra->commentCounter++;
		}

<CCOMMENT>\*\/  {
			yyextra->commentCounter--;
			if (yyextra->commentCounter == 0)
				BEGIN yyextra->ostate;
		}

<CCOMMENT>\n 	{ yyextra->lineno++; }

<CCOMMENT>.+ 	{ /* ignore */ }

<CCOMMENT><<EOF>> {
			return scan_yyerror("Unexpected end of string (inside comment)", yyextra->lineno);
		}

%%

static int
scan_yyerror(char *msg, int lineno) {
    out_warning(CNF_SYNTAXERROR, "scan_yyerror: %s at line %d", msg, lineno);
	return 0;
}

prscfg_yyscan_t
prscfgScannerInit(FILE *fh, prscfg_yy_extra_type *yyext) {
	yyscan_t	scanner;

	memset(yyext, 0, sizeof(*yyext));
	yyext->lineno = 1;
	yylex_init_extra(yyext, &scanner);

	buf = yy_create_buffer( fh, YY_BUF_SIZE, scanner );
	yy_switch_to_buffer( buf, scanner );

	return scanner;
}

prscfg_yyscan_t
prscfgScannerInitBuffer(char *buffer, prscfg_yy_extra_type *yyext) {
	yyscan_t	scanner;

	memset(yyext, 0, sizeof(*yyext));
	yyext->lineno = 1;
	yylex_init_extra(yyext, &scanner);

	buf = yy_scan_string( buffer, scanner );
	yy_switch_to_buffer( buf, scanner );

	return scanner;
}

void
prscfgScannerFinish(prscfg_yyscan_t scanner) {
	if (buf)
		yy_delete_buffer( buf, scanner );
	yylex_destroy(scanner);
	buf = NULL;
}

void
prscfgScannerStartValue(prscfg_yyscan_t scanner)
{
fprintf(stderr, "start val\n");
	yy_push_state(VALUE, scanner);
}

static void
prscfgScannerEndValue(prscfg_yyscan_t scanner)
{
fprintf(stderr, "end val\n");
	yy_pop_state(scanner);
}

/*
 * Arrange access to yyextra for subroutines of the main yylex() function.
 * We expect each subroutine to have a yyscanner parameter.  Rather than
 * use the yyget_xxx functions, which might or might not get inlined by the
 * compiler, we cheat just a bit and cast yyscanner to the right type.
 */
#undef yyextra
#define yyextra  (((struct yyguts_t *) yyscanner)->yyextra_r)

int
prscfgGetLineNo(prscfg_yyscan_t yyscanner) {
	return yyextra->lineno;
}

static int
addstring(prscfg_yyscan_t yyscanner, char *s, int l) {
    while( yyextra->length + l + 1 >= yyextra->total ) {
		yyextra->total *= 2;
		yyextra->strbuf=realloc(yyextra->strbuf, yyextra->total);
	}
	if (!yyextra->strbuf)
		return 1;	

	memcpy( yyextra->strbuf+yyextra->length, s, l);
	yyextra->length+=l;
	return 0;
}

static int
addchar(prscfg_yyscan_t yyscanner, char s) {
	if(  yyextra->length + 2 >= yyextra->total ) {
		yyextra->total*=2;
		yyextra->strbuf=realloc(yyextra->strbuf, yyextra->total);
	}
	if (!yyextra->strbuf)
		return 1;
	yyextra->strbuf[ yyextra->length++ ] = s;
	return 0;
}

static char *
strdupn(char *src, size_t size) {
        char    *dst = malloc(size + 1);

	if (!dst)
		return NULL;

	memcpy(dst, src, size);
	dst[size] = '\0';

	return dst;
}


