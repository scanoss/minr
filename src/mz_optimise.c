// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/mz_optimise.c
 *
 * SCANOSS .mz optimisation functions
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
  * @file mz_optimize.c
  * @date 26 Oct 2021 
  * @brief Implement MZ optimization functions
  */

#include <zlib.h>
#include <libgen.h>

#include "minr.h"
#include <ldb.h>
#include "hex.h"
#include "ignorelist.h"
#include "mz.h"


/**
 * @brief Check if job->id is found in job->xkeys (see -X)
 * 
 * @param job pointer to mz job
 * @return true if it was found
 */
bool mz_id_excluded(struct mz_job *job)
{
	if (!job->xkeys_ln) return false;

	for (uint64_t i = 0; i < job->xkeys_ln; i += MD5_LEN)
	{
		/* Compare job id (bytes 3-16) */
		if (!memcmp(job->xkeys + i + 2, job->id, MD5_LEN - 2))
		{
			/* Compare mz id (bytes 1-2) */
			if (!memcmp(job->xkeys + i, job->mz_id, 2)) return true;
		}
	}

	return false;
}

/**
 * @brief Check if job->id is found in the LDB
 * 
 * @param job pointee to mz job.
 * @return true if exist
 */
bool mz_id_exists_in_ldb(struct mz_job *job)
{
	if (!job->orphan_rm) return true;

	struct ldb_table oss_file;
	strcpy(oss_file.db, "oss");
	strcpy(oss_file.table, TABLE_NAME_FILE);
	oss_file.key_ln = 16;
	oss_file.rec_ln = 0;
	oss_file.ts_ln = 2;
	oss_file.tmp = false;

	uint8_t file_id[16];
	ldb_hex_to_bin(job->md5, 4, file_id);
	memcpy(file_id + 2, job->id, MZ_MD5);

	if (!ldb_key_exists(oss_file, file_id)) return false;

	return true;
}

/**
 * @brief Check a md5 match
 * 
 * @param mz1 input 1
 * @param mz2 input 2
 * @return true if they match
 */
bool mz_md5_match(uint8_t *mz1, uint8_t *mz2)
{
	for (int i = 0; i < MZ_MD5; i++)
	if (mz1[i] != mz2[i]) return false;

	return true;
}

/**
 * @brief Handler function to be passed to mz_parse()
 *		Eliminates duplicated data, unwanted content and (optionally) orphan files (not found in the KB)
 * 
 * @param job pointer to mz job
 * @return true 
 */
static bool mz_optimise_handler(struct mz_job *job)
{
	/* Uncompress */
	MZ_DEFLATE(job);
	job->data[job->data_ln] = 0;

	uint8_t md5[16];
	MD5((uint8_t *)job->data, job->data_ln, md5);

	/* Skip if corrupted file */
	if (!mz_md5_match(job->id, md5 + 2))
	{
		job->igl_c++;
	}

	/* Check if data contains unwanted header */
	else if (job->data_ln < MIN_FILE_SIZE)
	{
		job->min_c++;
	}

	/* Check if data is too square */
	else if (too_much_squareness(job->data))
	{
		job->igl_c++;
	}

	/* Check if data contains unwanted header */
	else if (unwanted_header(job->data))
	{
		job->igl_c++;
	}

	/* Check if file is not duplicated */
	else if (mz_id_exists(job->ptr, job->ptr_ln, job->id))
	{
		job->dup_c++;
	}

	/* Check if mz_id is to be excluded (see -X) */
	else if (mz_id_excluded(job))
	{
		job->exc_c++;
	}

	/* Check if file exists in the LDB */
	else if (!mz_id_exists_in_ldb(job))
	{
		job->orp_c++;
	}

	else
	{
		memcpy(job->ptr + job->ptr_ln, job->id, job->ln);
		job->ptr_ln += job->ln;
	}
	free(job->data);
	return true;
}

static bool mz_optimise_dup_handler(struct mz_job *job)
{
	/* Uncompress */
	MZ_DEFLATE(job);
	job->data[job->data_ln] = 0;

	uint8_t md5[16];
	MD5((uint8_t *)job->data, job->data_ln, md5);

	/* Check if file is not duplicated */
	if (mz_id_exists(job->ptr, job->ptr_ln, job->id))
	{
		job->dup_c++;
	}
	else
	{
		memcpy(job->ptr + job->ptr_ln, job->id, job->ln);
		job->ptr_ln += job->ln;
	}

	free(job->data);

	return true;
}


/**
 * @brief Optimise an mz file removing duplicated data
 * 
 * @param job pointer to mz job
 */
void mz_optimise(struct mz_job *job, mz_optimise_mode_t mode)
{
	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(job->path), 4);
	ldb_hex_to_bin(job->md5, 4, job->mz_id);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* Reserve memory for destination mz */
	job->ptr = calloc(job->mz_ln, 1);

	/* Launch optimisation */
	switch (mode)
	{
	case MZ_OPTIMISE_ALL:
		mz_parse(job, mz_optimise_handler);
		break;
	case MZ_OPTIMISE_DUP:
		mz_parse(job, mz_optimise_dup_handler);
		break;
	
	default:
		break;
	}
	
	/* Write updated mz file */
	file_write(job->path, job->ptr, job->ptr_ln);

	if (job->dup_c) printf("%u duplicated files eliminated\n", job->dup_c);
	if (job->orp_c) printf("%u orphan files eliminated\n", job->orp_c);
	if (job->igl_c) printf("%u ignored files eliminated\n", job->igl_c);
	if (job->min_c) printf("%u small files eliminated\n", job->min_c);
	if (job->exc_c) printf("%u keys excluded\n", job->exc_c);

	free(job->mz);
}


