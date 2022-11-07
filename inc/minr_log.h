#ifndef __MINR_LOG_H
#define __MINR_LOG_H
#include <stdio.h>

extern char log_file[FILENAME_MAX];
void minr_log(const char *fmt, ...);

#endif