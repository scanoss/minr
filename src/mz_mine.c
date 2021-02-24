// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/mz_mine.c
 *
 * Meta functions to mine data from MZ archives
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

#include <libgen.h>
#include "minr.h"
#include "ldb.h"
#include "copyright.h"
#include "quality.h"
#include "license.h"
#include "mz_mine.h"
#include "crypto.h"

bool mz_quality_handler(struct mz_job *job)
{
	/* Decompress */
	mz_deflate(job);

	/* Fill MD5 with item id */
	mz_id_fill(job->md5, job->id);

	/* Mine license */
	mine_quality(NULL, job->md5, job->data, job->data_ln);

	return true;
}

void mz_mine_quality(struct mz_job *job)
{
	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(job->path), 4);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* Launch quality mining */
	mz_parse(job, mz_quality_handler);

	free(job->mz);
}

/* License types
   0 = Declared in component
   1 = Declared in file with SPDX-License-Identifier
   2 = Detected in header
   */
bool mz_license_handler(struct mz_job *job)
{
	/* Decompress */
	mz_deflate(job);

	/* Fill MD5 with item id */
	mz_id_fill(job->md5, job->id);

	/* Mine license */
	mine_license(NULL, job->md5, job->data, job->data_ln, job->licenses, job->license_count);

	return true;
}

void mz_mine_license(struct mz_job *job)
{
	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(job->path), 4);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* Launch license mining */
	mz_parse(job, mz_license_handler);

	free(job->mz);
}

bool mz_copyright_handler(struct mz_job *job)
{
	/* Decompress */
	mz_deflate(job);

	/* Fill MD5 with item id */
	mz_id_fill(job->md5, job->id);

	/* Mine copyrights */
	mine_copyright(NULL, job->md5, job->data, job->data_ln);

	return true;
}

void mz_mine_copyright(struct mz_job *job)
{
	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(job->path), 4);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* Launch copyright mining */
	mz_parse(job, mz_copyright_handler);

	free(job->mz);
}

bool mz_crypto_handler(struct mz_job *job)
{
	/* Decompress */
	mz_deflate(job);

	/* Fill MD5 with item id */
	mz_id_fill(job->md5, job->id);
	unsigned char aux[10];
	memcpy(aux,job->data, 10);
	/* Mine cryptographic algorithms */
	mine_crypto(NULL, job->md5, job->data, job->data_ln);
	return true;
}

void mz_mine_crypto(struct mz_job *job)
{
	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(job->path), 4);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* Launch crypto mining */
	mz_parse(job, mz_crypto_handler);

	free(job->mz);
}

