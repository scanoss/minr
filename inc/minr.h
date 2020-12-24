#ifndef __MINR_H
    #define _MINR_H
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/minr.h
 *
 * Global minr declarations
 *
 * Copyright (C) 2018-2020 SCANOSS.COM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Definitions */
#define MINR_VERSION "2.0.6"
#define MZ_CACHE_SIZE 16384
#define MZ_FILES 65536
#define MZ_HEAD 18 // Head contains 14 bytes of the MD5 + 4 bytes for compressed SIZE
#define MZ_MD5 14
#define MZ_SIZE 4
#define FILE_FILES 256
#define MAX_ARG_LEN 1024
#define MIN_FILE_REC_LEN 70
#define MAX_PATH_LEN 4096
#define MAX_FILE_SIZE (4 * 1048576)
#define MAX_FILE_HEADER 4096
#define BUFFER_SIZE 1048576
#define MIN_FILE_SIZE 64 // files below this size will be ignored
#define DISCARD_PATH_IF_LONGER_THAN 1024
#define MD5_LEN 16
#define MD5_LEN_HEX 32
#define NODE_PTR_LEN 5
#define REC_SIZE_LEN 2
#define LDB_KEY_LN 4
#define MAX_COPYRIGHT_LEN 256
#define MAX_NONALNUM_IN_COPYRIGHT 10 // Maximum amount of non alphanumeric bytes accepted in a copyright statement

/* Set best practices to twice the average (calculated over entire OSSKB) */
#define BEST_PRACTICES_MAX_LINES (210 * 2)
#define BEST_PRACTICES_MAX_LINE_LN (36 * 2)
#define BEST_PRACTICES_MAX_LINES_PER_COMMENT (27 * 2)

/* Structures */
struct mz_cache_item
{
	uint16_t length;
	uint8_t data[MZ_CACHE_SIZE];
}; 

struct mz_job
{
	char *path;        // Path to mz file
	uint8_t *mz;       // Pointer to entire mz file contents
	uint64_t mz_ln;    // MZ file length
	uint8_t *id;       // MZ record ID
	uint64_t ln;       // MZ record length
	char md5[33];      // MZ record hex ID (MD5)
	char *data;        // Pointer to uncompressed data
	uint64_t data_ln;  // Uncompressed data length
	uint8_t *zdata;    // Pointer to compressed data
	uint64_t zdata_ln; // Compressed data length
	void *ptr;         // Pointer to temporary data
	uint64_t ptr_ln;   // Temporary data length
	uint32_t dup_c;    // Duplicated counter
	uint32_t bll_c;    // Blacklisted counter
	uint32_t orp_c;    // Orphan file counter
	bool check_only;   // Perform only an mz validation (without list output)
    bool orphan_rm;
	uint8_t *key;      // File key to be printed via STDOUT (-k)
};

typedef enum { none, license, copyright, quality } metadata;

/* Pointers */
uint8_t *buffer;
uint32_t *hashes, *lines;

/* Paths */
extern char mined_path[MAX_ARG_LEN];
extern char tmp_path[MAX_PATH_LEN];

/* File descriptor arrays */
FILE *out_component;
FILE **out_file;
int *out_snippet;

extern int min_file_size;

bool download(char *tmp_component, char *url, char *md5);
void recurse(char *component_record, char *tmp_component, char *tmp_dir, bool all_extensions, bool exclude_mz, char* urlid, char *src, uint8_t *zsrc, struct mz_cache_item *mz_cache);
bool check_dependencies(void);
void minr_join(char *source, char *destination);

#endif