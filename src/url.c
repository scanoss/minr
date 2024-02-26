// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/url.c
 *
 * URL handling routines
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
  * @file url.c
  * @date 22 Sep 2021 
  * @brief Implement functions used to mine a URL
  */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "minr.h"
#include "ldb.h"
#include "wfp.h"
#include "minr_log.h"
#include <sys/time.h>
/**
 * @brief Calculate purl md5
 * 
 * @param job pointer to minr job
 */
void get_purl_id(struct minr_job *job)
{
	/* Clear memory */
	memset(job->purl_md5, 0, 16);

	/* Extract purl and version from metadata */
	char purl[MAX_PATH_LEN] = "\0";
	char version[MAX_PATH_LEN] = "\0";
	extract_csv(purl, job->metadata, 6, MAX_ARG_LEN);
	extract_csv(version, job->metadata, 3, MAX_ARG_LEN);
	if (!*purl) return;

	/* Calculate purl md5 */
	MD5((uint8_t *)purl, strlen(purl), job->purl_md5);
	ldb_bin_to_hex(job->purl_md5, MD5_LEN, job->purlid);

	/* Compile purl@version string */
	if (!*version) return;
	char purlversion[2 * MAX_PATH_LEN] = "\0";
	sprintf(purlversion, "%s@%s", purl, version);

	/* Calculate purl@version md5 */
	MD5((uint8_t *)purlversion, strlen(purlversion), job->version_md5);
	ldb_bin_to_hex(job->version_md5, MD5_LEN, job->versionid);
}

/**
 * @brief Add url to mnir job
 * 
 * @param job pointer to minr job
 */
void url_add(struct minr_job *job)
{
	char path[MAX_PATH_LEN]="\0";
	sprintf(path, "%s/%s.csv", job->mined_path, TABLE_NAME_URL);

	FILE *fp = fopen(path, "a");
	if (!fp)
	{
		printf("Cannot create file %s\n", path);
		exit(EXIT_FAILURE);
	}
	fprintf(fp, "%s,%s,%s\n", job->urlid, job->metadata, *job->download_url ? job->download_url : job->url);
	fclose(fp);

	/* Obtain purl id and purl@version id */
	get_purl_id(job);

	/* Load license from metadata */
	extract_csv(job->license, job->metadata, 5, MAX_ARG_LEN);
	if (*job->license)
	{
		sprintf(path, "%s/%s.csv", job->mined_path, TABLE_NAME_LICENSE);
		FILE *fp = fopen(path, "a");
		if (!fp)
		{
			printf("Cannot create file %s\n", path);
			exit(EXIT_FAILURE);
		}
		fprintf(fp, "%s,0,%s\n", job->versionid, job->license);
		fprintf(fp, "%s,0,%s\n", job->purlid, job->license);
		fclose(fp);
	}
}

/**
 * @brief Download a URL for a job
 * 
 * @param job pointer to minr job
 */
void url_download(struct minr_job *job)
{
	bool downloaded = false;
	char * aux_root_dir = NULL;
	
	/* Reserve memory for snippet fingerprinting */
	if (!job->exclude_mz)
	{
//		wfp_init(NULL);
		/* Reserve memory for mz_cache for mined/sources (65536 files) */
		job->mz_cache = malloc(MZ_FILES * sizeof(struct mz_cache_item));
		job->mz_cache_extra = malloc(MZ_FILES * sizeof(struct mz_cache_item));
		for (int i = 0; i < MZ_FILES; i++)
		{
			job->mz_cache[i].length = 0;
			job->mz_cache_extra[i].length = 0;
		}
	}

	/* Mine a local folder instead of a URL */
	if (is_dir(job->url))
	{
		/* No need for tmp_dir, we'll use the provided directory instead */
		strcpy(job->tmp_dir, job->url);

		/* URLID will be the hash of the metadata passed */
		uint8_t urlid[MD5_LEN];
		MD5((uint8_t *)job->metadata, strlen(job->metadata), urlid);
		ldb_bin_to_hex(urlid, MD5_LEN, job->urlid);

		downloaded = true;
	}
	/* Create temporary component directory */
	else
	{
		struct timeval  tv;
		gettimeofday(&tv, NULL);
		sprintf(job->tmp_dir,"%s/minr-%d-%lu", tmp_path, getpid(), tv.tv_sec);
		mkdir(job->tmp_dir, 0755);
		/*keep a copy of this root dir to erase later*/
		aux_root_dir = strdup(job->tmp_dir);
		/* urlid will contain the hex md5 of the entire component */
		downloaded = download(job);
	}

	/* Open all file handlers in mined/files (256 files) */
	job->out_file = open_file(job->mined_path, TABLE_NAME_FILE);
	/*Pivot table will have only one file*/
	char pivot_path[MAX_PATH_LEN];
	sprintf(pivot_path, "%s/%s/", job->mined_path, TABLE_NAME_PIVOT);
	if (create_dir(pivot_path) && *job->urlid)
	{
		strncat(pivot_path, job->urlid, 2);
		strcat(pivot_path, ".csv");
		job->out_pivot = fopen(pivot_path, "a");
		if (!job->out_pivot)
			minr_log("Error opening %s\n", pivot_path);
	}
	else
	{
		minr_log("Error creating %s\n", pivot_path);
	}
	
	if (job->mine_all)
	{
		job->out_file_extra = open_file(job->mined_extra_path, TABLE_NAME_FILE);
		/*Pivot table will have only one file*/
		char pivot_path[MAX_PATH_LEN];
		sprintf(pivot_path, "%s/%s/", job->mined_extra_path, TABLE_NAME_PIVOT);
		if (create_dir(pivot_path))
		{
			strncat(pivot_path, job->urlid, 2);
			strcat(pivot_path, ".csv");
			job->out_pivot_extra= fopen(pivot_path, "a");
			if (!job->out_pivot_extra)
				minr_log("Error opening %s\n", pivot_path);
		}
		else
		{
			minr_log("Error creating %s\n", pivot_path);
		}
	}

	/* Process downloaded/expanded directory */
	if (is_dir(job->tmp_dir) && downloaded)
	{
		/* Add info to urls.csv */
		url_add(job);
		
		/* Find for license declaration into specific files in the roor dir (no recursive) */
		mine_license_exec(job);

		recurse(job, job->tmp_dir);
	}

	else
	{
		printf("Capture failed: %s\n",job->tmp_dir);
	}

	/* Delete temp directory which store source package via url downloading*/	
	if (aux_root_dir != NULL && aux_root_dir[0] == '\0')
	{
		// aux_root_dir is NULL string
	}
	else if (aux_root_dir == NULL)
	{
		// aux_root_dir is NULL pointer
	}
	else
	{
		rm_dir(aux_root_dir);
	}

	free(aux_root_dir);
	/* Close files */
	for (int i=0; i < 256; i++)
	{
		fclose(job->out_file[i]);
		if (job->mine_all)
		{
			fclose(job->out_file_extra[i]);
		}
	}
	
	if (job->out_pivot)
		fclose(job->out_pivot);

	if (job->mine_all)
		fclose(job->out_pivot_extra);

	if (!job->exclude_mz)
	{
		/* Flush mz cache */
		mz_flush(job->mined_path, job->mz_cache);
		mz_flush(job->mined_extra_path, job->mz_cache_extra);

		free(job->mz_cache);
		free(job->mz_cache_extra);
		wfp_free();
	}

	free(job->out_file);
	
	if (job->mine_all)
		free(job->out_file_extra);

}
