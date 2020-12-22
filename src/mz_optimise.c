// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/mz_optimise.c
 *
 * SCANOSS .mz optimization functions
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
#include <zlib.h>
#include <libgen.h>

#include "minr.h"
#include "ldb.h"
#include "hex.h"
#include "blacklist.h"
#include "mz.h"
/* Check if job->id is found in the LDB */
bool mz_id_exists_in_ldb(struct mz_job *job)
{
	struct ldb_table oss_file;
	strcpy(oss_file.db, "oss");
	strcpy(oss_file.table, "file");
	oss_file.key_ln = 16;
	oss_file.rec_ln = 0;
	oss_file.ts_ln = 2;
	oss_file.tmp = false;

	uint8_t file_id[16];
	hex_to_bin(job->md5, 4, file_id);
	memcpy(file_id + 2, job->id, MZ_MD5);

	if (!ldb_key_exists(oss_file, file_id)) return false;

	return true;
}

bool mz_optimise_handler(struct mz_job *job)
{
		/* Uncompress */
		uint64_t src_ln = MAX_FILE_SIZE;
		if (Z_OK != uncompress((uint8_t *)job->data, &src_ln, job->zdata, job->zdata_ln))
		{
			printf("[CORRUPTED]\n");
			exit(EXIT_SUCCESS);
		}
		job->data_ln = src_ln - 1;
		job->data[job->data_ln] = 0;

		/* Check if data contains unwanted header */
		if (unwanted_header(job->data))
		{
			job->bll_c++;
		}

		/* Check if file is not duplicated */
		else if (mz_id_exists(job->ptr, job->ptr_ln, job->id))
		{
			job->dup_c++;
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

	return true;
}

/* Optimise an mz file removing duplicated data */
void mz_optimise(struct mz_job *job, char *path)
{
	job->path = path;

	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(path), 4);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* Reserve memory for destination mz */
	job->ptr = calloc(job->mz_ln, 1);

	/* Launch optimisation */
	mz_parse(job, mz_optimise_handler);

	/* Write updated mz file */
	file_write(job->path, job->ptr, job->ptr_ln);

	if (job->dup_c) printf("%u duplicated files eliminated\n", job->dup_c);
	if (job->orp_c) printf("%u orphan files eliminated\n", job->orp_c);
	if (job->bll_c) printf("%u blacklisted files eliminated\n", job->bll_c);

	free(job->mz);
	free(job->ptr);
}
