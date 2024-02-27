// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/wfp.c
 *
 * Functions handling wfp extraction
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

/**
  * @file wfp.c.c
  * @date 28 Oct 2021 
  * @brief ???
  */

#include <stdint.h>
#include <libgen.h>
#include <stdbool.h>
#include <fcntl.h>
#include <zlib.h>
#include "ldb.h"
#include "minr.h"
#include "ignorelist.h"
#include "winnowing.h"
#include "hex.h"
#include "wfp.h"
#include "file.h"
#include "mz.h"

int *out_snippet;

void wfp_init(char * base_path)
{
	if (base_path)
		out_snippet = open_snippet(base_path);
}

void wfp_free(void)
{
	if (out_snippet)
	{
		/* Close files */
		for (int i=0; i < 256; i++) 
			close(out_snippet[i]);
	}	
	free(out_snippet);
}

#define WFP_BUFFER_SIZE 1048576

/**
 * @brief Extrac wfp from a surce
 * 
 * @param md5 file md5
 * @param src data source
 * @param length data lenght
 */
void extract_wfp(uint8_t *md5, char *src, uint32_t length, bool check_mz)
{
	/* File discrimination check: Unwanted header? */
	if (unwanted_header(src)) return;

	/* Check if data is too square */
	if (too_much_squareness(src)) return;

	/* File discrimination check: Binary? */
	uint32_t src_ln = strlen(src);

	if (length != src_ln || !strchr(src, '\n')) return;

	/* Store buffer lengths */
	long buffer_ln[256];
	for (int i = 0; i < 256; i++) 
		buffer_ln[i]=0;
	
	uint32_t mem_alloc =  src_ln > MAX_FILE_SIZE ? src_ln : MAX_FILE_SIZE;

	uint8_t *buffer = malloc(WFP_BUFFER_SIZE * 256);
	uint32_t *hashes = malloc(mem_alloc);
	uint32_t *lines = malloc(mem_alloc);

	if (!buffer || !hashes || !lines)
	{
		free(buffer);
		free(hashes);
		free(lines);
		return;
	}

	/* Capture hashes (Winnowing) */
	uint32_t size = winnowing(src, hashes, lines, mem_alloc);

	uint8_t n = 0;
	uint16_t line = 0;

	for (uint32_t i = 0; i < size; i++)
	{
		/* Copy remaining 3 bytes of the crc32 (inverting) */
		uint32_reverse((uint8_t *)&hashes[i]);
		n = *(uint8_t *)(&hashes[i]);

		memcpy(buffer + (WFP_BUFFER_SIZE * n) + buffer_ln[n], (char *) &hashes[i] + 1, 3);
		buffer_ln[n] += 3;

		/* Copy md5 hash (16 bytes) */
		memcpy(buffer + (WFP_BUFFER_SIZE * n) + buffer_ln[n], (char *) md5, 16);
		buffer_ln[n] += 16;

		/* Copy originating line number */
		line = (lines[i] > 65535) ? 65535 : lines[i];
		memcpy(buffer + (WFP_BUFFER_SIZE * n) + buffer_ln[n], (char *)&line, 2);
		buffer_ln[n] += 2;

		/* Flush buffer */
		if (buffer_ln[n] + 21 >= WFP_BUFFER_SIZE)
		{
			if (!write(out_snippet[n], buffer + (n * WFP_BUFFER_SIZE), buffer_ln[n]))
				printf("Warning: error writing snippet sector\n");
			buffer_ln[n] = 0;
		}
	}

	/* Flush buffer */
	for (int i = 0; i < 256; i++)
		if (buffer_ln[i]) if (!write(out_snippet[i], buffer + (WFP_BUFFER_SIZE * i), buffer_ln[i]))
				printf("Warning: error writing snippet sector\n");
	
	free (buffer);
	free (hashes);
	free (lines);
}

/**
 * @brief MZ wfp extract handler
 * 
 * @param job pointer mz job
 * @return true 
 */
bool mz_wfp_extract_handler(struct mz_job *job)
{
	/* Fill MD5 with item id */
	memcpy(job->ptr + 2, job->id, MZ_MD5);

	/* Decompress */
	MZ_DEFLATE(job);
	job->data[job->data_ln] = 0;
	extract_wfp(job->ptr, job->data, job->data_ln, true);
	free(job->data);

	return true;
}

/**
 * @brief Extracts wfps from the given mz file path 
 * 
 * @param path path to mz file
 */
void mz_wfp_extract(char *path)
{
	uint8_t mzid[MD5_LEN] = "\0";
	uint8_t mzkey[MD5_LEN] = "\0";

	/* Create job structure */
	struct mz_job job;
	memset(&job, 0, sizeof(job));
	strcpy(job.path, path);
	memset(job.mz_id, 0, 2);
	job.mz = NULL;
	job.mz_ln = 0;
	job.id = mzid;
	job.ln = 0;
	job.data = NULL;        // Uncompressed data
	job.data_ln = 0;
	job.zdata = NULL;      // Compressed data
	job.zdata_ln = 0;
	job.md5[MD5_LEN * 2 + 1] = 0;
	job.key = NULL;
	job.ptr = mzkey;

	/* Extract first two MD5 bytes from the file name */
	memcpy(job.md5, basename(job.path), 4);
	ldb_hex_to_bin(job.md5, 4, job.ptr);

	/* Read source mz file into memory */
	job.mz = file_read(job.path, &job.mz_ln);

	/* Launch wfp extraction */
	mz_parse(&job, mz_wfp_extract_handler);
	free(job.mz);
}
