#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include <prscfl.h>

static void
dumpParamDefCName(FILE *fh, ParamDef *def) {
	fputs("_name", fh);
	dumpStructName(fh, def, "__");
	fputc('_', fh);
	fputc('_', fh);
	fputs(def->name, fh);
}

static int
dumpParamDefNameList(FILE *fh, ParamDef *def, ParamDef *odef, int level) {
    if (def && def->name) {
			int maxlevel = dumpParamDefNameList(fh, def->parent, odef, level + 1);

			fprintf(fh,"\t{ \"%s\", -1, ", def->name);

			if (level == 0) {
				fputs("NULL }\n", fh);
			} else {
				dumpParamDefCName(fh, odef);
				fprintf(fh, " + %d },\n", maxlevel - level);
			}

			return maxlevel;
	}

	return level;
}

static void
dumpParamDefCNameRecursive(FILE *fh, ParamDef *def) {

	while(def) {

		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	stringType:
				fputs("static NameAtom ", fh);
				dumpParamDefCName(fh, def);
				fputs("[] = {\n", fh);
				dumpParamDefNameList(fh, def, def, 0);
				fputs("};\n", fh);
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				dumpParamDefCNameRecursive(fh, def->paramValue.structval);
				break;
			case	arrayType:
				dumpParamDefCNameRecursive(fh, def->paramValue.arrayval);
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

static void
dumpStructPath(FILE *fh, ParamDef *def) {
	fputs("\tc", fh);
	dumpStructName(fh, def, "->");
	fputs("->", fh);
	fputs(def->name, fh);
}

static void
dumpDefault(FILE *fh, ParamDef *def) {

	while(def) {

		switch(def->paramType) {
			case	int32Type:
				dumpStructPath(fh, def);
				fprintf(fh, " = %"PRId32";\n", def->paramValue.int32val);
				break;
			case	uint32Type:
				dumpStructPath(fh, def);
				fprintf(fh, " = %"PRIu32"U;\n", def->paramValue.uint32val);
				break;
			case	int64Type:
				dumpStructPath(fh, def);
				fprintf(fh, " = %"PRId64"LL;\n", def->paramValue.int64val);
				break;
			case	uint64Type:
				dumpStructPath(fh, def);
				fprintf(fh, " = %"PRIu64"ULL;\n", def->paramValue.uint64val);
				break;
			case	doubleType:
				dumpStructPath(fh, def);
				fprintf(fh, " = %g;\n", def->paramValue.doubleval);
				break;
			case	stringType:
				dumpStructPath(fh, def);
				if (def->paramValue.stringval == NULL) {
					fputs(" = NULL;\n", fh);
				} else {
					char *ptr = def->paramValue.stringval;
					fputs(" = strdup(\"", fh);

					while(*ptr) {
						if (*ptr == '"')
							fputc('\\', fh);
						fputc(*ptr, fh);
						ptr++;
					}
					fputs("\");\n", fh);
				}
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				dumpStructPath(fh, def);
				fputs(" = malloc(sizeof( *(", fh);
				dumpStructPath(fh, def);
				fputs(") ));\n", fh);
				dumpDefault(fh, def->paramValue.structval);
				break;
			case	arrayType:
				dumpStructPath(fh, def);
				fprintf(fh, " = NULL;\n");
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

static void
dumpDefaultArray(FILE *fh, char *name, ParamDef *def) {
	while(def) {
		if (def->paramType == arrayType) {
			ParamDef *ptr;
			
			fputs("static void\nacceptDefault", fh);
			dumpParamDefCName(fh, def);
			fputs("(", fh);
			fputs(name, fh);
			dumpStructName(fh, def->paramValue.arrayval, "_");
			fputs(" *c) {\n",fh);

			/* reset parent */
			ptr = def->paramValue.arrayval;
			while(ptr) {
				ptr->parent = NULL;
				ptr = ptr->next;
			}

			dumpDefault(fh, def->paramValue.arrayval);

			/* restor parent */
			ptr = def->paramValue.arrayval;
			while(ptr) {
				ptr->parent = def;
				ptr = ptr->next;
			}

			fputs("}\n\n", fh);

			dumpDefaultArray(fh, name, def->paramValue.arrayval);
		} else if (def->paramType == structType) {
			dumpDefaultArray(fh, name, def->paramValue.structval);
		}

		def = def->next;
	}
}


static void
dumpArrayIndex(FILE *fh, int n) {
	fputs("opt->name", fh);
	while(n-- > 0)
		fputs("->next", fh);
	fputs("->index", fh);
}

static int
dumpStructFullPath(FILE *fh, ParamDef *def, int innerCall) {
	if (def) {
		int n = dumpStructFullPath(fh, def->parent, 1);
		fputs("->", fh);
		fputs(def->name, fh);
		if (def->paramType == arrayType && innerCall) {
			fputs("[", fh);
			dumpArrayIndex(fh, n);
			fputs("]", fh);
		}
		return n + 1;
	} else {
		fputs("c", fh);
		return 0;
	}
}

static void
arrangeArray(FILE *fh, ParamDef *def) {
	if (def) {
		 arrangeArray(fh, def->parent);

		if (def->paramType == arrayType) {
			int	n;
			fputs("\t\tARRAYALLOC(", fh);
			n = dumpStructFullPath(fh, def, 0);
			fputs(", ", fh);
			dumpArrayIndex(fh, n-1);
			fputs(" + 1, ", fh);
			dumpParamDefCName(fh, def);
			fputs(");\n", fh);
		}
	}
}

static void
printIf(FILE *fh, ParamDef *def, int i) {
	if (i)
		fputs("\telse ", fh);
	else
		fputs("\t", fh);

	fputs("if ( cmpNameAtoms( opt->name, ", fh);
	dumpParamDefCName(fh, def);
	fputs(") ) {\n", fh);
	fputs("\t\tif (opt->paramType != ", fh);
	switch(def->paramType) {
		case	int32Type:
		case	uint32Type:
		case	int64Type:
		case	uint64Type:
		case	doubleType:
			fputs("numberType", fh);
			break;
		case	stringType:
			fputs("stringType", fh);
			break;
		default:
			fprintf(stderr,"Unexpected def type: %d", def->paramType);
			break;
	}

	fputs(" )\n\t\t\treturn -1;\n", fh);
	arrangeArray(fh, def);
}

static int
makeAccept(FILE *fh, ParamDef *def, int i) {

	while(def) {
		
		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	stringType:
				printIf(fh, def, i);
				fputs("\t\t", fh);
				dumpStructFullPath(fh, def, 1);
				switch(def->paramType) {
					case	int32Type:
						fputs(" = strtol(opt->paramValue.numberval, NULL, 10);\n", fh); 
						break;
					case	uint32Type:
						fputs(" = strtoul(opt->paramValue.numberval, NULL, 10);\n", fh); 
						break;
					case	int64Type:
						fputs(" = strtoll(opt->paramValue.numberval, NULL, 10);\n", fh); 
						break;
					case	uint64Type:
						fputs(" = strtoull(opt->paramValue.numberval, NULL, 10);\n", fh); 
						break;
					case	doubleType:
						fputs(" = strtod(opt->paramValue.numberval, NULL);\n", fh); 
						break;
					case	stringType:
						fputs(" = (opt->paramValue.stringval) ? strdup(opt->paramValue.stringval) : NULL;\n", fh); 
					default:
						break;
				}
				fputs("\t}\n",fh);
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				i = makeAccept(fh, def->paramValue.structval, i);
				break;
			case	arrayType:
				i = makeAccept(fh, def->paramValue.arrayval, i);
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		i++;
		def = def->next;
	}

	return i;
}

void 
cDump(FILE *fh, char* name, ParamDef *def) {
	
	fputs(
		"#include <stdlib.h>\n"
		"#include <string.h>\n"
		"#include <stdio.h>\n\n"
		"#include <prscfg.h>\n\n"
		"/*\n"
		" * Autogenerated file, do not edit it!\n"
		" */\n\n",
		fh
	);

	fputs(
		"static int\n"
		"cmpNameAtoms(NameAtom *a, NameAtom *b) {\n"
		"	while(a && b) {\n"
		"		if (strcasecmp(a->name, b->name) != 0)\n"
		"			return 0;\n"
		"		a = a->next;\n"
		"		b = b->next;\n"
		"	}\n"
		"	return (a == NULL && b == NULL) ? 1 : 0;\n"
		"}\n"
		"\n",
		fh
	);


	fprintf(fh,
		"void\n"
		"fill_default_%s(%s *c) {\n"
		, name, name
	);

	dumpDefault(fh, def);
	fputs("}\n\n", fh);

	dumpDefaultArray(fh, name, def);

	dumpParamDefCNameRecursive(fh, def);

	fputs(
		"\n"
		"#define ARRAYALLOC(x,n,t)  do {                                     \\\n"
		"   int l = 0;                                                       \\\n"
		"   __typeof__(x) y = (x);                                           \\\n"
		"   if ( (n) <= 0 ) return -2; /* wrong index */                     \\\n"
		"   while(y && *y) {                                                 \\\n"
		"       l++; y++;                                                    \\\n"
		"   }                                                                \\\n"
		"   if ( (n) >= l ) {                                                \\\n"
		"      if ( (x) == NULL )                                            \\\n"
		"          (x) = y = malloc( ((n)+1) * sizeof( __typeof__(*(x))) );  \\\n" 
		"      else {                                                        \\\n"
		"          (x) = realloc((x), ((n)+1) * sizeof( __typeof__(*(x))) ); \\\n"
		"          y = (x) + l;                                              \\\n"
		"      }                                                             \\\n"
		"      memset(y, 0, (((n)+1) - l) * sizeof( __typeof__(*(x))) );     \\\n"
		"      while ( y - (x) < (n) ) {                                     \\\n"
		"          *y = malloc( sizeof( __typeof__(**(x))) );                \\\n"
		"          acceptDefault##t(*y);                                     \\\n"
		"          y++;                                                      \\\n"
		"      }                                                             \\\n"
		"   }                                                                \\\n"
		"} while(0)\n\n"
		, fh
	);

	fprintf(fh, 
		"static int\n"
		"acceptValue(%s* c, OptDef* opt) {\n\n"
		, name);

	makeAccept(fh, def, 0);

	fputs(
		"\telse {\n"
		"\t\treturn 1;\n"
		"\t}\n"
		"\treturn 0;\n"
		"}\n\n",
		fh
	);

	fputs(
		"#define PRINTBUFLEN	8192\n"
		"static char*\n"
		"dumpOptDef(NameAtom *atom) {\n"
		"\tstatic char	buf[PRINTBUFLEN], *ptr;\n"
		"\tint  i = 0;\n\n"
		"\tptr = buf;\n"
		"\twhile(atom) {\n"
		"\t\tif (i) ptr += snprintf(ptr, PRINTBUFLEN - 1 - (ptr - buf), \".\");\n"
		"\t\tptr += snprintf(ptr, PRINTBUFLEN - 1 - (ptr - buf), \"%s\", atom->name);\n"
		"\t\tif (atom->index >= 0)\n"
		"\t\t\tptr += snprintf(ptr, PRINTBUFLEN - 1 - (ptr - buf), \"[%d]\", atom->index);\n"
		"\t\ti = 1;\n"
		"\t\tatom = atom->next;\n"
		"\t}\n"
		"\treturn buf;\n"
		"}\n\n"
		, fh
	);

	fprintf(fh,
		"void\n"
		"parse_cfg_file_%s(%s *c, FILE *fh) {\n"
		"\tint    r;\n"
		"\tOptDef *option, *opt;\n\n"
		"\toption = opt = parseCfgDef(fh);\n\n"
		"\twhile(opt) {\n"
		"\t\t/* out_warning(\"accept '%cs'\\n\", dumpOptDef(opt->name)); */\n"
		"\t\tif ( (r = acceptValue(c, opt)) != 0 )\n"
		"\t\t\tout_warning(\"Could not accept '%cs': %cd\\n\", dumpOptDef(opt->name), r);\n"
		"\t\topt = opt->next;\n"
		"\t}\n\n"
		"\tfreeCfgDef(option);\n"
		"}\n\n"
		, name, name, '%', '%', '%'
	);
}
