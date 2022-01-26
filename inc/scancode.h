#ifndef __SCANCODE_H
    #define __SCANCODE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minr.h"

bool scancode_prepare_tmp_dir(char * id);
bool scancode_remove_tmp_dir(char *id);
bool scancode_copy_to_tmp(char *path, char *id);
bool scancode_run(char * id, char *csv_file);
bool scancode_check(void);
#endif