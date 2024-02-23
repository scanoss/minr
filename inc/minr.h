#ifndef __MINR_H
    #define __MINR_H
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/minr.h
 *
 * Global minr declarations
 *
 * Copyright (C) 2018-2021 SCANOSS.COM
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
#define MINR_VERSION "2.6.0"
#define FILE_FILES 256
#define MAX_ARG_LEN 1024
#define MIN_FILE_REC_LEN 70
#define MAX_PATH_LEN 4096
#define MAX_FILE_SIZE (8 * 1048576)
#define MAX_FILE_HEADER 4096
#define MAX_HEADER_LINES 30
#define MAX_CSV_LINE_LEN 1024
#define MIN_FILE_SIZE 256 // files below this size will be ignored
#define DISCARD_PATH_IF_LONGER_THAN 1024
#define MD5_LEN 16
#define MD5_LEN_HEX 32
#define NODE_PTR_LEN 5
#define REC_SIZE_LEN 2
#define LDB_KEY_LN 4
#define MAX_COPYRIGHT_LEN 256
#define MAX_NONALNUM_IN_COPYRIGHT 10 // Maximum amount of non alphanumeric bytes accepted in a copyright statement
#define MIN_ALNUM_IN_COPYRIGHT 3 // Minimum amount of alphanumeric bytes required in a copyright statement

/* Set best practices to twice the average (calculated over entire OSSKB) */
#define BEST_PRACTICES_MAX_LINES (210 * 2)
#define BEST_PRACTICES_MAX_LINE_LN (36 * 2)
#define BEST_PRACTICES_MAX_LINES_PER_COMMENT (27 * 2)

#define MAX_LICENSE_ID 64
#define MAX_LICENSE_TEXT 1024

#define TABLE_NAME_FILE "file"
#define TABLE_NAME_URL "url"
#define TABLE_NAME_LICENSE "license"
#define TABLE_NAME_CRYPTOGRAPHY "cryptography"
#define TABLE_NAME_QUALITY "quality"
#define TABLE_NAME_DEPENDENCY "dependency"
#define TABLE_NAME_COPYRIGHT "copyright"
#define TABLE_NAME_ATTRIBUTION "attribution"
#define TABLE_NAME_NOTICES	"notices"
#define TABLE_NAME_VULNERABILITY "vulnerability"
#define TABLE_NAME_WFP "wfp"
#define TABLE_NAME_PURL "purl"
#define TABLE_NAME_SOURCES "sources"
#define TABLE_NAME_PIVOT "pivot"


#define LDB_ROOT "/var/lib/ldb"

/* Structures */
typedef struct normalized_license
{
  char spdx_id[MAX_LICENSE_ID];
  char text[MAX_LICENSE_TEXT];
  int ln;
} normalized_license;

struct minr_job
{
	uint8_t md5[MD5_LEN];
	char fileid[MD5_LEN * 2 + 1];

	uint8_t purl_md5[MD5_LEN];    // purl md5
	char purlid[MD5_LEN * 2 + 1]; // purl md5 hex

	uint8_t version_md5[MD5_LEN]; // purl@version md5
	char versionid[MD5_LEN * 2 + 1]; // purl@version md5 hex

	char path[MAX_PATH_LEN];
	bool is_attribution_notice;
	char url[MAX_ARG_LEN];
	char urlid[MD5_LEN * 2 + 1];
	char download_url[MAX_ARG_LEN]; // Manually specified download URL (-U)
	char metadata[MAX_ARG_LEN];
	char license[MAX_ARG_LEN];    // Declared url license
	char mined_path[MAX_ARG_LEN]; // Location of output mined/ directory
	char mined_extra_path[MAX_ARG_LEN]; // Location of output mined/extra directory
	char tmp_dir[MAX_ARG_LEN];    // Temporary directory for decompressing files
	bool all_extensions;
	bool exclude_mz;
	bool exclude_detection;
	bool scancode_mode;
	char local_mining;
	
	// minr -i
	char dbname[MAX_PATH_LEN];
	char import_path[MAX_PATH_LEN];
	char import_table[MAX_PATH_LEN];
	bool import_overwrite;
	bool skip_sort; // Do not sort before importing
	bool skip_csv_check; // Do not check number of CSV fields
	bool skip_delete; // Do not delete, -k(eep) files after importing
	bool mine_all;
	bool bin_import;

	// minr -f -t
	char join_from[MAX_PATH_LEN];
	char join_to[MAX_PATH_LEN];

	// minr -z
	char mz[MAX_PATH_LEN];

	// Memory allocation
	char *src; // for uncompressed source
	uint8_t *zsrc; // for compressed source
	uint64_t src_ln;
	uint64_t zsrc_ln;
	struct mz_cache_item *mz_cache; // for mz cache
	struct mz_cache_item * mz_cache_extra;
	normalized_license *licenses; // Array of known license identifiers
	int license_count;            // Number of known license identifiers

	FILE **out_file;
	FILE **out_file_extra;
	FILE * out_pivot;
	FILE *out_pivot_extra;

};

typedef enum { none, license, copyright, quality } metadata;


/* File descriptor arrays */
extern char tmp_path[MAX_ARG_LEN];
extern int min_file_size;
extern char *ATTRIBUTION_NOTICES[];

bool check_dependencies(void);
bool download(struct minr_job *job);
void recurse(struct minr_job *job, char *path);
void minr_join(struct minr_job *job);
void minr_join_mz(char * table, char *source, char *destination, bool skip_delete, bool encrypted);
void mine_license(struct minr_job *job, char *id, bool license_file);
bool mine_license_exec(struct minr_job *job);
void mine_copyright(char *mined_path, char *md5, char *src, uint64_t src_ln, bool license_file);
void mine_quality(char *mined_path, char *md5, char *src, long size);
normalized_license *load_licenses(int *count);
void mine_local_directory(struct minr_job *job, char* root);
void mine_local_file(struct minr_job *job, char *path);
void extract_csv(char *out, char *in, int n, long limit);
int count_chr(char chr, char *str);
int load_file(struct minr_job *job, char *path);
void print_md5(uint8_t *md5);
bool check_file_extension(char * path, bool bin_mode);
#endif
