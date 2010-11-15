%{

#include <stdlib.h>
#include <string.h>
#include <prscfl.h>
#include <prscfl_gram.h>

#define YY_EXTRA_TYPE prscfl_yy_extra_type *

static int scan_yyerror(char *s, int lineno);
static void addstring(prscfl_yyscan_t yyscanner, char *s, int l);
static void addchar(prscfl_yyscan_t yyscanner, char s);

static char * strdupn(char *src, size_t size);
static YY_BUFFER_STATE buf = NULL;

%}


%option 8bit
%option never-interactive
%option nodefault
%option noyywrap
%option nounput
%option prefix="prscfl_yy"
%option reentrant
%option noinput
%option bison-bridge
%option warn

%x xQUOTED
%x COMMENT
%x BUILTIN

KEYCHARSTART	[a-zA-Z_]
KEYCHAR			[a-zA-Z0-9_]
DIGIT			[0-9]

%%

<INITIAL>[Nn][Uu][Ll][Ll]	{
			yylval->str = strdup("NULL");
			return NULL_P;
		}

<INITIAL>[Rr][Oo]	{
			yylval->str = strdupn(yytext, yyleng);
			return RDONLY_P;
		}

<INITIAL>[Rr][Ww]	{
			yylval->str = strdupn(yytext, yyleng);
			return RDWR_P;
		}

<INITIAL>[Rr][Ee][Qq][Uu][Ii][Rr][Ee][Dd]	{
			yylval->str = strdupn(yytext, yyleng);
			return REQUIRED_P;
		}

<INITIAL>{KEYCHARSTART}{KEYCHAR}*	{
			yylval->str = strdupn(yytext, yyleng);
			return	KEY_P;
		}

<INITIAL>#[ \t]* {
			BEGIN COMMENT;
			yyextra->total = 256;
			yyextra->strbuf = malloc(yyextra->total);
			yyextra->length = 0;
		}

<INITIAL>[\=\[\]\{\}\,] { return *yytext; }

<INITIAL>\%\{			{ 
			yyextra->total = 256;
			yyextra->strbuf = malloc(yyextra->total);
			yyextra->length = 0;
			BEGIN BUILTIN;
	}

<INITIAL>\"	{
			yyextra->total = 256;
			yyextra->strbuf = malloc(yyextra->total);
			yyextra->length = 0;
			BEGIN xQUOTED;
	}

<INITIAL>[\-\+]?{DIGIT}+L {
			yylval->int64val = strtoll(yytext, NULL, 10);
			return INT64_P;
		}

<INITIAL>{DIGIT}+UL {
			yylval->uint64val = strtoull(yytext, NULL, 10);
			return UINT64_P;
		}

<INITIAL>{DIGIT}+U {
			yylval->uint32val = strtoul(yytext, NULL, 10);
			return UINT32_P;
		}

<INITIAL>[\-\+]?{DIGIT}+ {
			yylval->int32val = strtol(yytext, NULL, 10);
			return INT32_P;
		}

<INITIAL>[\-\+]?{DIGIT}+"."{DIGIT}+ {
        	yylval->doubleval = strtod(yytext, NULL);
			return DOUBLE_P;
		}

<INITIAL>[\-\+]?{DIGIT}+[eE][\+\-]?{DIGIT}+ {
        	yylval->doubleval = strtod(yytext, NULL);
			return DOUBLE_P;
		}

<INITIAL>[\-\+]?{DIGIT}+"."{DIGIT}+[eE][\+\-]?{DIGIT}+ {
        	yylval->doubleval = strtod(yytext, NULL);
			return DOUBLE_P;
		}

<xQUOTED>\\.  {
			addchar(yyscanner, yytext[1]);
		}

<xQUOTED>\\\n  {
			yyextra->lineno++;
		}

<xQUOTED>\"			{
			yyextra->strbuf[yyextra->length] = '\0';
			yylval->str = yyextra->strbuf;
			BEGIN INITIAL;
			yyextra->strbuf = NULL;
			return STRING_P;
		}

<xQUOTED>\\ {
			/* This is only needed for \ just before EOF */
		}

<xQUOTED><<EOF>> {
			return scan_yyerror("Unexpected end of string", yyextra->lineno);
		}

<xQUOTED>[^\\\"\n]+   {
			addstring(yyscanner, yytext, yyleng);
		}

<xQUOTED>\n   {
			addchar(yyscanner, yytext[0]);
			yyextra->lineno++;
		}

<COMMENT>\r			{ /* ignore */ }

<COMMENT>.+    		{ 
			addstring(yyscanner, yytext, yyleng);
		}

<COMMENT>\n    	{ 
			yyextra->strbuf[yyextra->length] = '\0';
			yylval->str = yyextra->strbuf;
			BEGIN INITIAL;
			yyextra->strbuf = NULL;
			yyextra->lineno++;
			return COMMENT_P;
		}

<BUILTIN>\n		{
			yyextra->lineno++;
			addchar(yyscanner, yytext[0]);
		}

<BUILTIN>\%\}	{
			yyextra->strbuf[yyextra->length] = '\0';
			yylval->str = yyextra->strbuf;
			BEGIN INITIAL;
			yyextra->strbuf = NULL;
			return BUILTIN_P;
		}

<BUILTIN>.		{
			addchar(yyscanner, yytext[0]);
		}

<BUILTIN><<EOF>> { return scan_yyerror("Unexpected end of file", yyextra->lineno); }

<INITIAL,COMMENT><<EOF>> {
			yyterminate();
		}

<INITIAL>[ \t\r\f]+   { /* ignore */ }

<INITIAL>\n   { yyextra->lineno++; }

<INITIAL>. { return scan_yyerror("syntax error: Unknown character", yyextra->lineno); }

%%

static int
scan_yyerror(char *msg, int lineno) {
    fprintf(stderr, "scan_yyerror: %s at line %d\n", msg, lineno);
	return 0;
}

prscfl_yyscan_t
prscflScannerInit(FILE *fh, prscfl_yy_extra_type *yyext) {
	yyscan_t	scanner;

	memset(yyext, 0, sizeof(*yyext));
	yyext->lineno = 1;
	yylex_init_extra(yyext, &scanner);

	buf = yy_create_buffer( fh, YY_BUF_SIZE, scanner );
	yy_switch_to_buffer( buf, scanner );

	return scanner;
}

void
prscflScannerFinish(prscfl_yyscan_t scanner) {
	if (buf)
		yy_delete_buffer( buf, scanner );
	buf = NULL;
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
prscflGetLineNo(prscfl_yyscan_t yyscanner) {
	return yyextra->lineno;
}

static void
addstring(prscfl_yyscan_t yyscanner, char *s, int l) {
    while( yyextra->length + l + 1 >= yyextra->total ) {
		yyextra->total *= 2;
		yyextra->strbuf=realloc(yyextra->strbuf, yyextra->total);
	}

	memcpy( yyextra->strbuf+yyextra->length, s, l);
	yyextra->length+=l;
}

static void
addchar(prscfl_yyscan_t yyscanner, char s) {
	if(  yyextra->length + 2 >= yyextra->total ) {
		yyextra->total*=2;
		yyextra->strbuf=realloc(yyextra->strbuf, yyextra->total);
	}

	yyextra->strbuf[ yyextra->length++ ] = s;
}
																
static char *
strdupn(char *src, size_t size) {
	char	*dst = malloc(size + 1);

	if (!dst)
		return NULL;
	
	memcpy(dst, src, size);
	dst[size] = '\0';

	return dst;
}


