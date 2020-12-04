// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/mz_deflate.c
 *
 * MZ decompression, validation and listing functions
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
		mz_corrupted();
	}
	else if (!job->check_only)
	{
		printf("%s [OK] %lu bytes\n", job->md5, job->data_ln);
	}

	free(actual);
	return true;
}

void mz_list(struct mz_job *job, char *path)
{
	job->path = path;

	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(path), 4);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* List mz contents */
	mz_parse(job, mz_list_handler);

	free(job->mz);
}

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

void mz_cat(struct mz_job *job, char *path, char *key)
{
	/* Calculate mz file path */
	char mz_path[MAX_PATH_LEN] = "\0";
	char mz_file_id[5] = "\0\0\0\0\0";
	memcpy(mz_file_id, key, 4);

	sprintf(mz_path, "%s/%s.mz", path, mz_file_id);

	/* Save path and key on job */
	job->path = mz_path;
	job->key = calloc(MD5_LEN, 1);
	hex_to_bin(key, MD5_LEN * 2, job->key);	

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* Search and display "key" file contents */
	mz_parse(job, mz_cat_handler);

	free(job->key);
	free(job->mz);
}

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

void mz_extract(struct mz_job *job, char *path)
{
	job->path = path;

	/* Extract first two MD5 bytes from the file name */
	memcpy(job->md5, basename(path), 4);

	/* Read source mz file into memory */
	job->mz = file_read(job->path, &job->mz_ln);

	/* Launch extraction */
	mz_parse(job, mz_extract_handler);

	free(job->mz);
}
