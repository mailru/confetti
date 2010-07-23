%{

#undef yylval
#undef yylloc

#include <stdlib.h>
#include <string.h>

#define YY_EXTRA_TYPE prscfg_yy_extra_type *

static int scan_yyerror(char *s);
static void addstring(prscfg_yyscan_t yyscanner, char *s, int l);
static void addchar(prscfg_yyscan_t yyscanner, char s);

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
%x SCOMMENT
%x CCOMMENT

KEYCHARSTART	[a-zA-Z_]
KEYCHAR			[a-zA-Z0-9_]
DIGIT			[0-9]

%%

<INITIAL>[Nn][Uu][Ll][Ll]	{
			yylval->str = strdup("NULL");
			return NULL_P;
		}

<INITIAL>\/\* {
			yyextra->commentCounter=1;
			BEGIN CCOMMENT;
		}

<INITIAL>{KEYCHARSTART}{KEYCHAR}*	{
			yylval->str = strndup(yytext, yyleng);
			return	KEY_P;
		}

<INITIAL>#[ \t]* {
			BEGIN SCOMMENT;
		}

<INITIAL>[\=\[\]\{\}\.] { return *yytext; }

<INITIAL>\"	{
			yyextra->total = 256;
			yyextra->strbuf = malloc(yyextra->total);
			yyextra->length = 0;
			BEGIN xQUOTED;
	}

<INITIAL>[\-\+]?{DIGIT}+ {
			yylval->str = strndup(yytext, yyleng);
			return NUMBER_P;
		}

<INITIAL>[\+\-]?{DIGIT}+"."{DIGIT}+ {
			yylval->str = strndup(yytext, yyleng);
			return NUMBER_P;
		}

<INITIAL>[\+\-]?{DIGIT}+[eE][\+\-]?{DIGIT}+ {
			yylval->str = strndup(yytext, yyleng);
			return NUMBER_P;
		}

<INITIAL>[\+\-]?{DIGIT}+"."{DIGIT}+[eE][\+\-]?{DIGIT}+ {
			yylval->str = strndup(yytext, yyleng);
			return NUMBER_P;
		}

<xQUOTED>\\(.|\n)   {
			addchar(yyscanner, yytext[1]);
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
			return scan_yyerror("Unexpected end of string");
		}

<xQUOTED>[^\\\"]+   {
			addstring(yyscanner, yytext, yyleng);
		}

<CCOMMENT>\/\*  {
			yyextra->commentCounter++;
		}

<CCOMMENT>\*\/  {
			yyextra->commentCounter--;
			if (yyextra->commentCounter == 0)
				BEGIN INITIAL;
		}

<CCOMMENT><<EOF>> {
			return scan_yyerror("Unexpected end of string (inside comment)");
		}

<CCOMMENT>(.|\n) { /* ignore */ }

<SCOMMENT>.+    		{ /* ignore */ }

<SCOMMENT>[\r\n]+    	{ BEGIN INITIAL; }

<INITIAL,SCOMMENT><<EOF>> {
			yyterminate();
		}

<INITIAL>[ \t\r\n\f]+   { /* ignore */ }

<INITIAL>. { return scan_yyerror("syntax error: Unknown character"); }

%%

static int
scan_yyerror(char *msg) {
    fprintf(stderr, "scan_yyerror: %s", msg);
	return 0;
}

prscfg_yyscan_t
prscfgScannerInit(FILE *fh, prscfg_yy_extra_type *yyext) {
	yyscan_t	scanner;

	memset(yyext, 0, sizeof(*yyext));
	yylex_init_extra(yyext, &scanner);

	buf = yy_create_buffer( fh, YY_BUF_SIZE, scanner );
	yy_switch_to_buffer( buf, scanner );

	return scanner;
}

void
prscfgScannerFinish(prscfg_yyscan_t scanner) {
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

static void
addstring(prscfg_yyscan_t yyscanner, char *s, int l) {
    while( yyextra->length + l + 1 >= yyextra->total ) {
		yyextra->total *= 2;
		yyextra->strbuf=realloc(yyextra->strbuf, yyextra->total);
	}

	memcpy( yyextra->strbuf+yyextra->length, s, l);
	yyextra->length+=l;
}

static void
addchar(prscfg_yyscan_t yyscanner, char s) {
	if(  yyextra->length + 2 >= yyextra->total ) {
		yyextra->total*=2;
		yyextra->strbuf=realloc(yyextra->strbuf, yyextra->total);
	}

	yyextra->strbuf[ yyextra->length++ ] = s;
}
																


