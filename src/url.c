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

#include <ctype.h>
#include <openssl/md5.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "minr.h"
#include "ldb.h"

/* Returns the pair md5 of "component/vendor" */
void vendor_component_md5(char *vendor, char *component, uint8_t *out)
{
	char pair[MAX_PATH_LEN] = "\0";
	if (strlen(component) + strlen(vendor) + 2 >= 1024) return;

	/* Calculate pair_md5 */
	sprintf(pair, "%s/%s", vendor, component);
	for (int i = 0; i < strlen(pair); i++) pair[i] = tolower(pair[i]);
	MD5((uint8_t *)pair, strlen(pair), out);

	/* Log pair_md5 */
	char hex[MD5_LEN * 2 + 1] = "\0";
	ldb_bin_to_hex(out, MD5_LEN, hex);
}

/* Calculate vendor/component md5 */
void get_vendor_component_id(struct minr_job *job)
{
	/* Extract vendor and component */
	char vendor[MAX_ARG_LEN] = "\0";
	char component[MAX_ARG_LEN] = "\0";

	/* Clear memory */
	memset(job->pair_md5, 0, 16);

	/* Extract vendor and component from metadata */
	extract_csv(vendor, job->metadata, 1, MAX_ARG_LEN);
	extract_csv(component, job->metadata, 2, MAX_ARG_LEN);
	if (!*vendor || !*component) return;

	/* Calculate md5 */
	vendor_component_md5(vendor, component, job->pair_md5);
	ldb_bin_to_hex(job->pair_md5, MD5_LEN, job->pairid);
}

/* Calculate md5 of "component/vendor/version" */
void version_md5(struct minr_job *job)
{
	/* Clear memory */
	memset(job->version_md5, 0, 16);

	/* Extract vendor and component */
	char vendor[MAX_ARG_LEN] = "\0";
	char component[MAX_ARG_LEN] = "\0";
	char version[MAX_ARG_LEN] = "\0";

	/* Extract vendor and component from metadata */
	extract_csv(vendor, job->metadata, 1, MAX_ARG_LEN);
	extract_csv(component, job->metadata, 2, MAX_ARG_LEN);
	extract_csv(version, job->metadata, 3, MAX_ARG_LEN);

	if (!*vendor || !*component || !*version) return;
	if (strlen(component) + strlen(vendor) + strlen(version) + 3 >= 1024) return;

	/* Calculate md5 */
	char md5[MAX_PATH_LEN] = "\0";

	/* Calculate pair_md5 */
	sprintf(md5, "%s/%s/%s", vendor, component, version);
	for (int i = 0; i < strlen(md5); i++) md5[i] = tolower(md5[i]);
	MD5((uint8_t *)md5, strlen(md5), job->version_md5);
	ldb_bin_to_hex(job->version_md5, MD5_LEN, job->versionid);
}

void url_add(struct minr_job *job)
{
	char path[MAX_PATH_LEN]="\0";
	sprintf(path, "%s/urls.csv", job->mined_path);

	FILE *fp = fopen(path, "a");
	if (!fp)
	{
		printf("Cannot create file %s\n", path);
		exit(EXIT_FAILURE);
	}
	fprintf(fp, "%s,%s,%s\n", job->urlid, job->metadata, job->url);
	fclose(fp);

	/* Obtain vendor/component id and vendor/component/license id */
	get_vendor_component_id(job);
	version_md5(job);

	/* Load license from metadata */
	extract_csv(job->license, job->metadata, 5, MAX_ARG_LEN);
	if (*job->license)
	{
		sprintf(path, "%s/licenses.csv", job->mined_path);
		FILE *fp = fopen(path, "a");
		if (!fp)
		{
			printf("Cannot create file %s\n", path);
			exit(EXIT_FAILURE);
		}
		fprintf(fp, "%s,0,%s\n", job->versionid, job->license);
		fprintf(fp, "%s,0,%s\n", job->pairid, job->license);
		fclose(fp);
	}
}

void rm_tmpdir(struct minr_job *job)
{
	/* Assemble command */
	char command[MAX_PATH_LEN] = "\0";
	sprintf(command, "rm -rf %s", job->tmp_dir);

	/* Execute command */
	FILE *fp = popen(command, "r");
	pclose(fp);
}

void url_download(struct minr_job *job)
{
	bool downloaded = false;
	job->src = calloc(MAX_FILE_SIZE + 1, 1);
	job->zsrc = calloc((MAX_FILE_SIZE + 1) * 2, 1);

	/* Reserve memory for snippet fingerprinting */
	if (!job->exclude_mz)
	{
		buffer = malloc(BUFFER_SIZE * 256);
		hashes = malloc(MAX_FILE_SIZE);
		lines  = malloc(MAX_FILE_SIZE);

		/* Reserve memory for mz_cache for mined/sources (65536 files) */
		job->mz_cache = malloc(MZ_FILES * sizeof(struct mz_cache_item));
		for (int i = 0; i < MZ_FILES; i++) job->mz_cache[i].length = 0;
	}

	/* Open all file handlers in mined/files (256 files) */
	out_file = open_file(job->mined_path);

	/* Mine a local folder instead of a URL */
	if (is_dir(job->url))
	{
		/* No need for tmp_dir, we'll use the provided directory instead */
		sprintf(job->tmp_dir, job->url);

		/* URLID will be the hash of the metadata passed */
		uint8_t urlid[MD5_LEN];
		MD5((uint8_t *)job->metadata, strlen(job->metadata), urlid);
		ldb_bin_to_hex(urlid, MD5_LEN, job->urlid);

		downloaded = true;
	}

	/* Create temporary component directory */
	else
	{
		sprintf(job->tmp_dir,"%s/minr-%d", tmp_path, getpid());
		mkdir(job->tmp_dir, 0755);

		/* urlid will contain the hex md5 of the entire component */
		downloaded = download(job);
	}

	/* Process downloaded/expanded directory */
	if (is_dir(job->tmp_dir) && downloaded)
	{
		/* Add info to urls.csv */
		url_add(job);

		recurse(job, job->tmp_dir);

		if (!is_dir(job->url)) rm_tmpdir(job);
	}

	else
	{
		printf("Capture failed\n");
		exit(EXIT_FAILURE);
	}

	/* Close files */
	for (int i=0; i < 256; i++) fclose(out_file[i]);

	if (!job->exclude_mz)
	{
		/* Flush mz cache */
		mz_flush(job->mined_path, job->mz_cache);

		free(job->mz_cache);
		free(buffer);
		free(hashes);
		free(lines);
	}

	free(out_file);
	free(job->src);
	free(job->zsrc);
}
