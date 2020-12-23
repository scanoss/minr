// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/wfp.c
 *
 * Functions handling wfp extraction
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
#include <stdint.h>
#include <libgen.h>
#include <stdbool.h>
#include <fcntl.h>
#include <zlib.h>
#include "minr.h"
#include "md5.h"
#include "blacklist.h"
#include "winnowing.h"
#include "hex.h"
#include "wfp.h"


void extract_wfp(uint8_t *md5, char *src, int length, bool check_mz)
{
	/* File discrimination check: Unwanted header? */
	if (unwanted_header(src)) return;

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

/* Extracts wfps from the given mz file path */
void mz_wfp_extract(char *path)
{
	/* Open mz file */
	int mzfile = open(path, O_RDONLY);
	if (mzfile < 0)
	{
		printf("Cannot open file %s\n", path);
		exit(EXIT_FAILURE);
	}

	/* Extract first two MD5 bytes from the file name */
	uint8_t actual_md5[16] = "\0";
	uint8_t md5[16] = "\0";
	hex_to_bin(basename(path), 4, md5);

	/* Obtain file size */
	uint64_t size = lseek64(mzfile, 0, SEEK_END);
	if (!size)
	{
		printf("File %s is empty\n", path);
		exit(EXIT_FAILURE);
	}

	/* Read entire .mz file to memory */
	char *src = malloc(MAX_FILE_SIZE);
	uint8_t *mz = malloc(size);	
	lseek64 (mzfile, 0, SEEK_SET);
	if (!read(mzfile, mz, size)) printf("Warning: error reading %s\n", path);
	close(mzfile);

	/* Recurse mz contents */
	uint64_t ptr = 0;
	int corrupted = 0;
	while (ptr < size)
	{
		/* Read 14 remaining bytes of the MD5 */
		memcpy(md5 + 2, mz + ptr, 14);

		/* Read compressed data length */
		uint32_t tmpln;
		memcpy((uint8_t*)&tmpln, mz + ptr + 14, 4);

		/* Uncompress data */
		uint64_t zsrc_ln = tmpln;
		uint64_t src_ln = 1024 * 1024 * 4;

		/* Uncompress */
		if (Z_OK != uncompress((uint8_t *)src, &src_ln, mz + ptr + MZ_HEAD, zsrc_ln))
		{
			corrupted++;
		}

		else
		{
			/* Check resulting file integrity */
			calc_md5(src, src_ln - 1, actual_md5);
			if (memcmp(md5, actual_md5, 16))
			{
				printf("Record failed verification\n");
				exit(EXIT_FAILURE);
			}

			extract_wfp(md5, src, src_ln - 1, true);
		}

		/* Increment ptr */
		ptr += (MZ_HEAD + zsrc_ln);
	}
	if (corrupted) fprintf(stderr, "Warning! %d files corrupted\n", corrupted);
	close(mzfile);
	free(mz);
	free(src);
}

