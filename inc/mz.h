#ifndef __MZ_H
#define __MZ_H

#include <stdint.h>
#include <stdbool.h>

bool mz_id_exists(uint8_t *mz, uint64_t size, uint8_t *id);
uint8_t *file_read(char *filename, uint64_t *size);
void mz_deflate(struct mz_job *job);
void mz_id_fill(char *md5, uint8_t *mz_id);
void mz_parse(struct mz_job *job, bool (*mz_parse_handler) ());
void file_write(char *filename, uint8_t *src, uint64_t src_ln);
void mz_id_fill(char *md5, uint8_t *mz_id);
void mz_deflate(struct mz_job *job);
void mz_corrupted(void);
void mz_add(uint8_t *md5, char *src, int src_ln, bool check, uint8_t *zsrc, struct mz_cache_item *mz_cache);
bool mz_check(char *path);
void mz_flush(struct mz_cache_item *mz_cache);
void mz_list(struct mz_job *job, char *path);
void mz_extract(struct mz_job *job, char *path);
void mz_optimise(struct mz_job *job, char *path);
void mz_mine_license(struct mz_job *job, char *path);
void mz_cat(struct mz_job *job, char *path, char *key);
#endif