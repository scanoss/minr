// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/attributions.c
 *
 * Attribution notices functions
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
  * @file attributions.c
  * @date 19 October 2021 
  * @brief Contains the functions for processing attribution and notices.
  */

#include <stdbool.h>
#include <stdint.h>
#include <zlib.h>
#include "hex.h"
#include "ignorelist.h"
#include "ldb.h"
#include "minr.h"
#include "scancode.h"
#include "file.h"
#include <dirent.h>
#include <ctype.h>

char *ATTRIBUTION_NOTICES[] =
	{
		"COPYING",
		"COPYING.lib",
		"LICENSE",
		"LICENSE.md",
		"LICENSE.txt",
		"LICENSE-Community.txt",
		"LICENSES",
		"NOTICE",
		NULL};

#define LICENSE_PATTERN_NUMBER 3

const char *LICENSE_PATTERN[] = {"LIC", "COPY", "NOTI"};

/**
 * @brief Verify if the filename is an attribution notice
 * 
 * @param path  
 * @return true 
 */
bool is_attribution_notice(char *path)
{
	char *file = basename(path);
	if (!file)
		return true;
	if (!*file)
		return true;

	int i = 0;
	while (ATTRIBUTION_NOTICES[i])
		if (stricmp(ATTRIBUTION_NOTICES[i++], file))
			return true;

	return false;
}

/**
 * @brief Write entry id to attribution.csv
 * 
 * @param job pointer to minr job
 */
void attribution_add(struct minr_job *job)
{
	char path[MAX_PATH_LEN] = "\0";
	sprintf(path, "%s/attribution.csv", job->mined_path);

	char notice_id[MD5_LEN * 2 + 1] = "\0";
	ldb_bin_to_hex(job->md5, MD5_LEN, notice_id);

	FILE *fp = fopen(path, "a");
	if (!fp)
	{
		printf("Cannot create file %s\n", path);
		exit(EXIT_FAILURE);
	}
	fprintf(fp, "%s,%s\n", job->purlid, notice_id);
	fclose(fp);
}


/**
 * @brief Check if a given file meets one of the name patter specified in LICENSE_PATTERN
 * 
 * @param path Path to the file
 */
static bool validate_file(char *path)
{
	if (!is_file(path))
		return false;

	char *file_name = strrchr(path, '/'); //find the las /
	if (!file_name)							  //if there is not / then point to the path
		file_name = path;
	else
		file_name++; //skip the bar

	char upper_file_name[strlen(file_name) + 1];
	memset(upper_file_name, 0, sizeof(upper_file_name));
	int i = 0;
	while (*file_name)
	{
		upper_file_name[i] = toupper(*file_name);
		i++;
		file_name++;
	}

	for (int j = 0; j < LICENSE_PATTERN_NUMBER; j++)
	{
		if (strstr(upper_file_name, LICENSE_PATTERN[j]))
		{
			return true;
		}
	}

	return false;
}

/**
 * @brief Find files in to a give directory that math the name patter specified in LICENSE_PATTERN and process those.
 * 
 * @param job  pointer to miner job struct
 * @return true if at least one file was found
 */
static bool find_files(struct minr_job *job)
{
	DIR *dp;
	struct dirent *entry;

	bool found = false;
	char *path = job->tmp_dir;
	if (!(dp = opendir(path)))
		return found;

	while ((entry = readdir(dp)))
	{
		if (valid_path(path, entry->d_name))
		{
			/* Assemble path */
			char tmp_path[MAX_PATH_LEN] = "\0";
			sprintf(tmp_path, "%s/%s", path, entry->d_name);

			/* File discrimination check #1: Is this an actual file? */
			if (validate_file(tmp_path))
			{
				if (job->scancode_mode)
					scancode_copy_to_tmp(tmp_path, job->versionid);

				if (load_file(job, tmp_path))
					mine_license(job, job->versionid, true);
				found = true;
			}
		}
	}

	if (dp)
		closedir(dp);
	return found;
}
/**
 * @brief Execute the licese detection for a minr job over the matching files with the LICENSE_PATTERN.
 * 
 * @param path Path to the file
 */
bool mine_license_exec(struct minr_job *job)
{
	char *csv_path = NULL;
	/* Assemble csv path */
	if (!job->local_mining)
		asprintf(&csv_path, "%s/licenses.csv", job->mined_path);

	if (job->scancode_mode)
		scancode_prepare_tmp_dir(job->versionid);

	if (find_files(job))
	{
		if (job->scancode_mode)
			scancode_run(job->versionid, csv_path);
	}

	free(csv_path);

	return true;
}

/**
 * @brief Append attribution notice to archive
 * 
 * @param job pointer to mnir job
 * @param path string path
 */
void mine_attribution_notice(struct minr_job *job, char *path)
{
	if (!load_file(job, path))
		return;

	/* Reload file after license analysis */
	if (!load_file(job, path))
		return;

	/* Write entry to mined/attribution.csv */
	attribution_add(job);

	/* Create sources directory */
	char mzpath[LDB_MAX_PATH * 2];
	sprintf(mzpath, "%s/notices", job->mined_path);

	ldb_prepare_dir(mzpath);

	/* Compress data */
	job->zsrc_ln = compressBound(job->src_ln + 1);

	/* Save the first bytes of zsrc to accomodate the MZ header */
	compress(job->zsrc + MZ_HEAD, &job->zsrc_ln, (uint8_t *)job->src, job->src_ln + 1);
	uint32_t zln = job->zsrc_ln;

	/* Only the last 14 bytes of the MD5 go to the mz record (first two bytes are the file name) */
	memcpy(job->zsrc, job->md5 + 2, MZ_MD5);

	/* Add the 32-bit compressed file length */
	memcpy(job->zsrc + MZ_MD5, (char *)&zln, MZ_SIZE);

	int mzid = uint16(job->md5);
	int mzlen = job->zsrc_ln + MZ_HEAD;

	sprintf(mzpath, "%s/notices/%04x.mz", job->mined_path, mzid);
	FILE *f = fopen(mzpath, "a");
	if (f)
	{
		size_t written = fwrite(job->zsrc, mzlen, 1, f);
		if (!written)
		{
			printf("Error writing %s\n", mzpath);
			exit(EXIT_FAILURE);
		}
		fclose(f);
	}

	mine_copyright(job->mined_path, job->purlid, job->src, job->src_ln, true);
}
