#ifndef __MINR_IMPORT_H
#define __MINR_IMPORT_H

#include <stdbool.h>

#define DECODE_BASE64 8
extern int (*decode) (int op, unsigned char *key, unsigned char *nonce,
		        const char *buffer_in, int buffer_in_len, unsigned char *buffer_out);

void mined_import(struct minr_job *job);

#endif
