#ifndef __LICENSE_H
    #define __LICENSE_H

#include <stdbool.h>
#include <stdint.h>
#include "minr.h"

bool is_spdx_license_identifier(char *src);
void mine_license(struct minr_job *job, char *id, bool license_file);
void generate_license_ids_c(char *path);

#endif
