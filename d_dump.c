#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include <prscfl.h>

static void
putt(int level) {
	while(level--)
		putchar('\t');
}

static void
debugParamDef(ParamDef *def, int level) {

	while(def) {
		putt(level);
		switch(def->paramType) {
			case	int32Type:
				printf("int32_t\t%s\n", def->name);
				break;
			case	uint32Type:
				printf("u_int32_t\t%s\n", def->name);
				break;
			case	int64Type:
				printf("int64_t\t%s\n", def->name);
				break;
			case	uint64Type:
				printf("u_int64_t\t%s\n", def->name);
				break;
			case	doubleType:
				printf("double\t%s\n", def->name);
				break;
			case	stringType:
				printf("char*\t%s\n", def->name);
				break;
			case	commentType:
				fprintf(stderr, "Unexpected comment"); 
				break;
			case	structType:
				printf("struct\t%s\n", def->name);
				debugParamDef(def->paramValue.structval, level+1);
				break;
			case	arrayType:
				printf("array\t%s\n", def->name);
				debugParamDef(def->paramValue.arrayval, level+1);
				break;
			case 	builtinType:
				printf("BUILTIN\n");
				break;
			default:
				fprintf(stderr,"Unknown paramType (%d)\n", def->paramType);
				exit(1);
		}
		def = def->next;
	}
}

void 
dDump(ParamDef *def) {
	debugParamDef(def, 0);
}
