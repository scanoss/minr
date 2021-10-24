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
#include <stdint.h>
#include <libgen.h>
#include <stdbool.h>
#include <fcntl.h>
#include <zlib.h>
#include "ldb.h"
#include "minr.h"
#include "md5.h"
#include "ignorelist.h"
#include "winnowing.h"
#include "hex.h"
#include "wfp.h"

void extract_wfp(uint8_t *md5, char *src, int length, bool check_mz)
{
	/* File discrimination check: Unwanted header? */
	if (unwanted_header(src)) return;

	/* Check if data is too square */
	if (too_much_squareness(src)) return;

	/* File discrimination check: Binary? */
	int src_ln = strlen(src);
	if (length != src_ln) return;

	/* Store buffer lengths */
	long buffer_ln[256];
	for (int i = 0; i < 256; i++) buffer_ln[i]=0;

	/* Capture hashes (Winnowing) */
	uint32_t size = winnowing(src, hashes, lines, MAX_FILE_SIZE);

	uint8_t n = 0;
	uint16_t line = 0;

	for (uint32_t i = 0; i < size; i++)
	{
		/* Copy remaining 3 bytes of the crc32 (inverting) */
		uint32_reverse((uint8_t *)&hashes[i]);
		n = *(uint8_t *)(&hashes[i]);

		memcpy(buffer + (BUFFER_SIZE * n) + buffer_ln[n], (char *) &hashes[i] + 1, 3);
		buffer_ln[n] += 3;

		/* Copy md5 hash (16 bytes) */
		memcpy(buffer + (BUFFER_SIZE * n) + buffer_ln[n], (char *) md5, 16);
		buffer_ln[n] += 16;

		/* Copy originating line number */
		line = (lines[i] > 65535) ? 65535 : lines[i];
		memcpy(buffer + (BUFFER_SIZE * n) + buffer_ln[n], (char *)&line, 2);
		buffer_ln[n] += 2;

		/* Flush buffer */
		if (buffer_ln[n] + 21 >= BUFFER_SIZE)
		{
			if (!write(out_snippet[n], buffer + (n * BUFFER_SIZE), buffer_ln[n]))
				printf("Warning: error writing snippet sector\n");
			buffer_ln[n] = 0;
		}
	}

	/* Flush buffer */
	for (int i = 0; i < 256; i++)
		if (buffer_ln[i]) if (!write(out_snippet[i], buffer + (BUFFER_SIZE * i), buffer_ln[i]))
				printf("Warning: error writing snippet sector\n");
}

bool mz_wfp_extract_handler(struct mz_job *job)
{
	/* Fill MD5 with item id */
	memcpy(job->ptr + 2, job->id, MZ_MD5);

	/* Decompress */
	mz_deflate(job);

	job->data[job->data_ln] = 0;
	extract_wfp(job->ptr, job->data, job->data_ln, true);

	return true;
}

/* Extracts wfps from the given mz file path */
void mz_wfp_extract(char *path)
{
	char *src = calloc(MAX_FILE_SIZE + 1, 1);
	uint8_t *zsrc = calloc((MAX_FILE_SIZE + 1) * 2, 1);
	uint8_t mzid[MD5_LEN] = "\0";
	uint8_t mzkey[MD5_LEN] = "\0";

	/* Create job structure */
	struct mz_job job;
	strcpy(job.path, path);
	memset(job.mz_id, 0, 2);
	job.mz = NULL;
	job.mz_ln = 0;
	job.id = mzid;
	job.ln = 0;
	job.data = src;        // Uncompressed data
	job.data_ln = 0;
	job.zdata = zsrc;      // Compressed data
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
	free(src);
	free(zsrc);
}
