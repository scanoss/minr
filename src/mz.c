// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/mz.c
 *
 * SCANOSS .mz archive handling functions
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

/* Read data to memory and return a pointer. Also, update size */
#define FILE_OFFSET_BITS =64
#define __USE_LARGEFILE64

#include "minr.h"
#include <fcntl.h>
#include <zlib.h>
#include <stdio.h>
#include "mz.h"
#include <sys/types.h>
#include <unistd.h>
#include "hex.h"
#include "file.h"
//class fseeko64;
uint8_t *file_read(char *filename, uint64_t *size)
{
	FILE *f = fopen(filename, "r");
	if (!f)
	{
		printf("\nCannot open %s for writing\n", filename);
		exit(EXIT_FAILURE);
	}

	fseeko64(f, 0, SEEK_END);
	*size = ftello64(f);
	fseeko64(f, 0, SEEK_SET);

	uint8_t *tmp = calloc(*size, 1);
	fread(tmp, *size, 1, f);
	fclose(f);

	return tmp;
}

/* Write data to file */
void file_write(char *filename, uint8_t *src, uint64_t src_ln)
{
	FILE *out = fopen(filename, "w");
	if (!out)
	{
		printf("\nCannot open %s for writing\n", filename);
		exit(EXIT_FAILURE);
	}
	fwrite(src, 1, src_ln, out);
	fclose(out);
}

/* Searches for a 14-byte ID in *mz */
bool mz_id_exists(uint8_t *mz, uint64_t size, uint8_t *id)
{
	/* Recurse mz contents */
	uint64_t ptr = 0;
	while (ptr < size)
	{
		/* Position pointers */
		uint8_t *file_id = mz + ptr;
		uint8_t *file_ln = file_id + MZ_MD5;

		/* Compare id */
		if (!memcmp(file_id, id, MZ_MD5)) return true;

		/* Get compressed data size */
		uint32_t tmpln;
		memcpy((uint8_t*)&tmpln, file_ln, MZ_SIZE);

		/* Increment pointer */
		ptr += tmpln + MZ_MD5 + MZ_SIZE;
	}
	return false;
}

/* Optimizes an mz archive, eliminating duplicates and unwanted content */
void mz_parse(struct mz_job *job, bool (*mz_parse_handler) ())
{
	/* Recurse mz contents */
	uint64_t ptr = 0;
	while (ptr < job->mz_ln)
	{
		/* Position pointers */
		job->id = job->mz + ptr;
		uint8_t *file_ln = job->id + MZ_MD5;
		job->zdata = file_ln + MZ_SIZE;

		/* Get compressed data size */
		uint32_t tmpln;
		memcpy((uint8_t*)&tmpln, file_ln, MZ_SIZE);
		job->zdata_ln = tmpln;

		/* Get total mz record length */
		job->ln = MZ_MD5 + MZ_SIZE + job->zdata_ln;

		/* Pass job to handler */
		if (!mz_parse_handler(job)) return;

		/* Increment pointer */
		ptr += job->ln;
		if (ptr > job->mz_ln)
		{
			printf("%s integrity failed\n", job->path);
			exit(EXIT_FAILURE);
		}
	}
}

bool mz_exists_in_cache(uint8_t *md5, struct mz_cache_item *mz_cache)
{
	int mzid = uint16(md5);

	/* False if cache is empty */
	if (!mz_cache[mzid].length) return false;

	/* Search md5 in cache */
	uint8_t *cache = mz_cache[mzid].data;
	int cacheln = mz_cache[mzid].length;

	while (cache < (mz_cache[mzid].data + cacheln))
	{
		/* MD5 comparison starts on the third byte of the MD5 */
		if (!memcmp(md5 + 2, cache, MZ_MD5)) return true;

		/* Extract zsrc and displace cache pointer */
		uint32_t zsrc_ln;
		memcpy((uint8_t*)&zsrc_ln, cache + MZ_MD5, MZ_SIZE);
		cache += (MZ_HEAD + zsrc_ln);
	}

	return false;
}

bool mz_exists_in_disk(uint8_t *md5)
{
	char path[MAX_PATH_LEN];

	/* Assemble MZ path */
	uint16_t mzid = uint16(md5);
	sprintf(path, "%s/sources/%04x.mz", mined_path, mzid);

	/* Open mz file */
	int mz = open(path, O_RDONLY);
	if (mz < 0) return false;

	uint64_t ptr = 0;
	uint8_t header[MZ_HEAD];

	/* Get file size */
	uint64_t size = lseek64(mz, 0, SEEK_END);
	if (!size) return false;

	/* Recurse file */
	while (ptr < size)
	{
		/* Read MZ header */
		lseek64(mz, ptr, SEEK_SET);
		if (!read(mz, header, MZ_HEAD))
		{
			printf("[READ_ERROR]\n");
			exit(EXIT_FAILURE);
		}
		ptr += MZ_HEAD;

		/* If md5 matches, exit with true */
		if (!memcmp(md5 + 2, header, MZ_MD5))
		{
			close(mz);
			return true;
		}

		/* Increment pointer */
		uint32_t zsrc_ln;
		memcpy((uint8_t*)&zsrc_ln, header + MZ_MD5, MZ_SIZE);
		ptr += zsrc_ln;
	}
	close(mz);

	return false;
}


