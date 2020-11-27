// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/main.c
 *
 * SCANOSS Mining Tool
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

#define _GNU_SOURCE
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
#include "blacklist.h"
#include "blacklist_wfp.h"
#include "external/wfp/winnowing.c"
#include "bsort.c"
#include "minr.h"
#include "file.c"
#include "string.c"
#include "license_ids.c"
#include "license.c"
#include "copyright.c"
#include "quality.c"
#include "md5.c"
#include "hex.c"
#include "mz.c"
#include "minr.c"
#include "help.c"
#include "wfp.c"
#include "import.c"
#include "join.c"

void url_download(char *target, char *metadata, char *tmp_path, bool all_extensions, bool exclude_mz)
{
	char *src = calloc(MAX_FILE_SIZE + 1, 1);
	uint8_t *zsrc = calloc((MAX_FILE_SIZE + 1) * 2, 1);
	struct mz_cache_item *mz_cache = NULL;

	/* Reserve memory for snippet fingerprinting */
	if (!exclude_mz)
	{
		buffer = malloc(BUFFER_SIZE * 256);
		hashes = malloc(MAX_FILE_SIZE);
		lines  = malloc(MAX_FILE_SIZE);

		/* Reserve memory for mz_cache for mined/sources (65536 files) */
		mz_cache = malloc(MZ_FILES * sizeof(struct mz_cache_item));
		for (int i = 0; i < MZ_FILES; i++) mz_cache[i].length = 0;
	}

	/* Open all file handlers in mined/files (256 files) */
	out_file = open_file();

	char *tmp = calloc(MAX_PATH_LEN,1);
	sprintf(tmp, "%s/components.csv", mined_path);
	out_component = fopen(tmp, "a");
	if (!out_component)
	{
		printf("Cannot create file %s\n", tmp);
		free(tmp);
		exit(EXIT_FAILURE);
	}
	free(tmp);

	/* Create temporary component directory */
	char *tmp_component = malloc(MAX_PATH_LEN);
	sprintf(tmp_component,"%s/minr-%d", tmp_path, getpid());
	mkdir(tmp_component, 0755);

	/* urlid will contain the hex md5 of the entire component */
	char *urlid = calloc(33,1);
	bool downloaded = download(tmp_component, target, urlid);

	/* Process downloaded/expanded directory */
	if (is_dir(tmp_component) && downloaded)
	{
		char *component_record = malloc(MAX_PATH_LEN);
		sprintf(component_record, "%s,%s,%s",urlid, metadata, target);

		recurse(component_record, tmp_component, tmp_component, all_extensions, exclude_mz, urlid, src, zsrc, mz_cache);
		FILE *fp;
		char *command = malloc(MAX_PATH_LEN);
		sprintf(command, "rm -rf %s", tmp_component);
		fp = popen(command, "r");
		free(command);
		fclose(fp);

		free(component_record);
	}

	else
	{
		printf("Capture failed\n");
		exit(EXIT_FAILURE);
	}

	free(urlid);
	free(tmp_component);

	fclose(out_component);

	/* Close files */
	for (int i=0; i < 256; i++) fclose(out_file[i]);

	if (!exclude_mz)
	{
		/* Flush mz cache */
		mz_flush(mz_cache);

		free(mz_cache);
		free(buffer);
		free(hashes);
		free(lines);
	}

	free(out_file);
	free(src);
	free(zsrc);
}

int main(int argc, char *argv[])
{
	if (!check_dependencies()) exit(1);
	int exit_code = EXIT_SUCCESS;
	bool all_extensions = false;
	bool exclude_mz = false;
	bool skip_sort = false;
	bool erase_after = false;

	char import_path[MAX_PATH_LEN]="\0";
	char join_from[MAX_PATH_LEN] = "\0";
	char join_to[MAX_PATH_LEN] = "\0";
	char metadata[MAX_PATH_LEN] = "\0";
	char url[MAX_PATH_LEN] = "\0";
	char mz[MAX_PATH_LEN] = "\0";

	/* Parse arguments */
	int option;
	bool invalid_argument = false;

	while ((option = getopt(argc, argv, ":o:m:g:w:t:f:T:i:l:z:u:d:xseahv")) != -1)
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
				strcpy(mined_path, optarg);
				if (!create_dir(mined_path))
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
				strcpy(join_to, optarg);
				break;

			case 'f':
				strcpy(join_from, optarg);
				break;

			case 'T':
				strcpy(tmp_path, optarg);
				break;

			case 'i':
				strcpy(import_path, optarg);
				break;

			case 'l':
				generate_license_ids_c(optarg);
				exit(EXIT_SUCCESS);

			case 'z':
				strcpy(mz, optarg);
				break;

			case 'u':
				strcpy(url, optarg);
				break;

			case 'd':
				strcpy(metadata, optarg);
				break;

			case 'e':
				erase_after = true;
				break;

			case 's':
				skip_sort = true;
				break;

			case 'x':
				exclude_mz = true;
				break;

			case 'a':
				all_extensions = true;
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

	strcat(mined_path, "/mined");

	/* Import mined/ into the LDB */
	if (*import_path) mined_import(import_path, skip_sort, erase_after);

	/* Join mined/ structures */
	else if (*join_from && *join_to) minr_join(argv[2], argv[4]);

	/* Process mz file */
	else if (*mz)
	{
		if (!is_dir(mz))
		{
			printf("Cannot access directory %s\n", mz);
			exit(EXIT_FAILURE);
		}

		buffer = malloc(BUFFER_SIZE * 256);
		hashes = malloc(MAX_FILE_SIZE);
		lines  = malloc(MAX_FILE_SIZE);

		/* Open all file handlers in mined/snippets (256 files) */
		out_snippet = open_snippet(mz);

		/* Import snippets */
		for (int i = 0; i < MZ_FILES; i++)
		{
			char *file_path = calloc(2 * MAX_PATH_LEN, 1);
			sprintf(file_path, "%s/sources/%04x.mz", mz, i);
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
	else if (*metadata && *url)
	{

		if (!create_dir(mined_path))
		{
			printf("Cannot create output structure in %s\n", mined_path);
			exit(EXIT_FAILURE);
		}

		url_download(url, metadata, tmp_path, all_extensions, exclude_mz);

	}
	else
	{
		exit_code = EXIT_FAILURE;
		printf("Invalid input. Please try -h\n");
	}

	exit(exit_code);
}

