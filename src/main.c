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

/**
  * @file license.c
  * @date 28 Oct 2021 
  * @brief ???
  */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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
#include "scancode.h"
#include "minr_log.h"
#include <dlfcn.h>


void * lib_handle = NULL;
bool lib_load()
{
	/*set decode funtion pointer to NULL*/
	decode = NULL;
	lib_handle = dlopen("libscanoss_encoder.so", RTLD_NOW);
	char * err;
    if (lib_handle) 
	{
		minr_log( "Lib scanoss-enocder present\n");
		decode = dlsym(lib_handle, "scanoss_decode");
		if ((err = dlerror())) 
		{
			minr_log("%s\n", err);
			return false;
		}
		return true;
     }
	 
	 return false;
}

int main(int argc, char *argv[])
{
	if (!check_dependencies()) exit(1);
	int exit_code = EXIT_SUCCESS;

	/* Configuration defaults */
	strcpy(tmp_path, "/dev/shm");
	struct minr_job job;
	strcpy(job.mined_path, ".");
	strcpy(job.dbname, "oss");

	// Minr job
	*job.path = 0;
	*job.metadata = 0;
	*job.url = 0;
	*job.urlid = 0;
	*job.fileid = 0;
	*job.purlid = 0;
	*job.license = 0;
	*job.download_url = 0;
	job.all_extensions = false;
	job.exclude_mz = false;
	job.exclude_detection = false;
	job.is_attribution_notice = false;
	job.mine_all = false;
	// Import job
	job.skip_sort = false;
	job.skip_csv_check = false;
	job.skip_delete = false;
	*job.import_path=0;
	*job.import_table=0;
	job.import_overwrite=false;
	job.bin_import = false;
	// Join job
	*job.join_from=0;
	*job.join_to=0;

	// Snippet mine job
	*job.mz=0;
	job.mz_cache = NULL;
	job.mz_cache_extra = NULL;

	// Tmp data
	job.src_ln = 0;
	job.zsrc_ln = 0;

	// License info
	job.license_count = 0;
	job.local_mining = 0;
	job.scancode_mode = false;

	job.src = NULL;
	job.zsrc = NULL;
	job.out_pivot = NULL;
	job.out_pivot_extra = NULL;

	/* Parse arguments */
	int option;
	bool invalid_argument = false;

	bool lib_encoder_present = lib_load();

	while ((option = getopt(argc, argv, ":c:C:L:Q:Y:o:m:g:w:t:f:T:i:I:l:z:u:U:d:D:V:SxXsnkeahvOAb")) != -1)
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

			case 'I':
				strcpy(job.import_table, optarg);
				break;
			case 'b':
				if (lib_encoder_present)
				{
					job.bin_import = true;
					job.skip_csv_check = true;
				}
				else
				{
					printf("libscanoss-encoder.so must be present to run -b option\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'O':
				job.import_overwrite = true;
				break;

			case 'l':
				generate_license_ids_c(optarg);
				exit(EXIT_SUCCESS);

			case 'D':
				strcpy(job.dbname, optarg);
				break;

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

			case 'U':
				strcpy(job.download_url, optarg);
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
			case 'V':
				minr_log_path(optarg);
				break;
			case 's':
				job.skip_sort = true;
				break;
			case 'S':
				if (scancode_check())
					job.scancode_mode = true;
				else
					minr_log("scancode and jq must be present in the system to run -S option\n");
				break;
			case 'n':
				job.skip_csv_check = true;
				break;

			case 'k':
				job.skip_delete = true;
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
			
			case 'A':
				job.mine_all = true;
				break;

			case 'h':
				show_help();
				exit(EXIT_SUCCESS);
				break;

			case 'v':
				printf("minr-%s\n", MINR_VERSION);
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
	sprintf(job.mined_extra_path, "%s/extra", job.mined_path);
	/* Import mined/ into the LDB */
	if (*job.import_path)
	{
		if (!*job.import_table && job.import_overwrite)
		{
			minr_log( "Cannot overwrite all tables. Use of -O requires -I\n");
		}
		else
		{
			mined_import(&job);
		}
	}

	/* Join mined/ structures */
	else if (*job.join_from && *job.join_to) minr_join(&job);

	/* Mine local files */
	else if (job.local_mining != 0)
	{
		job.licenses = load_licenses(&job.license_count);
		if (is_file(job.url)) mine_local_file(&job, job.url);
		else if(is_dir(job.url)) mine_local_directory(&job,job.url);
		free(job.licenses);
	}

	/* Process mz file */
	else if (*job.mz)
	{
		if (!is_dir(job.mz) && !is_file(job.mz))
		{
			printf("Cannot access %s\n", job.mz);
			exit(EXIT_FAILURE);
		}

		char *file_path = calloc(MAX_PATH_LEN + 1, 1);

		/* Open all file handlers in mined/snippets (256 files) */
		if (is_dir(job.mz))
			wfp_init(job.mz);
		else
			wfp_init(job.mined_path);

		/* Import snippets from the entire sources/ directory */
		if (is_dir(job.mz))
		{
			for (int i = 0; i < MZ_FILES; i++)
			{
				sprintf(file_path, "%s/sources/%04x.mz", job.mz, i);
				if (file_size(file_path))
				{
					printf("%s\n", file_path);
					mz_wfp_extract(file_path);
				}
			}
		}

		/* Import snippets from a single file */
		else
		{
			mz_wfp_extract(job.mz);
		}
		
		free(file_path);
		wfp_free();

	}

	/* Mine URL */
	else if (*job.metadata && *job.url)
	{

		/* -d expects 6 CSV values: vendor,component,version,release_date,license,purl */
		if (count_chr(',', job.metadata) != 5)
		{
			printf("Wrong number of values passed with -d\n");
			printf("Expected vendor,component,version,release_date,license,purl\n");
			exit(EXIT_FAILURE);
		}

		if (!create_dir(job.mined_path))
		{
			printf("Cannot create output structure in %s\n", job.mined_path);
			exit(EXIT_FAILURE);
		}
		
		if (job.mine_all)
		{
			if (!create_dir(job.mined_extra_path))
			{
				printf("Cannot create output structure in %s\n", job.mined_path);
				exit(EXIT_FAILURE);
			}
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
	if (lib_encoder_present)
	{
		dlclose(lib_handle);
	}
	exit(exit_code);
}