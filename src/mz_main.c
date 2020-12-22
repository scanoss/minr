// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/mz_main.c
 *
 * The mz archive command tool
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
#include <zlib.h>
#include "minr.h"
#include "ldb.h"
#include "blacklist.h"
#include "mz.h"
#include "quality.h"
#include "mz_mine.h"
#include "license_ids.h"

void help()
{
	printf("The mz command is part of scanoss/minr and provides mz archive managing\n");
	printf("\n");
	printf("Usage:\n");
	printf("mz command/s [file.mz]\n");
	printf("\n");

	printf("Parameters:\n");
	printf("-x MZ   extract all files from MZ file\n");
	printf("-l MZ   list directory of MZ file, validating integrity\n");
	printf("-c MZ   check MZ file integrity\n");
	printf("-o MZ   optimise MZ (eliminate duplicates and unwanted content)\n");
	printf("-O MZ   optimise MZ, eliminating also orphan files (not found in local KB)\n");
	printf("\n");

	printf("Single file extraction to STDOUT:\n");
	printf("-k MD5  extracts file with id MD5 and display contents via STDOUT\n");
	printf("-p PATH specify mined/ directory (default: mined/)\n");
	printf("\n");

	printf("Data mining:\n");
	printf("-L MZ   detect license (SPDX ID) declarations (text and SPDX-License-Identifier tags)\n");
	printf("-C MZ   detect copyright declarations\n");
	printf("-Q MZ   extract code quality (best practices) information\n");
	printf("\n");

	printf("Help and version:\n");
	printf("-v      print version\n");
	printf("-h      print this help\n");
	exit(EXIT_SUCCESS);
}

bool validate_md5(char *txt)
{
    /* Check length */
    if (strlen(txt) != 32) return false;

    /* Check digits (and convert to lowercase) */
    for (int i = 0; i < 32; i++)
    {
        txt[i] = tolower(txt[i]);
        if (txt[i] > 'f') return false;
        if (txt[i] < '0') return false;
        if (txt[i] > '9' && txt[i] < 'a') return false;
    }
    return true;
}

/* Create an empty mz_job structure */
struct mz_job new_mz_job()
{
	struct mz_job job;
    job.path = NULL;
    job.mz = NULL;
    job.mz_ln = 0;
    job.id = NULL;
    job.ln = 0;
    job.dup_c = 0;
    job.bll_c = 0;
    job.orp_c = 0;
	job.md5[32] = 0;
	job.check_only = false;
	job.orphan_rm = false;
	job.key = NULL;

	// Uncompressed data buffer
    job.data = calloc(MAX_FILE_SIZE + 1, 1);
    job.data_ln = 0;

	// Compressed data
    job.zdata = calloc((MAX_FILE_SIZE + 1) * 2, 1);
    job.zdata_ln = 0;

	// Temporary data;
	job.ptr = NULL;
	job.ptr_ln = 0;

	return job;
}

void free_mz_job(struct mz_job job)
{
	free(job.data);
	free(job.zdata);
}

int main(int argc, char *argv[])
{
	int exit_status = EXIT_SUCCESS;

	/* Parse arguments */
	int option;
	bool invalid_argument = false;
	char mz_file[MAX_PATH_LEN] = "\0";
	strcpy(mz_file, "mined");
	char key[33] = "\0";
	bool key_provided = false;

	if (argc < 2)
	{
		printf("Missing parameters\n");
		exit(EXIT_FAILURE);	
	}

	char *src = calloc(MAX_FILE_SIZE + 1, 1);
	uint8_t *zsrc = calloc((MAX_FILE_SIZE + 1) * 2, 1);

	struct mz_job job;
	job.path = NULL;
	job.mz = NULL;
	job.mz_ln = 0;
	job.id = NULL;
	job.ln = 0;
	job.data = src;        // Uncompressed data
	job.data_ln = 0;
	job.zdata = zsrc;      // Compressed data
	job.zdata_ln = 0;
	job.ptr = NULL;        // Temporary data
	job.ptr_ln = 0;
	job.dup_c = 0;
	job.bll_c = 0;
	job.orp_c = 0;
	job.md5[32] = 0;
	job.check_only = false;
	job.orphan_rm = false;
	job.key = NULL;

	while ((option = getopt(argc, argv, ":p:k:c:x:l:C:Q:L:o:O:hv")) != -1)
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
			case 'p':
				strcpy(mz_file, optarg);
				break;

			case 'k':
				if (validate_md5(optarg))
				{
					key_provided = true;
					strcpy(key, optarg);
				}
				else
				{
					printf("Invalid key provided\n");
					exit_status = EXIT_FAILURE;
				}
				break;

			case 'c':
				job.check_only = true;
				mz_list(&job, optarg);
				break;

			case 'x':
				mz_extract(&job, optarg);
				break;

			case 'l':
				mz_list(&job, optarg);
				break;

			case 'C':
				mz_mine_copyright(&job, optarg);
				break;

			case 'Q':
				mz_mine_quality(&job, optarg);
				break;

			case 'o':
				mz_optimise(&job, optarg);
				break;

			case 'O':
				job.orphan_rm = true;
				mz_optimise(&job, optarg);
				break;

			case 'L':
				load_licenses();
				mz_mine_license(&job, optarg);
				break;

			case 'h':
				free(src);
				free(zsrc);
				help();
				break;

			case 'v':
				printf("mz is part of scanoss-minr-%s\n", MINR_VERSION);
				free(src);
				free(zsrc);
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

		if (invalid_argument)
		{
			exit_status = EXIT_FAILURE;
			break;
		}
	}

	for (;optind < argc-1; optind++)
	{
		printf("Invalid argument: %s\n", argv[optind]);
		invalid_argument = true;
	}

	/* Process -k request */
	if (key_provided) mz_cat(&job, mz_file, key);

	free(src);
	free(zsrc);

	if (invalid_argument)
	{
		printf("Error parsing arguments\n");
	}

	return exit_status;
}
