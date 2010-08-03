#include <stdarg.h>
#include <my_product_cfg.h>
#include <prscfg.h>


void
out_warning(char *format, ...) {
    va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

#include <prscfg.c>
#include <my_product_cfg.c>
