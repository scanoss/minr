// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/main.c
 *
 * SCANOSS Mining Tool
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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <zlib.h>
#include "ignorelist.h"
#include "ignored_wfp.h"
#include "minr.h"
#include "winnowing.h"
#include "file.h"
#include "license.h"
#include "ldb.h"
#include "help.h"
#include "wfp.h"
#include "import.h"
#include "crypto.h"
#include "url.h"

int main(int argc, char *argv[])
{
	if (!check_dependencies()) exit(1);
	int exit_code = EXIT_SUCCESS;

	/* Configuration defaults */
	strcpy(tmp_path, "/dev/shm");
	struct minr_job job;
	strcpy(job.mined_path, ".");

	// Minr job
	*job.path = 0;
	*job.metadata = 0;
	*job.url = 0;
	*job.urlid = 0;
	*job.fileid = 0;
	*job.pairid = 0;
	*job.license = 0;
	job.all_extensions = false;
	job.exclude_mz = false;
	job.exclude_detection = false;

	// Import job
	job.skip_sort = false;
	*job.import_path=0;

	// Join job
	*job.join_from=0;
	*job.join_to=0;

	// Snippet mine job
	*job.mz=0;
	job.mz_cache = NULL;

	// Tmp data
	job.src_ln = 0;
	job.zsrc_ln = 0;

	// License info
	job.license_count = 0;
	job.local_mining = 0;


	/* Parse arguments */
	int option;
	bool invalid_argument = false;

	while ((option = getopt(argc, argv, ":c:C:L:Q:Y:o:m:g:w:t:f:T:i:l:z:u:d:xXseahv")) != -1)
	{

		/* Check valid alpha is entered */
		if (optarg)
		{
			if ((strlen(optarg) > MAX_ARG_LEN))
			{
				invalid_argument = true;
				break;
			}
		}

		switch (option)
		{
			case 'o':
				strcpy(job.mined_path, optarg);
				if (!create_dir(job.mined_path))
				{
					invalid_argument = true;
					printf("Cannot create output directory\n");
				}
				break;

			case 'm':
				min_file_size = atoi(optarg);
				break;

			case 'g':
				GRAM   = atoi(optarg);
				break;

			case 'w':
				WINDOW = atoi(optarg);
				break;

			case 't':
				strcpy(job.join_to, optarg);
				break;

			case 'f':
				strcpy(job.join_from, optarg);
				break;

			case 'T':
				strcpy(tmp_path, optarg);
				break;

			case 'i':
				strcpy(job.import_path, optarg);
				break;

			case 'l':
				generate_license_ids_c(optarg);
				exit(EXIT_SUCCESS);

			case 'c':
				strcpy(tmp_path, optarg);
				create_crypto_definitions(tmp_path);
				exit(EXIT_SUCCESS);
				break;

			case 'z':
				strcpy(job.mz, optarg);
				break;

			case 'u':
				strcpy(job.url, optarg);
				break;
			
			case 'C':
				job.local_mining = 4;
				strcpy(job.url, optarg);
				break;
			
			case 'L':
				strcpy(job.url, optarg);
				job.local_mining = 2;
				break;
			
			case 'Q':
				strcpy(job.url, optarg);
				job.local_mining = 3;
				break;

			case 'Y':
				strcpy(job.url, optarg);
				job.local_mining = 1;
				break;
				
			case 'd':
				strcpy(job.metadata, optarg);
				break;

			case 's':
				job.skip_sort = true;
				break;

			case 'x':
				job.exclude_mz = true;
				break;

			case 'X':
				job.exclude_detection = true;
				break;

			case 'a':
				job.all_extensions = true;
				break;

			case 'h':
				show_help();
				exit(EXIT_SUCCESS);
				break;

			case 'v':
				printf("scanoss-minr-%s\n", MINR_VERSION);
				exit(EXIT_SUCCESS);
				break;

			case ':':
				printf("Missing value for parameter\n");
				invalid_argument = true;
				break;

			case '?':
				printf("Unsupported option: %c\n", optopt);
				invalid_argument = true;
				break;
		}
		if (invalid_argument) break;
	}

	for (;optind < argc-1; optind++)
	{
		printf("Invalid argument: %s\n", argv[optind]);
		invalid_argument = true;
	}

	if (invalid_argument)
	{
		printf("Error parsing arguments\n");
		exit(EXIT_FAILURE);
	}

	strcat(job.mined_path, "/mined");

	/* Import mined/ into the LDB */
	if (*job.import_path) mined_import(job.import_path, job.skip_sort);

	/* Join mined/ structures */
	else if (*job.join_from && *job.join_to) minr_join(&job);

	/* Mine local files */
	else if (job.local_mining != 0)
	{
		job.licenses = load_licenses(&job.license_count);
		if (is_file(job.url)) mine_local_file(&job, job.url);
		else mine_local_directory(&job,job.url);
		free(job.licenses);
	}

	/* Process mz file */
	else if (*job.mz)
	{
		if (!is_dir(job.mz))
		{
			printf("Cannot access directory %s\n", job.mz);
			exit(EXIT_FAILURE);
		}

		buffer = malloc(BUFFER_SIZE * 256);
		hashes = malloc(MAX_FILE_SIZE);
		lines  = malloc(MAX_FILE_SIZE);

		/* Open all file handlers in mined/snippets (256 files) */
		out_snippet = open_snippet(job.mz);

		/* Import snippets */
		for (int i = 0; i < MZ_FILES; i++)
		{
			char *file_path = calloc(MAX_PATH_LEN + 1, 1);
			sprintf(file_path, "%s/sources/%04x.mz", job.mz, i);
			if (file_size(file_path))
			{
				printf("%s\n", file_path);
				mz_wfp_extract(file_path);
			}
			free(file_path);
		}

		free(buffer);
		free(hashes);
		free(lines);

		/* Close files */
		for (int i=0; i < 256; i++) close(out_snippet[i]);
		free(out_snippet);
	}

	/* Mine URL */
	else if (*job.metadata && *job.url)
	{

		/* -d expects 6 CSV values: vendor,component,version,release_date,license,purl */
		if (count_chr(',', job.metadata) != 5)
		{
			printf("Wrong number of values passed with -d\n");
			exit(EXIT_FAILURE);
		}

		if (!create_dir(job.mined_path))
		{
			printf("Cannot create output structure in %s\n", job.mined_path);
			exit(EXIT_FAILURE);
		}

		/* Load licenses */
		job.licenses = load_licenses(&job.license_count);

		/* Mine URL or folder */

		url_download(&job);

		free(job.licenses);

	}
	else
	{
		exit_code = EXIT_FAILURE;
		printf("Invalid input. Please try -h\n");
	}
	clean_crypto_definitions();
	exit(exit_code);
}

