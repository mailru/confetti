#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include <prscfl.h>

void
printPrefix(FILE *fh, int level) {
	int i;

	for(i=0; i < level*4; i++)
		fputc(' ', fh);
}

static void
dumpParamDef(FILE *fh, int level, ParamDef *def) {
	while(def) {
		if (def->comment) {
			ParamDef	*i = def->comment;
	
			fputc('\n', fh);
			while(i) {
				printPrefix(fh, level);
				fprintf(fh, "# %s\n", i->paramValue.commentval);
				i = i->next;
			}
		}
	
		printPrefix(fh, level);
		fprintf(fh, "%s = ", def->name);
	
		switch(def->paramType) {
			case	int32Type:
				fprintf(fh, "%"PRId32, def->paramValue.int32val);
				break;
			case	uint32Type:
				fprintf(fh, "%"PRIu32, def->paramValue.int32val);
				break;
			case	int64Type:
				fprintf(fh, "%"PRId64, def->paramValue.int64val);
				break;
			case	uint64Type:
				fprintf(fh, "%"PRIu64, def->paramValue.uint64val);
				break;
			case	doubleType:
				fprintf(fh, "%g", def->paramValue.doubleval);
				break;
			case	stringType:
				if ( def->paramValue.stringval) {
					char *ptr = def->paramValue.stringval;
					fputc('\"', fh);

					while(*ptr) {
						if (*ptr == '"')
							fputc('\\', fh);
						fputc(*ptr, fh);
						ptr++;
					}
	
					fputc('\"', fh);
				} else {
					fputs("NULL", fh);
				}
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				fputs("{\n", fh);
				dumpParamDef(fh, level+1, def->paramValue.structval);
				printPrefix(fh, level);
				fputs("}", fh);
				break;
			case	arrayType:
				fputs("[\n", fh);
				printPrefix(fh, level+1);
				fputs("{\n", fh);
				dumpParamDef(fh, level+2, def->paramValue.arrayval);
				printPrefix(fh, level+1);
				fputs("}\n", fh);
				printPrefix(fh, level);
				fputs("]", fh);
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}

		fputc('\n', fh);

		def = def->next;
	}
}

void 
fDump(FILE *fh, ParamDef *def) {
	ParamDef	root;

	root.paramType = structType;
	root.paramValue.structval = def;
	root.name = NULL;
	root.parent = NULL;
	root.next = NULL;
	def->parent = &root;

	dumpParamDef(fh, 0, def);
}
