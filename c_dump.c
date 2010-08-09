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
	fputs("c", fh);
	dumpStructName(fh, def, "->");
	fputs("->", fh);
	fputs(def->name, fh);
}

static void
dumpDefault(FILE *fh, ParamDef *def) {

	while(def) {

		fputs("\t", fh);
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
					fputs("\tif (", fh);
					dumpStructPath(fh, def);
					fputs(" == NULL) return -6;\n", fh);
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
				fputs("\tif (", fh);
				dumpStructPath(fh, def);
				fputs(" == NULL) return -6;\n", fh);
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
			
			fputs("static int\nacceptDefault", fh);
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

			/* restore parent */
			ptr = def->paramValue.arrayval;
			while(ptr) {
				ptr->parent = def;
				ptr = ptr->next;
			}

			fputs("\treturn 0;\n", fh);
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
dumpStructFullPath(FILE *fh, ParamDef *def, int innerCall, int isiterator) {
	if (def) {
		int n = dumpStructFullPath(fh, def->parent, 1, isiterator);
		fputs("->", fh);
		fputs(def->name, fh);
		if (def->paramType == arrayType && innerCall) {
			fputs("[", fh);
			if (isiterator) {
				fputs("i->idx", fh);
				dumpParamDefCName(fh, def);	
			} else
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
			if (def->rdonly) 
				fputs("\t\tif (check_rdonly)\n\t\t\treturn -3;\n", fh);
			fputs("\t\tARRAYALLOC(", fh);
			n = dumpStructFullPath(fh, def, 0, 0);
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
				if (def->rdonly) 
					fputs("\t\tif (check_rdonly)\n\t\t\treturn -3;\n", fh);
				fputs("\t\terrno = 0;\n", fh);
				switch(def->paramType) {
					case	int32Type:
						fputs("\t\tlong int i32 = strtol(opt->paramValue.numberval, NULL, 10);\n", fh);
						fputs("\t\tif (i32 == 0 && errno == EINVAL)\n\t\t\treturn -4;\n", fh);
						fputs("\t\tif ( (i32 == LONG_MIN || i32 == LONG_MAX) && errno == ERANGE)\n\t\t\treturn -5;\n", fh);
						fputs("\t\t", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" = i32;\n", fh);
						break;
					case	uint32Type:
						fputs("\t\tunsigned long int u32 = strtoul(opt->paramValue.numberval, NULL, 10);\n", fh);
						fputs("\t\tif (u32 == 0 && errno == EINVAL)\n\t\t\treturn -4;\n", fh);
						fputs("\t\tif ( u32 == ULONG_MAX && errno == ERANGE)\n\t\t\treturn -5;\n", fh);
						fputs("\t\t", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" = u32;\n", fh);
						break;
					case	int64Type:
						fputs("\t\tlong long int i64 = strtoll(opt->paramValue.numberval, NULL, 10);\n", fh);
						fputs("\t\tif (i64 == 0 && errno == EINVAL)\n\t\t\treturn -4;\n", fh);
						fputs("\t\tif ( (i64 == LLONG_MIN || i64 == LLONG_MAX) && errno == ERANGE)\n\t\t\treturn -5;\n", fh);
						fputs("\t\t", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" = i64;\n", fh);
						break;
					case	uint64Type:
						fputs("\t\tunsigned long long int u64 = strtoull(opt->paramValue.numberval, NULL, 10);\n", fh);
						fputs("\t\tif (u64 == 0 && errno == EINVAL)\n\t\t\treturn -4;\n", fh);
						fputs("\t\tif ( u64 == ULLONG_MAX && errno == ERANGE)\n\t\t\treturn -5;\n", fh);
						fputs("\t\t", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" = u64;\n", fh);
						break;
					case	doubleType:
						fputs("\t\t", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" = strtod(opt->paramValue.numberval, NULL);\n", fh); 
						fputs("\t\tif ( (", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" == 0 || ", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" == -HUGE_VAL || ", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" == HUGE_VAL) && errno == ERANGE)\n\t\t\treturn -5;\n", fh);
						break;
					case	stringType:
						fputs("\t\t", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" = (opt->paramValue.stringval) ? strdup(opt->paramValue.stringval) : NULL;\n", fh);
						fputs("\t\t if (opt->paramValue.stringval && ", fh);
							dumpStructFullPath(fh, def, 1, 0);
							fputs(" == NULL)\n\t\t\treturn -6;\n", fh);
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
				makeIteratorStates(fh, def->paramValue.arrayval);
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
				makeArrayIndexes(fh, def->paramValue.arrayval);
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
makeSwitchArrayList(FILE *fh, ParamDef *def, int level) {
	while(def) {
		switch(def->paramType) {
			case	int32Type:
			case	uint32Type:
			case	int64Type:
			case	uint64Type:
			case	doubleType:
			case	stringType:
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
				makeSwitchArrayList(fh, def->paramValue.arrayval, level);
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
			fputt(fh, level+1); fputs("out_warning(\"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%\"PRId32, ", fh);
				dumpStructFullPath(fh, def, 0, 1);
				fputs(");\n", fh);
			break;
		case	uint32Type:
			fputt(fh, level); fputs("*v = malloc(32);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(\"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%\"PRIu32, ", fh);
				dumpStructFullPath(fh, def, 0, 1);
				fputs(");\n", fh);
			break;
		case	int64Type:
			fputt(fh, level); fputs("*v = malloc(32);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(\"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%\"PRId64, ", fh);
				dumpStructFullPath(fh, def, 0, 1);
				fputs(");\n", fh);
			break;
		case	uint64Type:
			fputt(fh, level); fputs("*v = malloc(32);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(\"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%\"PRIu64, ", fh);
				dumpStructFullPath(fh, def, 0, 1);
				fputs(");\n", fh);
			break;
		case	doubleType:
			fputt(fh, level); fputs("*v = malloc(32);\n", fh);
			fputt(fh, level); fputs("if (*v == NULL) {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(\"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
			fputt(fh, level); fputs("sprintf(*v, \"%g\", ", fh);
				dumpStructFullPath(fh, def, 0, 1);
				fputs(");\n", fh);
			break;
		case	stringType:
			fputt(fh, level); fputs("*v = (", fh);
				dumpStructFullPath(fh, def, 0, 1);
				fputs(") ? strdup(", fh);
				dumpStructFullPath(fh, def, 0, 1);
				fputs(") : NULL;\n", fh);
			fputt(fh, level); fputs("if (*v == NULL && ", fh);
				dumpStructFullPath(fh, def, 0, 1);
				fputs(") {\n", fh);
			fputt(fh, level+1); fputs("free(i);\n",fh); 
			fputt(fh, level+1); fputs("out_warning(\"No memory to output value\");\n", fh);
			fputt(fh, level+1); fputs("return NULL;\n",fh); 
			fputt(fh, level); fputs("}\n",fh); 
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
				resetSubArray(fh, def->paramValue.arrayval, level);
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
		int n = dumpStructNameFullPath(fh, def->parent, 1);
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
		int n = dumpArrayIndexes(fh, def->parent, 1);
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
						resetSubArray(fh, parent->paramValue.arrayval, level);
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
				makeSwitchArrayList(fh, def->paramValue.arrayval, level);
				fputt(fh, level+3); fputs("if (", fh);
				dumpStructFullPath(fh, def, 0, 1);
					fputs(" && ", fh);
					dumpStructFullPath(fh, def, 0, 1);
					fputs("[i->idx", fh);
					dumpParamDefCName(fh, def);
					fputs("]) {\n", fh);
				fputt(fh, level+4); fputs("switch(i->state) {\n", fh);
				fputt(fh, level+5); fputs( "case S", fh);
				dumpParamDefCName(fh, def);
				fputs(":\n", fh);
				makeSwitch(fh, def->paramValue.arrayval, def, level+3, NULL);
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
					resetSubArray(fh, parent->paramValue.arrayval, level+1);
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
		"int\n"
		"fill_default_%s(%s *c) {\n"
		, name, name
	);

	dumpDefault(fh, def);
	fputs("\treturn 0;\n}\n\n", fh);

	dumpDefaultArray(fh, name, def);

	dumpParamDefCNameRecursive(fh, def);

	fputs(
		"\n"
		"#define ARRAYALLOC(x,n,t)  do {                                     \\\n"
		"   int l = 0, ar;                                                   \\\n"
		"   __typeof__(x) y = (x), t;                                        \\\n"
		"   if ( (n) <= 0 ) return -2; /* wrong index */                     \\\n"
		"   while(y && *y) {                                                 \\\n"
		"       l++; y++;                                                    \\\n"
		"   }                                                                \\\n"
		"   if ( (n) >= l ) {                                                \\\n"
		"      if ( (x) == NULL )                                            \\\n"
		"          t = y = malloc( ((n)+1) * sizeof( __typeof__(*(x))) );    \\\n" 
		"      else {                                                        \\\n"
		"          t = realloc((x), ((n)+1) * sizeof( __typeof__(*(x))) );   \\\n"
		"          y = t + l;                                                \\\n"
		"      }                                                             \\\n"
		"      if (t == NULL)  return -6;                                    \\\n"
		"      (x) = t;                                                      \\\n"
		"      memset(y, 0, (((n)+1) - l) * sizeof( __typeof__(*(x))) );     \\\n"
		"      while ( y - (x) < (n) ) {                                     \\\n"
		"          *y = malloc( sizeof( __typeof__(**(x))) );                \\\n"
		"          if (*y == NULL)  return -6;                               \\\n"
		"          if ( (ar = acceptDefault##t(*y)) != 0 ) return ar;        \\\n"
		"          y++;                                                      \\\n"
		"      }                                                             \\\n"
		"   }                                                                \\\n"
		"} while(0)\n\n"
		, fh
	);

	fprintf(fh, 
		"static int\n"
		"acceptValue(%s* c, OptDef* opt, int check_rdonly) {\n\n"
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
		"static void\n"
		"acceptCfgDef(%s *c, OptDef *opt, int check_rdonly, int *n_accepted, int *n_skipped) {\n"
		"\tint	r;\n\n"
		"\tif (n_accepted) *n_accepted=0;\n"
		"\tif (n_skipped) *n_skipped=0;\n"
		"\n"
		"\twhile(opt) {\n"
		"\t\tr = acceptValue(c, opt, check_rdonly);\n"
		"\t\tswitch(r) {\n"
		"\t\t\tcase 1:\n"
		"\t\t\t\tout_warning(\"Could not find '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase 0:\n"
		"\t\t\t\tif (n_accepted) (*n_accepted)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase -1:\n"
		"\t\t\t\tout_warning(\"Wrong value type for '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase -2:\n"
		"\t\t\t\tout_warning(\"Wrong array index in '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase -3:\n"
		"\t\t\t\tout_warning(\"Could not accept read only '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase -4:\n"
		"\t\t\t\tout_warning(\"Could not parse integer value for '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase -5:\n"
		"\t\t\t\tout_warning(\"Wrong range for '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tcase -6:\n"
		"\t\t\t\tout_warning(\"Not enough memory to accept '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t\tdefault:\n"
		"\t\t\t\tout_warning(\"Unknown error for '%cs' option\", dumpOptDef(opt->name));\n"
		"\t\t\t\tif (n_skipped) (*n_skipped)++;\n"
		"\t\t\t\tbreak;\n"
		"\t\t}\n\n"
		"\t\topt = opt->next;\n"
		"\t}\n"
		"}\n\n"
		, name, '%', '%', '%', '%', '%', '%', '%', '%'
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
		"\t\t\tout_warning(\"Unknown state for %s_iterator_t: %cd\", i->state);\n"
		"\t\t\tfree(i);\n"
		"\t}\n"
		"\treturn NULL;\n"
		"}\n\n"
		, name, '%' );
}
