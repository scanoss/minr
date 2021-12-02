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
  * @brief ???
  */

#include <stdbool.h>
#include <stdint.h>
#include <zlib.h>
#include "hex.h"
#include "ignorelist.h"
#include "ldb.h"
#include "minr.h"

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
NULL
};

/**
 * @brief Verify if the filename is an attribution notice
 * 
 * @param path  
 * @return true 
 */
bool is_attribution_notice(char *path)
{
	char *file = basename(path);
	if (!file) return true;
	if (!*file) return true;

	int i=0;
	while (ATTRIBUTION_NOTICES[i])
		if (stricmp(ATTRIBUTION_NOTICES[i++], file))
			return true;

	return false;
}

/**
 * @brief Write entry id to attribution.csv
 * 
 * @param job 
 */
void attribution_add(struct minr_job *job)
{
	char path[MAX_PATH_LEN]="\0";
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
 * @brief Append attribution notice to archive
 * 
 * @param job 
 * @param path 
 */
void mine_attribution_notice(struct minr_job *job, char *path)
{
	if (!load_file(job,path)) return;

	mine_license(job, job->purlid, true);

	/* Reload file after license analysis */
	if (!load_file(job,path)) return;

	/* Write entry to mined/attribution.csv */
	attribution_add(job);

	/* Create sources directory */
	char mzpath[LDB_MAX_PATH * 2];
	sprintf(mzpath, "%s/notices", job->mined_path);

	ldb_prepare_dir(mzpath);

	/* Compress data */
	job->zsrc_ln = compressBound(job->src_ln + 1);

	/* Save the first bytes of zsrc to accomodate the MZ header */
	compress(job->zsrc + MZ_HEAD, &job->zsrc_ln, (uint8_t *) job->src, job->src_ln + 1);
	uint32_t zln = job->zsrc_ln;

	/* Only the last 14 bytes of the MD5 go to the mz record (first two bytes are the file name) */
	memcpy(job->zsrc, job->md5 + 2, MZ_MD5);

	/* Add the 32-bit compressed file length */
	memcpy(job->zsrc + MZ_MD5, (char *) &zln, MZ_SIZE);

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
