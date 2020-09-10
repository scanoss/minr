// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/global.c
 *
 * Global declarations
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

/* Definitions */
#define MINR_VERSION "1.15"
#define MZ_CACHE_SIZE 32768
#define MZ_FILES 65536
#define MZ_HEAD 18 // Head contains 14 bytes of the MD5 + 4 bytes for compressed SIZE
#define MZ_MD5 14
#define MZ_SIZE 4
#define FILE_FILES 256
#define MAX_ARG_LEN 1024
#define MAX_PATH_LEN 4096
#define MAX_FILE_SIZE (4 * 1048576)
#define BUFFER_SIZE 1048576
#define MIN_FILE_SIZE 3 // files below this size will be ignored

/* Structures */
struct mz_cache_item
{
	uint16_t length;
	uint8_t data[MZ_CACHE_SIZE];
}; 

/* Pointers */
uint8_t *buffer;
uint32_t *hashes, *lines;

/* Paths */
char mined_path[MAX_ARG_LEN] = ".";
char tmp_path[MAX_PATH_LEN] = "/dev/shm";

/* File descriptor arrays */
FILE *out_component;
FILE **out_file;
int *out_snippet;

int min_file_size = MIN_FILE_SIZE;
