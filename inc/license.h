#ifndef __LICENSE_H
    #define __LICENSE_H

#include <stdbool.h>

bool is_spdx_license_identifier(char *src);
void mine_license(char *md5, char *src, uint64_t src_ln);
void generate_license_ids_c(char *path);

#endif