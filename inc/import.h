#ifndef __IMPORT_H
#define __IMPORT_H

#include <stdbool.h>

#define DECODE_BASE64 8
int (*decode) (int op, unsigned char *key, unsigned char *nonce,
		        const char *buffer_in, int buffer_in_len, unsigned char *buffer_out);

void mined_import(struct minr_job *job);

#endif
