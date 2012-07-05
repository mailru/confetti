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
%option nounput
%option prefix="prscfg_yy"
%option reentrant
%option noinput
%option bison-bridge
%option warn

%x xQUOTED
%x CCOMMENT

KEYCHARSTART	[a-zA-Z_]
KEYCHAR			[a-zA-Z0-9_]
DIGIT			[0-9]

/* Chars that may comprise a non-quoted string. */
STRINGCHAR		[^ \t\f\r\n\[\]{}\"=,.]

/* Chars that may comprise a non-quoted path string. A path is identified
   by the presence of a slash and it allows dots in addition to regular
   non-quoted string chars. */
PATHCHAR		[^ \t\f\r\n\[\]{}\"=,]

%%

<INITIAL>[Nn][Uu][Ll][Ll]	{
			yylval->str = strdup("NULL");
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);	
			return NULL_P;
		}

<INITIAL>[Oo][Pp][Tt] {
			yylval->str = strdup("OPT");
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);	
			return OPT_P;
		}

<INITIAL>{KEYCHARSTART}{KEYCHAR}*	{
			yylval->str = strdupn(yytext, yyleng);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);	
			return KEY_P;
		}

<INITIAL>{DIGIT}+ {
			yylval->str = strdupn(yytext, yyleng);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);	
			return NATURAL_P;
		}

<INITIAL>\"	{
			yyextra->total = 256;
			yyextra->strbuf = malloc(yyextra->total);
			if (!yyextra->strbuf)
				scan_yyerror("No memory", yyextra->lineno);	
			yyextra->length = 0;
			BEGIN xQUOTED;
	}

<INITIAL>\/\* {
			yyextra->commentCounter=1;
			BEGIN CCOMMENT;
		}

<INITIAL>{PATHCHAR}*(\/{PATHCHAR}*)+ {
			/* Accept an unquoted path-like string. */
			yylval->str = strdupn(yytext, yyleng);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);
			return STRING_P;
		}

<INITIAL>[+-]?{DIGIT}+"."{STRINGCHAR}+ {
			/* Accept an unquoted fpnum-like string. */
			yylval->str = strdupn(yytext, yyleng);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);
			return STRING_P;
		}

<INITIAL>{STRINGCHAR}+ {
			/* Accept an unquoted string. */
			yylval->str = strdupn(yytext, yyleng);
			if (!yylval->str)
				scan_yyerror("No memory", yyextra->lineno);
			return STRING_P;
}

<INITIAL>[\=\[\]\{\}\.\,] { return *yytext; }

<INITIAL>#[^\n]*		{ /* ignore single-line comment */ }

<INITIAL>[ \t\r\f]+   	{ /* ignore whitespace */ }

<INITIAL>\n   			{ yyextra->lineno++; }

<INITIAL><<EOF>> {
			yyterminate();
		}

<INITIAL>. { return scan_yyerror("syntax error: Unknown character", yyextra->lineno); }

<xQUOTED>\\.  {
			if (addchar(yyscanner, yytext[1]))
				scan_yyerror("No memory", yyextra->lineno);
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

<xQUOTED>[^\\\"\n]+   {
			if (addstring(yyscanner, yytext, yyleng))
				scan_yyerror("No memory", yyextra->lineno);
		}

<xQUOTED>\n   {
			if (addchar(yyscanner, yytext[0]))
				scan_yyerror("No memory", yyextra->lineno);
			yyextra->lineno++;
		}

<xQUOTED><<EOF>> {
			return scan_yyerror("Unexpected end of string", yyextra->lineno);
		}

<CCOMMENT>\/\*  {
			yyextra->commentCounter++;
		}

<CCOMMENT>\*\/  {
			yyextra->commentCounter--;
			if (yyextra->commentCounter == 0)
				BEGIN INITIAL;
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