bool mz_exists(uint8_t *md5, struct mz_cache_item *mz_cache)
{
	if (mz_exists_in_cache(md5, mz_cache)) return true;

	return mz_exists_in_disk(md5);
}

void mz_write(int mzid, uint8_t *data, int datalen)
{
	char path[MAX_PATH_LEN];

	/* Create sources directory */
	sprintf(path, "%s/sources", mined_path);
	if (!create_dir(path))
	{
		printf("Cannot create file %s\n", path);
	}

	else
	{
		sprintf(path, "%s/sources/%04x.mz", mined_path, mzid);
		FILE *f = fopen(path, "a");
		if (f)
		{
			size_t written = fwrite(data, datalen, 1, f);
			if (!written)
			{
				printf("Error writing %s\n", path);
				exit(EXIT_FAILURE);
			}
			fclose(f);
		}
		else 
		{
			printf("Error opening %s\n", path);
			exit(EXIT_FAILURE);
		}
	}
}

/* Adds a file to the mz archive. File structure is a series of:
   MD5(14) + COMPRESSED_SRC_SIZE(4) + COMPRESSED_SRC(N)
   The first two bytes of the md5 are in the actual XXXX.mz filename
   */
void mz_add(uint8_t *md5, char *src, int src_ln, bool check, uint8_t *zsrc, struct mz_cache_item *mz_cache)
{
	if (check) if (mz_exists(md5, mz_cache)) return;

	uint64_t zsrc_ln = compressBound(src_ln + 1);

	/* We save the first bytes of zsrc to accomodate the MZ header */
	compress(zsrc + MZ_HEAD, &zsrc_ln, (uint8_t *) src, src_ln + 1);
	uint32_t zln = zsrc_ln;

	/* Only the last 14 bytes of the MD5 go to the mz record (first two bytes are the file name) */
	memcpy(zsrc, md5 + 2, MZ_MD5); 

	/* Add the 32-bit compressed file length */
	memcpy(zsrc + MZ_MD5, (char *) &zln, MZ_SIZE);

	int mzid = uint16(md5);
	int mzlen = zsrc_ln + MZ_HEAD;

	/* If it won't fit in the cache, write it directly */
	if (mzlen > MZ_CACHE_SIZE)
	{
		mz_write(mzid, zsrc, mzlen);
	}
	else
	{
		/* Flush cache and add to cache */
		if (mz_cache[mzid].length + mzlen > MZ_CACHE_SIZE)
		{
			mz_write(mzid, zsrc, mzlen);
			mz_cache[mzid].length = mzlen;
			memcpy(mz_cache[mzid].data, zsrc, mzlen);
		}

		/* Just add to cache */
		else
		{
			memcpy(mz_cache[mzid].data + mz_cache[mzid].length, zsrc, mzlen);
			mz_cache[mzid].length += mzlen;
		}
	}
}

/* Write all cached mz records */
void mz_flush(struct mz_cache_item *mz_cache)
{
	for (int i = 0; i < MZ_FILES; i++)
	{
		if (mz_cache[i].length)
		{
			mz_write(i, mz_cache[i].data, mz_cache[i].length);
			mz_cache[i].length = 0;
		}
	}
}

/* Checks mz container for integrity */
bool mz_check(char *path)
{

	/* Open mz file */
	int mzfile = open(path, O_RDONLY);
	if (mzfile < 0) return false;

	/* Obtain file size */
	uint64_t size = lseek64(mzfile, 0, SEEK_END);
	if (!size) return false;

	/* Read entire .mz file to memory */
	uint8_t *mz = malloc(size);	
	lseek64 (mzfile, 0, SEEK_SET);
	if (!read(mzfile, mz, size)) 
	{
		free(mz);
		return false;
	}
	close(mzfile);

	/* Recurse mz contents */
	uint64_t ptr = 0;
	uint32_t ln = 0;

	while (ptr < size)
	{
		/* Read compressed data length */
		memcpy((uint8_t*)&ln, mz + ptr + MZ_MD5, MZ_SIZE);

		/* Increment ptr */
		ptr += (MZ_HEAD + ln);
	}
	close(mzfile);
	free(mz);

	if (ptr > size) return false;

	return true;
}

/* Fills bytes 3-16 of md5 with id */
void mz_id_fill(char *md5, uint8_t *mz_id)
{
	for (int i = 0; i < 14; i++)
	{
		sprintf(md5 + 4 + 2 * i, "%02x", mz_id[i]);
	}
}

void mz_corrupted()
{
	printf("Corrupted mz file\n");
	exit(EXIT_FAILURE);
}

void mz_deflate(struct mz_job *job)
{
	/* Decompress data */
	job->data_ln = MAX_FILE_SIZE;
	if (Z_OK != uncompress((uint8_t *)job->data, &job->data_ln, job->zdata, job->zdata_ln))
	{
		mz_corrupted();
	}
	job->data_ln--;
}

