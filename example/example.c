#include <stdio.h>
#include <stdarg.h>

#include <my_product_cfg.h>

void
out_warning(char *format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int
main(int argc, char* argv[]) {
	my_product	cfg;

	fill_default_my_product(&cfg);

	if (argc > 1) {
		FILE *fh = fopen(argv[1], "r");

		if (!fh) {
			fprintf(stderr, "Could not open file %s\n", argv[1]);
			return 1;
		}

		parse_cfg_file_my_product(&cfg, fh);

		fclose(fh);
	}

	return 0;
}
