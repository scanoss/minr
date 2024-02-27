#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include "minr_log.h"

static char log_file[FILENAME_MAX] = "\0";

void minr_log_path(char * path)
{
	strcpy(log_file, path);
}

/**
 * @brief Print the logs in stderr
 * @param fmt string to be printed  
 * @param ... //TODO
 */
void minr_log(const char *fmt, ...)
{
	if (!*log_file) return;

    va_list args;
    va_start(args, fmt);

	FILE *log = fopen(log_file, "a");
	if (!log)
	{
		fprintf(stderr, "Warning: Cannot access the log file\n");
		return;
	}
	/* Add entry to log */
	if (*fmt) 
        vfprintf(log, fmt, args);

	fclose(log);
	va_end(args);
}