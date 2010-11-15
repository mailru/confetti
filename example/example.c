#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <prscfg.h>
#include <my_product_cfg.h>

static int useStdout = 0;
void
out_warning(ConfettyError r __attribute__ ((unused)), char *format, ...) {
    va_list args;

	va_start(args, format);
	vfprintf(useStdout ? stdout : stderr, format, args);
	va_end(args);
	if (useStdout)
		fputc('\n', stdout);
}

int
main(int argc, char* argv[]) {
	my_product	cfg;
	my_product_iterator_t	*i;
	char		*key, *value;

	fill_default_my_product(&cfg);

	if (argc > 1) {
		int			nAccepted, nSkipped;
		FILE *fh = fopen(argv[1], "r");

		if (!fh) {
			fprintf(stderr, "Could not open file %s\n", argv[1]);
			return 1;
		}

		parse_cfg_file_my_product(&cfg, fh, 0, &nAccepted, &nSkipped);

		printf("==========Accepted: %d; Skipped: %d===========\n", nAccepted, nSkipped);

		fclose(fh);
	}

	i = my_product_iterator_init();
	while ( (key = my_product_iterator_next(i, &cfg, &value)) != NULL ) {
		if (value) {
			printf("%s => '%s'\n", key, value);
			free(value);
		} else {
			printf("%s => (null)\n", key);
		}
	}

	printf("==========Check===========\n");
	useStdout = 1;
	printf("Missed required: %d\n", check_cfg_my_product(&cfg));

	

	return 0;
}
