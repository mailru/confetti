#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include <prscfl.h>

static void
dumpParamDefCName(FILE *fh, ParamDef *def) {
	fputs("_name", fh);
	dumpStructName(fh, def, "__");
	if (def->name) {
		fputc('_', fh);
		fputc('_', fh);
		fputs(def->name, fh);
	}
}

static int
dumpParamDefNameList(FILE *fh, ParamDef *def, ParamDef *odef, int level) {
    if (def) {
			int maxlevel;

			if (def->name == NULL) 
				def = def->parent;

			maxlevel = dumpParamDefNameList(fh, def->parent, odef, level + 1);

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
			case	boolType:
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
				if (def->flags & PARAMDEF_REQUIRED) {
					fputs("static NameAtom ", fh);
					dumpParamDefCName(fh, def);
					fputs("[] = {\n", fh);
					dumpParamDefNameList(fh, def, def, 0);
					fputs("};\n", fh);
				}
				dumpParamDefCNameRecursive(fh, def->paramValue.structval);
				break;
			case	arrayType:
				fputs("static NameAtom ", fh);
				dumpParamDefCName(fh, def);
				fputs("[] = {\n", fh);
				dumpParamDefNameList(fh, def, def, 0);
				fputs("};\n", fh);

				dumpParamDefCNameRecursive(fh, def->paramValue.arrayval->paramValue.structval);
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

static void
dumpStructPath(FILE *fh, ParamDef *def, char *name) {
	fprintf(fh, "%s", name);
	dumpStructName(fh, def, "->");
	fputs("->", fh);
	fputs(def->name, fh);
}

static void
dumpInit(FILE *fh, ParamDef *def) {

	while(def) {

		fputs("\t", fh);
		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
				dumpStructPath(fh, def, "c");
				fprintf(fh, " = %"PRId32";\n", 0);
				break;
			case	stringType:
				dumpStructPath(fh, def, "c");
				fputs(" = NULL;\n", fh);
				break;
			case	boolType:
				dumpStructPath(fh, def, "c");
				fputs(" = false;\n", fh);
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment");
				break;
			case	structType:
				dumpStructPath(fh, def, "c");
				fputs(" = NULL;\n", fh);
				break;
			case	arrayType:
				dumpStructPath(fh, def, "c");
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
dumpDefault(FILE *fh, ParamDef *def) {

	while(def) {

		fputs("\t", fh);
		switch(def->paramType) {
			case	int32Type:
				dumpStructPath(fh, def, "c");
				fprintf(fh, " = %"PRId32";\n", def->paramValue.int32val);
				break;
			case	uint32Type:
				dumpStructPath(fh, def, "c");
				fprintf(fh, " = %"PRIu32"U;\n", def->paramValue.uint32val);
				break;
			case	int64Type:
				dumpStructPath(fh, def, "c");
				fprintf(fh, " = %"PRId64"LL;\n", def->paramValue.int64val);
				break;
			case	uint64Type:
				dumpStructPath(fh, def, "c");
				fprintf(fh, " = %"PRIu64"ULL;\n", def->paramValue.uint64val);
				break;
			case	doubleType:
				dumpStructPath(fh, def, "c");
				fprintf(fh, " = %g;\n", def->paramValue.doubleval);
				break;
			case	stringType:
				dumpStructPath(fh, def, "c");
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
					fputs("\tif (", fh);
					dumpStructPath(fh, def, "c");
					fputs(" == NULL) return CNF_NOMEMORY;\n", fh);
				}
				break;
			case	boolType:
				dumpStructPath(fh, def, "c");
				fprintf(fh, " = %s;\n", def->paramValue.boolval ? "true" : "false");
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				dumpStructPath(fh, def, "c");
				fputs(" = malloc(sizeof( *(", fh);
				dumpStructPath(fh, def, "c");
				fputs(") ));\n", fh);
				fputs("\tif (", fh);
				dumpStructPath(fh, def, "c");
				fputs(" == NULL) return CNF_NOMEMORY;\n", fh);
				fputs("\t", fh);
				dumpStructPath(fh, def, "c");
				fputs("->__confetti_flags = CNF_FLAG_STRUCT_NOTSET;\n", fh);
				dumpDefault(fh, def->paramValue.structval);
				break;
			case	arrayType:
				dumpStructPath(fh, def, "c");
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

			fputs("static int\nacceptDefault", fh);
			dumpParamDefCName(fh, def);
			fputs("(", fh);
			fputs(name, fh);
			dumpStructName(fh, def->paramValue.arrayval->paramValue.structval, "_");
			fputs(" *c) {\n",fh);

			/* reset parent */
			ptr = def->paramValue.arrayval->paramValue.structval;
			while(ptr) {
				ptr->parent = NULL;
				ptr = ptr->next;
			}

			dumpDefault(fh, def->paramValue.arrayval->paramValue.structval);

			/* restore parent */
			ptr = def->paramValue.arrayval->paramValue.structval;
			while(ptr) {
				ptr->parent = def->paramValue.arrayval;
				ptr = ptr->next;
			}

			fputs("\treturn 0;\n", fh);
			fputs("}\n\n", fh);

			dumpDefaultArray(fh, name, def->paramValue.arrayval->paramValue.structval);
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
dumpStructFullPath(FILE *fh, char *name, char *itername, ParamDef *def, int innerCall, int isiterator, int show_index) {
	if (def) {
		int n;
	
		if (def->name == NULL)
			def = def->parent;

		n = dumpStructFullPath(fh, name, itername, def->parent, 1, isiterator, show_index);

		fputs("->", fh);
		fputs(def->name, fh);
		if (def->paramType == arrayType && innerCall) {
			fputs("[", fh);
			if (show_index) {
				if (isiterator) {
					fprintf(fh, "%s->idx", itername);
					dumpParamDefCName(fh, def);
				} else
					dumpArrayIndex(fh, n);
			}
			fputs("]", fh);
		}
		return n + 1;
	} else {
		fputs(name, fh);
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
			n = dumpStructFullPath(fh, "c", "i", def, 0, 0, 1);
			fputs(", ", fh);
			dumpArrayIndex(fh, n-1);
			fputs(" + 1, ", fh);
			dumpParamDefCName(fh, def);
			if (def->flags & PARAMDEF_RDONLY)
				fputs(", check_rdonly, CNF_FLAG_STRUCT_NEW | CNF_FLAG_STRUCT_NOTSET);\n", fh);
			else
				fputs(", 0, CNF_FLAG_STRUCT_NEW | CNF_FLAG_STRUCT_NOTSET);\n", fh);
			fputs("\t\tif (", fh);
			dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
			fputs("->__confetti_flags & CNF_FLAG_STRUCT_NEW)\n", fh);
			fputs("\t\t\tcheck_rdonly = 0;\n", fh);
		} else {
			fputs("\t\t", fh);
			dumpStructFullPath(fh, "c", "i", def->parent, 1, 0, 1);
			fputs("->__confetti_flags &= ~CNF_FLAG_STRUCT_NOTSET;\n", fh);
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
		case	boolType:
			fputs("stringType", fh);
			break;
		case	arrayType:
			fputs("arrayType", fh);
			break;
		default:
			fprintf(stderr,"Unexpected def type: %d", def->paramType);
			break;
	}

	fputs(" )\n\t\t\treturn CNF_WRONGTYPE;\n", fh);
	if (def->paramType != arrayType) {
		arrangeArray(fh, def);
	} else {
		arrangeArray(fh, def->parent);

		fputs("\t\tARRAYALLOC(", fh);
		dumpStructFullPath(fh, "c", "i", def, 0, 0, 1);
		fputs(", 1, ", fh);
		dumpParamDefCName(fh, def);
		if (def->flags & PARAMDEF_RDONLY)
			fputs(", check_rdonly, CNF_FLAG_STRUCT_NEW | CNF_FLAG_STRUCT_NOTSET);\n", fh);
		else
			fputs(", 0, CNF_FLAG_STRUCT_NEW | CNF_FLAG_STRUCT_NOTSET);\n", fh);
	}
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
			case	boolType:
				printIf(fh, def, i);
				fputs("\t\terrno = 0;\n", fh);
				switch(def->paramType) {
					case	int32Type:
						fputs("\t\tlong int i32 = strtol(opt->paramValue.numberval, NULL, 10);\n", fh);
						fputs("\t\tif (i32 == 0 && errno == EINVAL)\n\t\t\treturn CNF_WRONGINT;\n", fh);
						fputs("\t\tif ( (i32 == LONG_MIN || i32 == LONG_MAX) && errno == ERANGE)\n\t\t\treturn CNF_WRONGRANGE;\n", fh);
						if (def->flags & PARAMDEF_RDONLY) {
							fputs("\t\tif (check_rdonly && ", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" != i32)\n\t\t\treturn CNF_RDONLY;\n", fh);
						}
						fputs("\t\t", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" = i32;\n", fh);
						break;
					case	uint32Type:
						fputs("\t\tunsigned long int u32 = strtoul(opt->paramValue.numberval, NULL, 10);\n", fh);
						fputs("\t\tif (u32 == 0 && errno == EINVAL)\n\t\t\treturn CNF_WRONGINT;\n", fh);
						fputs("\t\tif ( u32 == ULONG_MAX && errno == ERANGE)\n\t\t\treturn CNF_WRONGRANGE;\n", fh);
						if (def->flags & PARAMDEF_RDONLY) {
							fputs("\t\tif (check_rdonly && ", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" != u32)\n\t\t\treturn CNF_RDONLY;\n", fh);
						}
						fputs("\t\t", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" = u32;\n", fh);
						break;
					case	int64Type:
						fputs("\t\tlong long int i64 = strtoll(opt->paramValue.numberval, NULL, 10);\n", fh);
						fputs("\t\tif (i64 == 0 && errno == EINVAL)\n\t\t\treturn CNF_WRONGINT;\n", fh);
						fputs("\t\tif ( (i64 == LLONG_MIN || i64 == LLONG_MAX) && errno == ERANGE)\n\t\t\treturn CNF_WRONGRANGE;\n", fh);
						if (def->flags & PARAMDEF_RDONLY) {
							fputs("\t\tif (check_rdonly && ", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" != i64)\n\t\t\treturn CNF_RDONLY;\n", fh);
						}
						fputs("\t\t", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" = i64;\n", fh);
						break;
					case	uint64Type:
						fputs("\t\tunsigned long long int u64 = strtoull(opt->paramValue.numberval, NULL, 10);\n", fh);
						fputs("\t\tif (u64 == 0 && errno == EINVAL)\n\t\t\treturn CNF_WRONGINT;\n", fh);
						fputs("\t\tif ( u64 == ULLONG_MAX && errno == ERANGE)\n\t\t\treturn CNF_WRONGRANGE;\n", fh);
						if (def->flags & PARAMDEF_RDONLY) {
							fputs("\t\tif (check_rdonly && ", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" != u64)\n\t\t\treturn CNF_RDONLY;\n", fh);
						}
						fputs("\t\t", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" = u64;\n", fh);
						break;
					case	doubleType:
						fputs("\t\tdouble dbl = strtod(opt->paramValue.numberval, NULL);\n", fh); 
						fputs("\t\tif ( (dbl == 0 || dbl == -HUGE_VAL || dbl == HUGE_VAL) && errno == ERANGE)\n\t\t\treturn CNF_WRONGRANGE;\n", fh);
						if (def->flags & PARAMDEF_RDONLY) {
							fputs("\t\tif (check_rdonly && ", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" != dbl)\n\t\t\treturn CNF_RDONLY;\n", fh);
						}
						fputs("\t\t", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" = dbl;\n", fh);
						break;
					case	stringType:
						if (def->flags & PARAMDEF_RDONLY) {
							fputs("\t\tif (check_rdonly && ( (opt->paramValue.stringval == NULL && ", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" == NULL) || strcmp(opt->paramValue.stringval, ", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(") != 0))\n\t\t\treturn CNF_RDONLY;\n", fh);
						}
						fputs("\t\t if (", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
						fputs(") free(", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
						fputs(");\n", fh);
						fputs("\t\t", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" = (opt->paramValue.stringval) ? strdup(opt->paramValue.stringval) : NULL;\n", fh);
						fputs("\t\tif (opt->paramValue.stringval && ", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" == NULL)\n\t\t\treturn CNF_NOMEMORY;\n", fh);
						break;
					case	boolType:
						fputs("\t\tbool bln;\n\n", fh);
						fputs("\t\tif (strcasecmp(opt->paramValue.stringval, \"true\") == 0 ||\n", fh);
						fputs("\t\t\t\tstrcasecmp(opt->paramValue.stringval, \"yes\") == 0 ||\n", fh);
						fputs("\t\t\t\tstrcasecmp(opt->paramValue.stringval, \"enable\") == 0 ||\n", fh);
						fputs("\t\t\t\tstrcasecmp(opt->paramValue.stringval, \"on\") == 0 ||\n", fh);
						fputs("\t\t\t\tstrcasecmp(opt->paramValue.stringval, \"1\") == 0 )\n", fh);
						fputs("\t\t\tbln = true;\n", fh);
						fputs("\t\telse if (strcasecmp(opt->paramValue.stringval, \"false\") == 0 ||\n", fh);
						fputs("\t\t\t\tstrcasecmp(opt->paramValue.stringval, \"no\") == 0 ||\n", fh);
						fputs("\t\t\t\tstrcasecmp(opt->paramValue.stringval, \"disable\") == 0 ||\n", fh);
						fputs("\t\t\t\tstrcasecmp(opt->paramValue.stringval, \"off\") == 0 ||\n", fh);
						fputs("\t\t\t\tstrcasecmp(opt->paramValue.stringval, \"0\") == 0 )\n", fh);
						fputs("\t\t\tbln = false;\n", fh);
						fputs("\t\telse\n", fh);
						fputs("\t\t\treturn CNF_WRONGRANGE;\n", fh);

						if (def->flags & PARAMDEF_RDONLY) {
							fputs("\t\tif (check_rdonly && ", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" != bln)\n\t\t\treturn CNF_RDONLY;\n", fh);
						}
						fputs("\t\t", fh);
							dumpStructFullPath(fh, "c", "i", def, 1, 0, 1);
							fputs(" = bln;\n", fh);
						break;
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
				printIf(fh, def, i);
				fputs("\t}\n",fh);
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

static void
makeIteratorStates(FILE *fh, ParamDef *def) {

	while(def) {

		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	stringType:
			case	boolType:
				fputs("\tS", fh);
				dumpParamDefCName(fh, def);
				fputs(",\n", fh);
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				fputs("\tS", fh);
				dumpParamDefCName(fh, def);
				fputs(",\n", fh);
				makeIteratorStates(fh, def->paramValue.structval);
				break;
			case	arrayType:
				fputs("\tS", fh);
				dumpParamDefCName(fh, def);
				fputs(",\n", fh);
				makeIteratorStates(fh, def->paramValue.arrayval->paramValue.structval);
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

static void
makeArrayIndexes(FILE *fh, ParamDef *def) {

	while(def) {

		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	stringType:
			case	boolType:
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				makeArrayIndexes(fh, def->paramValue.structval);
				break;
			case	arrayType:
				fputs("\tint\tidx", fh);
				dumpParamDefCName(fh, def);
				fputs(";\n", fh);
				makeArrayIndexes(fh, def->paramValue.arrayval->paramValue.structval);
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

static void
fputt(FILE *fh, int level) {
	while(level--)
		fputc('\t', fh);
}

static void
fputts(FILE *fh, int level, char *str) {
	while(level--)
		fputc('\t', fh);
	fputs(str, fh);
}

static void
makeSwitchArrayList(FILE *fh, ParamDef *def, int level) {
	while(def) {
		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	stringType:
			case	boolType:
				fputt(fh, level+2); fputs( "case S", fh);
				dumpParamDefCName(fh, def);
				fputs(":\n", fh);
				break;
			case structType:
				fputt(fh, level+2); fputs( "case S", fh);
				dumpParamDefCName(fh, def);
				fputs(":\n", fh);
				makeSwitchArrayList(fh, def->paramValue.structval, level);
				break;
			case arrayType:
				fputt(fh, level+2); fputs( "case S", fh);
				dumpParamDefCName(fh, def);
				fputs(":\n", fh);
				makeSwitchArrayList(fh, def->paramValue.arrayval->paramValue.structval, level);
				break;
			default:
				break;
		}

		def = def->next;
	}
}

static void
strdupValue(FILE *fh, ParamDef *def, int level) {
	switch(def->paramType) {
		case	int32Type:
			fputt(fh, level); fputs("*v = malloc(32);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(CNF_NOMEMORY, \"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%\"PRId32, ", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(");\n", fh);
			break;
		case	uint32Type:
			fputt(fh, level); fputs("*v = malloc(32);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(CNF_NOMEMORY, \"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%\"PRIu32, ", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(");\n", fh);
			break;
		case	int64Type:
			fputt(fh, level); fputs("*v = malloc(32);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(CNF_NOMEMORY, \"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%\"PRId64, ", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(");\n", fh);
			break;
		case	uint64Type:
			fputt(fh, level); fputs("*v = malloc(32);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(CNF_NOMEMORY, \"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%\"PRIu64, ", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(");\n", fh);
			break;
		case	doubleType:
			fputt(fh, level); fputs("*v = malloc(32);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(CNF_NOMEMORY, \"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%g\", ", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(");\n", fh);
			break;
		case	stringType:
			fputt(fh, level);
			fputs("*v = (", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(") ? strdup(", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(") : NULL;\n", fh);
			fputt(fh, level); fputs("if (*v == NULL && ", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(") {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(CNF_NOMEMORY, \"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			break;
		case	boolType:
			fputt(fh, level); fputs("*v = malloc(8);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(CNF_NOMEMORY, \"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%s\", ", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(" ? \"true\" : \"false\");\n", fh);
			break;
		default:
			break;
	}
}

static void
resetSubArray(FILE *fh, ParamDef *def, int level) {
	while(def) {
		switch(def->paramType) {
			case structType:
				resetSubArray(fh, def->paramValue.structval, level);
				break;
			case arrayType:
				fputt(fh, level + 3);
				fputs("i->idx", fh);
				dumpParamDefCName(fh, def);
				fputs(" = 0;\n", fh);
				resetSubArray(fh, def->paramValue.arrayval->paramValue.structval, level);
				break;
			default:
				break;
		}

		def = def->next;
	}
}

static int
dumpStructNameFullPath(FILE *fh, ParamDef *def, int innerCall) {
	if (def) {
		int n;

		if (def->name == NULL)
			def = def->parent;

		n = dumpStructNameFullPath(fh, def->parent, 1);
		if (n!=0)
			fputs(".", fh);
		fputs(def->name, fh);
		if (def->paramType == arrayType && innerCall) {
			fputs("[%d]", fh);
		}
		return n + 1;
	} else {
		return 0;
	}
}

static int
dumpArrayIndexes(FILE *fh, ParamDef *def, int innerCall) {
	if (def) {
		int n;

		if (def->name == NULL)
			def = def->parent;

		n = dumpArrayIndexes(fh, def->parent, 1);
		if (def->paramType == arrayType && innerCall) {
			fputs(", ", fh);
			fputs("i->idx", fh);
			dumpParamDefCName(fh, def);	
		}
		return n + 1;
	} else {
		return 0;
	}
}

static void
makeSwitch(FILE *fh, ParamDef *def, ParamDef *parent, int level, ParamDef *next) {

	while(def) {

		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	stringType:
			case	boolType:
				/* case */
				fputt(fh, level+2); fputs( "case S", fh);
				dumpParamDefCName(fh, def);
				fputs(":\n", fh);
				/* make val */
				strdupValue(fh, def, level+3);
				/* make name */
				fputt(fh, level + 3);
				fputs("snprintf(buf, PRINTBUFLEN-1, \"", fh);
				dumpStructNameFullPath(fh, def, 0);
				fputs("\"", fh);
				dumpArrayIndexes(fh, def, 0);
				fputs(");\n", fh);
				/* extra work to switch to the next state */
				if (def->next || next) {
						fputt(fh, level + 3);
						fputs("i->state = S", fh);
						dumpParamDefCName(fh, def->next ? def->next : next);
						fputs(";\n", fh);
				} else {
					if (parent) {
						fputt(fh, level + 3);
						fputs("i->state = S", fh);
						dumpParamDefCName(fh, parent);
						fputs(";\n", fh);
						fputt(fh, level + 3);
						fputs("i->idx", fh);
						dumpParamDefCName(fh, parent);
						fputs("++;\n", fh);	
						resetSubArray(fh, parent->paramValue.arrayval->paramValue.structval, level);
					} else {
						fputt(fh, level + 3);
						fputs("i->state = _S_Finished;\n", fh);
					}
				}
				fputt(fh, level + 3);
				fputs("return buf;\n", fh);
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				fputt(fh, level+2); fputs( "case S", fh);
				dumpParamDefCName(fh, def);
				fputs(":\n", fh);
				makeSwitch(fh, def->paramValue.structval, parent, level, def->next ? def->next : next);
				break;
			case	arrayType:
				fputt(fh, level+2); fputs( "case S", fh);
				dumpParamDefCName(fh, def);
				fputs(":\n", fh);
				fputt(fh, level+3); fputs( "i->state = S", fh);
				dumpParamDefCName(fh, def);
				fputs(";\n", fh);
				makeSwitchArrayList(fh, def->paramValue.arrayval->paramValue.structval, level);
				fputt(fh, level+3); fputs("if (", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
					fputs(" && ", fh);
					dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
					fputs("[i->idx", fh);
					dumpParamDefCName(fh, def);
					fputs("]) {\n", fh);
				fputt(fh, level+4); fputs("switch(i->state) {\n", fh);
				fputt(fh, level+5); fputs( "case S", fh);
				dumpParamDefCName(fh, def);
				fputs(":\n", fh);
				makeSwitch(fh, def->paramValue.arrayval->paramValue.structval, def, level+3, NULL);
				fputt(fh, level+5); fputs("default:\n", fh);
				fputt(fh, level+6); fputs("break;\n", fh);
				fputt(fh, level+4); fputs("}\n", fh);
				fputt(fh, level+3); fputs("}\n", fh);
				if (parent && !def->next) {
					fputt(fh, level+3); fputs("else {\n", fh);
					fputt(fh, level+4); fputs( "i->state = S", fh);
					dumpParamDefCName(fh, parent);
					fputs(";\n", fh);
					fputt(fh, level+4);
					fputs("i->idx", fh);
					dumpParamDefCName(fh, parent);
					fputs("++;\n", fh);	
					resetSubArray(fh, parent->paramValue.arrayval->paramValue.structval, level+1);
					fputt(fh, level+4); 
					fputs("goto again;\n", fh);
					fputt(fh, level+3); fputs("}\n", fh);
				}
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

static int
countParents(ParamDef *def) {
	int n = 0;

	while(def) {
		if (def->name != NULL)
			n++;
		def = def->parent;
	}

	return n;
}

static void
dumpCheckArrayIndexes(FILE *fh, ParamDef *def, int level) {
	ParamDef	*parent = def->parent;
	int			i, n;


	while(parent) { 
		if (parent->name == NULL)
			parent = parent->parent;

		if (parent->paramType == arrayType) {
			fputt(fh, level+2);
			dumpParamDefCName(fh, def);

			n = countParents(parent->parent);
			for(i=0; i<n; i++)
				fputs("->next", fh);

			fputs("->index = i->idx", fh);
			dumpParamDefCName(fh, parent);
			fputs(";\n", fh);
		}

		parent = parent->parent;
	}
}

static void
makeOutCheck(FILE *fh, ParamDef *def, int level) {
	fputt(fh, level+2);
	fputs("res++;\n", fh);
	dumpCheckArrayIndexes(fh, def, level);
	fputt(fh, level+2);
	fputs("out_warning(CNF_NOTSET, \"Option '%s' is not set (or has a default value)\", dumpOptDef(", fh);
	dumpParamDefCName(fh, def);
	fputs("));\n", fh);
	fputt(fh, level+1);
	fputs("}\n", fh);
}

static void
makeCheck(FILE *fh, ParamDef *def, int level) {
	while(def) {
		switch(def->paramType) {
			case	int32Type:
				if ((def->flags & PARAMDEF_REQUIRED) == 0)
					break;
				fputt(fh, level+1);
				fputs("if (", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fprintf(fh, " == %"PRId32") {\n", def->paramValue.int32val);
				makeOutCheck(fh, def, level);
				break;
			case	uint32Type:
				if ((def->flags & PARAMDEF_REQUIRED) == 0)
					break;
				fputt(fh, level+1);
				fputs("if (", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fprintf(fh, " == %"PRIu32") {\n", def->paramValue.uint32val);
				makeOutCheck(fh, def, level);
				break;
			case	int64Type:
				if ((def->flags & PARAMDEF_REQUIRED) == 0)
					break;
				fputt(fh, level+1);
				fputs("if (", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fprintf(fh, " == %"PRId64") {\n", def->paramValue.int64val);
				makeOutCheck(fh, def, level);
				break;
			case	uint64Type:
				if ((def->flags & PARAMDEF_REQUIRED) == 0)
					break;
				fputt(fh, level+1);
				fputs("if (", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fprintf(fh, " == %"PRIu64") {\n", def->paramValue.uint64val);
				makeOutCheck(fh, def, level);
				break;
			case	doubleType:
				if ((def->flags & PARAMDEF_REQUIRED) == 0)
					break;
				fputt(fh, level+1);
				fputs("if (", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fprintf(fh, " == %g) {\n", def->paramValue.doubleval);
				makeOutCheck(fh, def, level);
				break;
			case	stringType:
				if ((def->flags & PARAMDEF_REQUIRED) == 0)
					break;
				fputt(fh, level+1);
				if (def->paramValue.stringval == NULL) {
					fputs("if (", fh);
					dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
					fputs(" == NULL) {\n", fh);
				} else {
					char *ptr = def->paramValue.stringval;

					fputs("if (", fh);
					dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
					fputs(" != NULL && strcmp(", fh);
					dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
					fputs(", \"", fh);

					while(*ptr) {
						if (*ptr == '"')
							fputc('\\', fh);
						fputc(*ptr, fh);
						ptr++;
					}
					fputs("\") == 0) {\n", fh);
				}
				makeOutCheck(fh, def, level);
				break;
			case	boolType:
				if ((def->flags & PARAMDEF_REQUIRED) == 0)
					break;
				fputt(fh, level+1);
				fputs("if (", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fprintf(fh, " == %s) {\n", def->paramValue.boolval ? "true" : "false");
				makeOutCheck(fh, def, level);
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				fputts(fh, level + 1, "if (");
				dumpStructFullPath(fh, "c", "i", def, 1, 1, 1);
				fputs("->__confetti_flags & CNF_FLAG_STRUCT_NOTSET) {\n", fh);
				if (def->flags & PARAMDEF_REQUIRED) {
					fputts(fh, level + 2, "res++;\n");
					dumpCheckArrayIndexes(fh, def, level);
					fputts(fh, level + 2, "out_warning(CNF_NOTSET, \"Option '%s' is not set\", dumpOptDef(");
					dumpParamDefCName(fh, def);
					fputs("));\n", fh);
				} else {
					fputts(fh, level + 2, "(void)0;\n");
				}
				fputts(fh, level + 1, "} else {\n");
					makeCheck(fh, def->paramValue.structval, level + 1);
					fputts(fh, level + 1, "}\n\n");
				break;
			case	arrayType:
				if (def->flags & PARAMDEF_REQUIRED) {
					fputt(fh, level+1);
					fputs("if (", fh);
					dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
					fputs(" == NULL) {\n", fh);
					makeOutCheck(fh, def, level);
				}

				fputt(fh, level+1);
				fputs("i->idx", fh);
				dumpParamDefCName(fh, def);
				fputs(" = 0;\n", fh);
				fputt(fh, level+1);
				fputs("while (", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(" && ", fh);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs("[i->idx", fh);
				dumpParamDefCName(fh, def);
				fputs("]) {\n", fh);

				makeCheck(fh, def->paramValue.arrayval, level+1);

				fputt(fh, level+2);
				fputs("i->idx", fh);
				dumpParamDefCName(fh, def);
				fputs("++;\n", fh);
				fputt(fh, level+1);
				fputs("}\n\n", fh);

				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}
		def = def->next;
	}
}

static void
makeCleanFlags(FILE *fh, ParamDef *def, int level) {
	while(def) {
		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	stringType:
			case	boolType:
				break;
			case	commentType:
				break;
			case	structType:
				fputt(fh, level + 1);
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs("->__confetti_flags &= ~CNF_FLAG_STRUCT_NEW;\n", fh);

				makeCleanFlags(fh, def->paramValue.structval, level);
				break;
			case	arrayType:
				fputs("\n", fh);
				fputts(fh, level + 1, "if (");
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(" != NULL) {\n", fh);
					fputts(fh, level + 1, "\ti->idx");
					dumpParamDefCName(fh, def);
					fputs(" = 0;\n", fh);
					fputts(fh, level + 1, "\twhile (");
					dumpStructFullPath(fh, "c", "i", def, 1, 1, 1);
					fputs(" != NULL) {\n", fh);
						fputt(fh, level + 1);
						dumpStructFullPath(fh, "\t\tc", "i", def, 1, 1, 1);
						fputs("->__confetti_flags &= ~CNF_FLAG_STRUCT_NEW;\n\n", fh);
						makeCleanFlags(fh, def->paramValue.arrayval->paramValue.structval, level + 2);
						fputs("\n", fh);
						fputts(fh, level + 1, "\t\ti->idx");
						dumpParamDefCName(fh, def);
						fputs("++;\n", fh);
					fputts(fh, level + 1, "\t}\n");
				fputts(fh, level + 1, "}\n");
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

static void
makeDup(FILE *fh, ParamDef *def, int level) {
	while(def) {
		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	boolType:
				fputt(fh, level + 1);
				dumpStructFullPath(fh, "dst", "i", def, 1, 1, 1);
				fputs(" = ", fh);
				dumpStructFullPath(fh, "src", "i", def, 1, 1, 1);
				fputs(";\n", fh);
				break;
			case	stringType:
				fputt(fh, level + 1);

				fputs("if (", fh);
				dumpStructFullPath(fh, "dst", "i", def, 1, 1, 1);
				fputs(") free(", fh);
				dumpStructFullPath(fh, "dst", "i", def, 1, 1, 1);
				fputs(");", fh);

				dumpStructFullPath(fh, "dst", "i", def, 1, 1, 1);
				fputs(" = ", fh);
				dumpStructFullPath(fh, "src", "i", def, 1, 1, 1);
				fputs(" == NULL ? NULL : strdup(", fh);
				dumpStructFullPath(fh, "src", "i", def, 1, 1, 1);
				fputs(");\n", fh);
				fputts(fh, level + 1, "if (");
				dumpStructFullPath(fh, "src", "i", def, 1, 1, 1);
				fputs(" != NULL && ", fh);
				dumpStructFullPath(fh, "dst", "i", def, 1, 1, 1);
				fputs(" == NULL)\n", fh);
					fputts(fh, level + 1, "\treturn CNF_NOMEMORY;\n");
				break;
			case	commentType:
				break;
			case	structType:
				fputs("\n", fh);
				fputt(fh, level + 1);
				dumpStructFullPath(fh, "dst", "i", def, 1, 1, 1);
				fputs(" = calloc(1, sizeof(*(", fh);
				dumpStructFullPath(fh, "dst", "i", def, 1, 1, 1);
				fputs(") ));\n", fh);

				fputts(fh, level + 1, "if (");
				dumpStructFullPath(fh, "dst", "i", def, 1, 1, 1);
				fputs(" == NULL)\n", fh);
					fputts(fh, level + 1, "\treturn CNF_NOMEMORY;\n");
				makeDup(fh, def->paramValue.structval, level);
				break;
			case	arrayType:
				fputs("\n", fh);
				fputt(fh, level + 1);
				dumpStructFullPath(fh, "dst", "i", def, 0, 1, 1);
				fputs(" = NULL;\n", fh);
				fputts(fh, level + 1, "if (");
				dumpStructFullPath(fh, "src", "i", def, 0, 1, 1);
				fputs(" != NULL) {\n", fh);
					fputts(fh, level + 1, "\ti->idx");
					dumpParamDefCName(fh, def);
					fputs(" = 0;\n", fh);
					fputts(fh, level, "\t\tARRAYALLOC(");
					dumpStructFullPath(fh, "dst", "i", def, 0, 1, 1);
					fputs(", 1, ", fh);
					dumpParamDefCName(fh, def);
					fputs(", 0, 0);\n\n", fh);
					fputts(fh, level + 1, "\twhile (");
					dumpStructFullPath(fh, "src", "i", def, 1, 1, 1);
					fputs(" != NULL) {\n", fh);
						fputts(fh, level + 1, "\t\tARRAYALLOC(");
						dumpStructFullPath(fh, "dst", "i", def, 0, 1, 1);
						fputs(", ", fh);
						fputs("i->idx", fh);
						dumpParamDefCName(fh, def);
						fputs(" + 1, ", fh);
						dumpParamDefCName(fh, def);
						fputs(", 0, 0);\n\n", fh);
						makeDup(fh, def->paramValue.arrayval->paramValue.structval, level + 2);
						fputs("\n", fh);
						fputts(fh, level + 1, "\t\ti->idx");
						dumpParamDefCName(fh, def);
						fputs("++;\n", fh);
					fputts(fh, level + 1, "\t}\n");
				fputts(fh, level + 1, "}\n");
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

static void
makeDestroy(FILE *fh, ParamDef *def, int level) {
	while(def) {
		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	boolType:
			case	commentType:
				break;
			case	stringType:
				fputts(fh, level + 1, "if (");
				dumpStructFullPath(fh, "c", "i", def, 1, 1, 1);
				fputs(" != NULL)\n", fh);
					fputts(fh, level + 1, "\tfree(");
					dumpStructFullPath(fh, "c", "i", def, 1, 1, 1);
					fputs(");\n", fh);
				break;
			case	structType:
				fputts(fh, level + 1, "if (");
				dumpStructFullPath(fh, "c", "i", def, 1, 1, 1);
				fputs(" != NULL) {\n", fh);
					makeDestroy(fh, def->paramValue.structval, level + 1);
					fputs("\n", fh);
					fputts(fh, level + 1, "\tfree(");
					dumpStructFullPath(fh, "c", "i", def, 1, 1, 1);
					fputs(");\n", fh);
				fputts(fh, level + 1, "}\n");
				break;
			case	arrayType:
				fputs("\n", fh);
				fputts(fh, level + 1, "if (");
				dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
				fputs(" != NULL) {\n", fh);
					fputts(fh, level + 1, "\ti->idx");
					dumpParamDefCName(fh, def);
					fputs(" = 0;\n", fh);
					fputts(fh, level + 1, "\twhile (");
					dumpStructFullPath(fh, "c", "i", def, 1, 1, 1);
					fputs(" != NULL) {\n", fh);
						makeDestroy(fh, def->paramValue.arrayval->paramValue.structval, level + 2);
						fputs("\n", fh);
						fputts(fh, level + 1, "\t\tfree(");
						dumpStructFullPath(fh, "c", "i", def, 1, 1, 1);
						fputs(");\n\n", fh);
						fputts(fh, level + 1, "\t\ti->idx");
						dumpParamDefCName(fh, def);
						fputs("++;\n", fh);
					fputts(fh, level + 1, "\t}\n\n");
					fputts(fh, level + 1, "\tfree(");
					dumpStructFullPath(fh, "c", "i", def, 0, 1, 1);
					fputs(");\n", fh);
				fputts(fh, level + 1, "}\n");
				break;

			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

static void
makeCmp(FILE *fh, ParamDef *def, int level) {
	while(def) {
		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	boolType:
				if (!(def->flags & PARAMDEF_RDONLY)) {
					fputts(fh, level + 1, "if (!only_check_rdonly) {\n");
					level++;
				}

				fputts(fh, level + 1, "if (");
				dumpStructFullPath(fh, "c1", "i1", def, 1, 1, 1);
				fputs(" != ", fh);
				dumpStructFullPath(fh, "c2", "i2", def, 1, 1, 1);
				fputs(") {\n", fh);
					fputts(fh, level + 1, "\tsnprintf(diff, PRINTBUFLEN - 1, \"%s\", \"");
					dumpStructFullPath(fh, "c", "i", def, 1, 1, 0);
					fputs("\");\n\n", fh);
					fputts(fh, level + 1, "\treturn diff;\n");
				fputts(fh, level + 1, "}\n");

				if (!(def->flags & PARAMDEF_RDONLY)) {
					level--;
					fputts(fh, level + 1, "}\n");
				}
				break;
			case	commentType:
				break;
			case	stringType:
				if (!(def->flags & PARAMDEF_RDONLY)) {
					fputts(fh, level + 1, "if (!only_check_rdonly) {\n");
					level++;
				}

				fputts(fh, level + 1, "if (confetti_strcmp(");
				dumpStructFullPath(fh, "c1", "i1", def, 1, 1, 1);
				fputs(", ", fh);
				dumpStructFullPath(fh, "c2", "i2", def, 1, 1, 1);
				fputs(") != 0) {\n", fh);
					fputts(fh, level + 1, "\tsnprintf(diff, PRINTBUFLEN - 1, \"%s\", \"");
					dumpStructFullPath(fh, "c", "i", def, 1, 1, 0);
					fputs("\");\n\n", fh);
					fputts(fh, level + 1, "\treturn diff;\n");
				fputs("}\n", fh);

				if (!(def->flags & PARAMDEF_RDONLY)) {
					level--;
					fputts(fh, level + 1, "}\n");
				}
				break;
			case	structType:
				makeCmp(fh, def->paramValue.structval, level);
				break;
			case	arrayType:
				fputs("\n", fh);
				fputts(fh, level + 1, "i1->idx");
				dumpParamDefCName(fh, def);
				fputs(" = 0;\n", fh);
				fputts(fh, level + 1, "i2->idx");
				dumpParamDefCName(fh, def);
				fputs(" = 0;\n", fh);
				fputts(fh, level + 1, "while (");
				dumpStructFullPath(fh, "c1", "i1", def, 0, 1, 1);
				fputs(" != NULL && ", fh);
				dumpStructFullPath(fh, "c1", "i1", def, 1, 1, 1);
				fputs(" != NULL && ", fh);
				dumpStructFullPath(fh, "c2", "i2", def, 0, 1, 1);
				fputs(" != NULL && " , fh);
				dumpStructFullPath(fh, "c2", "i2", def, 1, 1, 1);
				fputs(" != NULL) {\n", fh);
					makeCmp(fh, def->paramValue.arrayval->paramValue.structval, level + 1);
					fputs("\n", fh);
					fputts(fh, level + 1, "\ti1->idx");
					dumpParamDefCName(fh, def);
					fputs("++;\n", fh);
					fputts(fh, level + 1, "\ti2->idx");
					dumpParamDefCName(fh, def);
					fputs("++;\n", fh);
				fputts(fh, level + 1, "}\n");

				if (!(def->flags & PARAMDEF_RDONLY)) {
					fputts(fh, level + 1, "if (!only_check_rdonly) {\n");
					level++;
				}

				fputts(fh, level + 1, "if (!(");
				dumpStructFullPath(fh, "c1", "i1", def, 0, 1, 1);
				fputs(" == ", fh);
				dumpStructFullPath(fh, "c2", "i2", def, 0, 1, 1);
				fputs(" && ", fh);
				dumpStructFullPath(fh, "c1", "i1", def, 0, 1, 1);
				fputs(" == NULL) && (", fh);
				dumpStructFullPath(fh, "c1", "i1", def, 0, 1, 1);
				fputs(" == NULL || ", fh);
				dumpStructFullPath(fh, "c2", "i2", def, 0, 1, 1);
				fputs(" == NULL || ", fh);
				dumpStructFullPath(fh, "c1", "i1", def, 1, 1, 1);
				fputs(" != NULL || ", fh);
				dumpStructFullPath(fh, "c2", "i2", def, 1, 1, 1);
				fputs(" != NULL)) {\n", fh);
					fputts(fh, level + 1, "\tsnprintf(diff, PRINTBUFLEN - 1, \"%s\", \"");
					dumpStructFullPath(fh, "c", "i", def, 1, 1, 0);
					fputs("\");\n\n", fh);
					fputts(fh, level + 1, "\treturn diff;\n");
				fputts(fh, level + 1, "}\n");

				if (!(def->flags & PARAMDEF_RDONLY)) {
					level--;
					fputts(fh, level + 1, "}\n");
				}
				break;

			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		def = def->next;
	}
}

void 
cDump(FILE *fh, char* name, ParamDef *def) {

	fputs(
		"#include <errno.h>\n"
		"#include <limits.h>\n"
		"#include <inttypes.h>\n"
		"#include <math.h>\n"
		"#include <stdlib.h>\n"
		"#include <string.h>\n"
		"#include <stdio.h>\n\n"
		"/*\n"
		" * Autogenerated file, do not edit it!\n"
		" */\n\n",
		fh
	);

	if (def->paramType == builtinType) {
		fputs(def->paramValue.stringval, fh);
		def = def->next;
	}


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
		"init_%s(%s *c) {\n"
		"\tc->__confetti_flags = 0;\n\n"
		, name, name);

	dumpInit(fh, def);
	fputs("}\n\n", fh);

	fprintf(fh,
		"int\n"
		"fill_default_%s(%s *c) {\n"
		"\tc->__confetti_flags = 0;\n\n"
		, name, name
	);

	dumpDefault(fh, def);
	fputs("\treturn 0;\n}\n\n", fh);


	fprintf(fh,
		"void\n"
		"swap_%s(struct %s *c1, struct %s *c2) {\n"
		, name, name, name);

	fprintf(fh,
		"\tstruct %s tmpcfg = *c1;\n"
		"\t*c1 = *c2;\n"
		"\t*c2 = tmpcfg;\n"
		"}\n\n", name);


	dumpDefaultArray(fh, name, def);

	dumpParamDefCNameRecursive(fh, def);

	fputs(
		"\n"
		"#define ARRAYALLOC(x,n,t,_chk_ro, __flags)  do {                    \\\n"
		"   int l = 0, ar;                                                   \\\n"
		"   __typeof__(x) y = (x), t;                                        \\\n"
		"   if ( (n) <= 0 ) return CNF_WRONGINDEX; /* wrong index */         \\\n"
		"   while(y && *y) {                                                 \\\n"
		"       l++; y++;                                                    \\\n"
		"   }                                                                \\\n"
		"   if ( (n) >= (l + 1) ) {                                          \\\n"
		"      if (_chk_ro)  return CNF_RDONLY;                              \\\n"
		"      if ( (x) == NULL )                                            \\\n"
		"          t = y = malloc( ((n)+1) * sizeof( __typeof__(*(x))) );    \\\n"
		"      else {                                                        \\\n"
		"          t = realloc((x), ((n)+1) * sizeof( __typeof__(*(x))) );   \\\n"
		"          y = t + l;                                                \\\n"
		"      }                                                             \\\n"
		"      if (t == NULL)  return CNF_NOMEMORY;                          \\\n"
		"      (x) = t;                                                      \\\n"
		"      memset(y, 0, (((n)+1) - l) * sizeof( __typeof__(*(x))) );     \\\n"
		"      while ( y - (x) < (n) ) {                                     \\\n"
		"          *y = malloc( sizeof( __typeof__(**(x))) );                \\\n"
		"          if (*y == NULL)  return CNF_NOMEMORY;                     \\\n"
		"          if ( (ar = acceptDefault##t(*y)) != 0 ) return ar;        \\\n"
		"          (*y)->__confetti_flags = __flags;                         \\\n"
		"          y++;                                                      \\\n"
		"      }                                                             \\\n"
		"   }                                                                \\\n"
		"} while(0)\n\n"
		, fh
	);

	fprintf(fh, 
		"static ConfettyError\n"
		"acceptValue(%s* c, OptDef* opt, int check_rdonly) {\n\n"
		, name);

	makeAccept(fh, def, 0);

	fputs(
		"\telse {\n"
		"\t\treturn CNF_MISSED;\n"
		"\t}\n"
		"\treturn CNF_OK;\n"
		"}\n\n",
		fh
	);

	fprintf(fh,
		"static void cleanFlags(%s* c, OptDef* opt);\n\n"
		, name);

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
		"static void\n"
		"acceptCfgDef(%s *c, OptDef *opt, int check_rdonly, int *n_accepted, int *n_skipped) {\n"
		"\tConfettyError	r;\n"
		"\tOptDef		*orig_opt = opt;\n\n"
		"\tif (n_accepted) *n_accepted=0;\n"
		"\tif (n_skipped) *n_skipped=0;\n"
		"\n"
		"\twhile(opt) {\n"
		"\t\tr = acceptValue(c, opt, check_rdonly);\n"
		"\t\tswitch(r) {\n"
		"\t\t\tcase CNF_OK:\n"
		"\t\t\t\tif (n_accepted) (*n_accepted)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase CNF_MISSED:\n"
		"\t\t\t\tout_warning(r, \"Could not find '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase CNF_WRONGTYPE:\n"
		"\t\t\t\tout_warning(r, \"Wrong value type for '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase CNF_WRONGINDEX:\n"
		"\t\t\t\tout_warning(r, \"Wrong array index in '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase CNF_RDONLY:\n"
		"\t\t\t\tout_warning(r, \"Could not accept read only '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase CNF_WRONGINT:\n"
		"\t\t\t\tout_warning(r, \"Could not parse integer value for '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase CNF_WRONGRANGE:\n"
		"\t\t\t\tout_warning(r, \"Wrong range for '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase CNF_NOMEMORY:\n"
		"\t\t\t\tout_warning(r, \"Not enough memory to accept '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase CNF_NOTSET:\n"
		"\t\t\t\tout_warning(r, \"Option '%cs' is not set (or has a default value)\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tdefault:\n"
		"\t\t\t\tout_warning(r, \"Unknown error for '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t}\n\n"
		"\t\topt = opt->next;\n"
		"\t}\n"
		"\n"
		"\tcleanFlags(c, orig_opt);\n"
		"}\n\n"
		, name, '%', '%', '%', '%', '%', '%', '%', '%', '%'
	); 

	fprintf(fh,
		"void\n"
		"parse_cfg_file_%s(%s *c, FILE *fh, int check_rdonly, int *n_accepted, int *n_skipped) {\n"
		"\tOptDef *option;\n\n"
		"\toption = parseCfgDef(fh);\n"
		"\tacceptCfgDef(c, option, check_rdonly, n_accepted, n_skipped);\n"
		"\tfreeCfgDef(option);\n"
		"}\n\n"
		, name, name
	);

	fprintf(fh,
		"void\n"
		"parse_cfg_buffer_%s(%s *c, char *buffer, int check_rdonly, int *n_accepted, int *n_skipped) {\n"
		"\tOptDef *option;\n\n"
		"\toption = parseCfgDefBuffer(buffer);\n"
		"\tacceptCfgDef(c, option, check_rdonly, n_accepted, n_skipped);\n"
		"\tfreeCfgDef(option);\n"
		"}\n\n"
		, name, name
	);

	fputs(
		"/************** Iterator **************/\n"
		"typedef enum IteratorState {\n"
		"\t_S_Initial = 0,\n"
		, fh );
	makeIteratorStates(fh, def);
	fputs(
		"\t_S_Finished\n"
		"} IteratorState;\n\n"
		, fh);

	fprintf( fh, "struct %s_iterator_t {\n\tIteratorState\tstate;\n" , name);
	makeArrayIndexes(fh, def);
	fprintf( fh, "};\n\n");

	fprintf(fh,
		"%s_iterator_t*\n"
		"%s_iterator_init() {\n"
		"\t%s_iterator_t *i = malloc(sizeof(*i));\n"
		"\tif (i == NULL) return NULL;\n"
		"\tmemset(i, 0, sizeof(*i));\n"
		"\treturn i;\n"
		"}\n\n"
		"char*\n"
		"%s_iterator_next(%s_iterator_t* i, %s *c, char **v) {\n"
		"\tstatic char\tbuf[PRINTBUFLEN];\n\n"
		"\t*v = NULL;\n"
		"\tgoto again; /* keep compiler quiet */\n"
		"again:\n"
		"\tswitch(i->state) {\n"
		"\t\tcase _S_Initial:\n"
		, name, name, name, name, name, name);
	makeSwitch(fh, def, NULL, 0, NULL);
	fprintf(fh,
		"\t\tcase _S_Finished:\n"
		"\t\t\tfree(i);\n"
		"\t\t\tbreak;\n"
		"\t\tdefault:\n"
		"\t\t\tout_warning(CNF_INTERNALERROR, \"Unknown state for %s_iterator_t: %cd\", i->state);\n"
		"\t\t\tfree(i);\n"
		"\t}\n"
		"\treturn NULL;\n"
		"}\n\n"
		, name, '%' );
		
	fprintf(fh, "/************** Checking of required fields  **************/\nint\ncheck_cfg_%s(%s *c) {\n", name, name);
	fprintf( fh, "\t%s_iterator_t iterator, *i = &iterator;\n" , name);
	fputs("\tint\tres = 0;\n\n", fh);

	makeCheck(fh, def, 0);

	fputs("\treturn res;\n}\n\n",  fh);

	fprintf(fh,
		"static void\n"
		"cleanFlags(%s* c, OptDef* opt) {\n"
		, name);
	fprintf(fh,
		"\t%s_iterator_t iterator, *i = &iterator;\n\n", name);
	makeCleanFlags(fh, def, 0);
	fputs(
		"}\n\n",
		fh
	);

	fputs("/************** Duplicate config  **************/\n\n", fh);
	fprintf(fh,
		"int\n"
		"dup_%s(%s* dst, %s* src) {\n"
		, name, name, name);
	fprintf(fh,
		"\t%s_iterator_t iterator, *i = &iterator;\n\n", name);
	makeDup(fh, def, 0);
	fputs("\n\treturn CNF_OK;\n", fh);
	fputs("}\n\n", fh);

	fputs("/************** Destroy config  **************/\n\n", fh);
	fprintf(fh,
		"void\n"
		"destroy_%s(%s* c) {\n"
		, name, name);
	fprintf(fh,
		"\t%s_iterator_t iterator, *i = &iterator;\n\n", name);
	makeDestroy(fh, def, 0);
	fputs("}\n\n", fh);

	fputs("/************** Compare config  **************/\n\n", fh);
	fprintf(fh,
		"int\n"
		"confetti_strcmp(char *s1, char *s2) {\n"
			"\tif (s1 == NULL || s2 == NULL) {\n"
				"\t\tif (s1 != s2)\n"
					"\t\t\treturn s1 == NULL ? -1 : 1;\n"
				"\t\telse\n"
					"\t\t\treturn 0;\n"
			"\t}\n\n"
			"\treturn strcmp(s1, s2);\n"
		"}\n\n");

	fprintf(fh,
		"char *\n"
		"cmp_%s(%s* c1, %s* c2, int only_check_rdonly) {\n"
		, name, name, name);
	fprintf(fh,
		"\t%s_iterator_t iterator1, iterator2, *i1 = &iterator1, *i2 = &iterator2;\n"
		"\tstatic char diff[PRINTBUFLEN];\n\n",
		name);
	makeCmp(fh, def, 0);
	fputs("\n\treturn 0;\n", fh);
	fputs("}\n\n", fh);
}
