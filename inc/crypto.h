#ifndef __CRYPTO_H
    #define __CRYPTO_H

#include <stdbool.h>
#include <stdint.h>
void mine_crypto(char *mined_path, char *md5, char *src, uint64_t src_ln);
void load_crypto_definitions(void);
#endif
