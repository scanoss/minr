#ifndef __FILE_H
    #define __FILE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


//void file_md5(char *filepath, uint8_t *md5_result);
uint64_t get_file_size(char *path);
void read_file(char *out, char *path, uint64_t maxlen);
bool is_file(char *path);
bool is_dir(char *path);
uint64_t file_size(char *path);
int *open_snippet (char *base_path);
bool not_a_dot (char *path);
bool create_dir(char *path);
bool valid_path(char *dir, char *file);
bool check_disk_free(char *file, uint64_t needed);
FILE **open_file ();
#endif