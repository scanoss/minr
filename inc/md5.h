#ifndef __MD5_H
#define __MD5_H

#include <stdint.h>
#ifndef MD5
uint8_t *file_md5 (char *path);
void calc_md5(char *data, int size, uint8_t *out);
#endif
#endif