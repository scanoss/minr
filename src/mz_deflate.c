// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/mz_deflate.c
 *
 * MZ decompression, validation and listing functions
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
  * @file mz_deflate.c
  * @date 26 Oct 2021 
  * @brief <Implement the functions used to deplate MZ files
  */

#include <libgen.h>

#include "ldb.h"
#include "minr.h"
#include "md5.h"
#include "hex.h"

/**
 * @brief Compare two mz keys
 * 
 * @param a	key a
 * @param b	key b
 * @return 1 if a > b. -1 if b < a, 0 if they are equals
 */
int mz_key_cmp(const void * a, const void * b)
{
    const uint8_t *va = a;
    const uint8_t *vb = b;

    /* Compare byte by byte */
    for (int i = 0; i < MD5_LEN; i++)
    {
        if (va[i] > vb[i]) return 1;
        if (va[i] < vb[i]) return -1;
    }

    return 0;
}

/**
 * @brief Handling function for listing mz keys
 * 
 * @param job pointer to mz job
 * @return true 
 */
bool mz_dump_keys_handler(struct mz_job *job)
{
	/* Fill MD5 with item id */
	mz_id_fill(job->md5, job->id);

	ldb_hex_to_bin(job->md5, MD5_LEN * 2, job->ptr + job->ptr_ln);
	job->ptr_ln += MD5_LEN;

	return true;
}

/**
 * @brief  Output unique mz keys to STDOUT (binary) 
 * 
 * @param job pointer to mz job
 */
void mz_dump_keys(struct mz_job *job)
{
	/* Use job->ptr to store keys */
	job->ptr = malloc(job->mz_ln);

	/* Fetch keys */
	mz_parse(job, mz_dump_keys_handler);

	/* Sort keys */
	qsort(job->ptr, job->ptr_ln / MD5_LEN, MD5_LEN, mz_key_cmp);

	/* Output keys */
	for (int i = 0; i < job->ptr_ln; i += 16)
	{
		bool skip = false;
		if (i) if (!memcmp(job->ptr + i, job->ptr + i - MD5_LEN, MD5_LEN))
		{
			skip = true;
		}
		if (!skip) fwrite(job->ptr + i, MD5_LEN, 1, stdout);
	}

	free(job->ptr);
}

/**
 * @brief Handling function for listing mz contents
 * 
 * @param job 
 * @return true 
 */
bool mz_list_handler(struct mz_job *job)
{
	/* Fill MD5 with item id */
	mz_id_fill(job->md5, job->id);

	/* Decompress */
	mz_deflate(job);

	/* Calculate resulting data MD5 */
	uint8_t actual_md5[MD5_LEN];
	calc_md5(job->data, job->data_ln, actual_md5);

	/* Compare data checksum to validate */
	char *actual = bin_to_hex(actual_md5, MD5_LEN);

	if (strcmp(job->md5, actual))
	{
		printf("%s [NOK] %lu bytes\n", job->md5, job->data_ln);
	}
	else if (!job->check_only)
	{
		printf("%s [OK] %lu bytes\n", job->md5, job->data_ln);
	}

	free(actual);
	return true;
}

/**
 * @brief List the content of a mz file
 * 
 * @param job pointer to mz job
 */
void mz_list(struct mz_job *job)
{
	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(job->path), 4);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* List mz contents */
	if (!job->dump_keys) mz_parse(job, mz_list_handler);

	/* Dump mz keys */
	else mz_dump_keys(job);

	free(job->mz);
}

/**
 * @brief Decompress and print a mz file (handler)
 * 
 * @param job pointer to mz job
 * @return true 
 * @return false 
 */
bool mz_cat_handler(struct mz_job *job)
{
	if (!memcmp(job->id, job->key + 2, MZ_MD5))
	{
		/* Decompress */
		mz_deflate(job);

		job->data[job->data_ln] = 0;
		printf("%s", job->data);

		return false;
	}
	return true;
}

/**
 * @brief Decompress and print a mz file
 * 
 * @param job pointer to mz job
 * @param key key to be found
 */
void mz_cat(struct mz_job *job, char *key)
{
	/* Calculate mz file path */
	char mz_path[MAX_PATH_LEN] = "\0";
	char mz_file_id[5] = "\0\0\0\0\0";
	memcpy(mz_file_id, key, 4);

	sprintf(mz_path, "%s/%s.mz", job->path, mz_file_id);

	/* Save path and key on job */
	job->key = calloc(MD5_LEN, 1);
	ldb_hex_to_bin(key, MD5_LEN * 2, job->key);

	/* Read source mz file into memory */
	job->mz = file_read(mz_path, &job->mz_ln);

	/* Search and display "key" file contents */
	mz_parse(job, mz_cat_handler);

	free(job->key);
	free(job->mz);
}

/**
 * @brief Handler for mz extraction
 * 
 * @param job pointer to mz job
 * @return true 
 */
bool mz_extract_handler(struct mz_job *job)
{
	/* Fill MD5 with item id */
	mz_id_fill(job->md5, job->id);

	/* Decompress */
	mz_deflate(job);

	/* Calculate resulting data MD5 */
	uint8_t actual_md5[MD5_LEN];
	calc_md5(job->data, job->data_ln, actual_md5);

	/* Compare data checksum to validate */
	char *actual = bin_to_hex(actual_md5, MD5_LEN);

	if (strcmp(job->md5, actual))
	{
		mz_corrupted();
	}
	else
	{
		printf("Extracting %s (%lu bytes)\n", job->md5, job->data_ln);

		/* Extract data to file */
		file_write(job->md5, (uint8_t *)job->data, job->data_ln);
	}

	free(actual);

	return true;
}

/**
 * @brief Extract the content of a mz file
 * 
 * @param job pointer to mz job
 */
void mz_extract(struct mz_job *job)
{
	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(job->path), 4);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* Launch extraction */
	mz_parse(job, mz_extract_handler);

	free(job->mz);
}
