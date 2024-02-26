// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/mz_main.c
 *
 * The mz archive command tool
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
  * @file mz_main.c
  * @date 2 Sep 2021 
  * @brief Implement MZ interpreter
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
#include <zlib.h>
#include "minr.h"
#include <ldb.h>
#include "ignorelist.h"
#include "quality.h"
#include "mz_mine.h"
#include "crypto.h"
#include "mz.h"

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
	printf("-D MZ   optimise MZ (eliminate only duplicates)\n");
	printf("-K MZ   extract a list of unique file keys to STDOUT (binary)\n");
	printf("-X KEYS exclude list of KEYS (see -K) when running optimize (-O and -o)\n");
	printf("\n");

	printf("Single file extraction to STDOUT:\n");
	printf("-k MD5  extracts file with id MD5 and display contents via STDOUT\n");
	printf("-p PATH specify mined/ directory (default: mined/)\n");
	printf("\n");

	printf("Data mining:\n");
	printf("-L MZ   detect license (SPDX ID) declarations (text and SPDX-License-Identifier tags)\n");
	printf("-Y MZ   detect cryptographic algorithms usage\n");
	printf("-C MZ   detect copyright declarations\n");
	printf("-Q MZ   extract code quality (best practices) information\n");
	printf("\n");

	printf("Help and version:\n");
	printf("-v      print version\n");
	printf("-h      print this help\n");
	exit(EXIT_SUCCESS);
}

/**
 * @brief Check the characters are in the range of a hexadecimal number
 * 
 * @param txt 
 * @return true 
 * @return false 
 */
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

/**
 * @brief Copy arg into str (exit if argument is too long)
 * 
 * @param str 
 * @param arg 
 */
void argcpy(char *str, char *arg)
{
	if (strlen(arg) >= MAX_ARG_LEN)
	{
		printf("[ERROR_LONG_ARG] %s\n", arg);
		exit(EXIT_FAILURE);
	}
	strcpy(str, arg);
}

int main(int argc, char *argv[])
{
	int exit_status = EXIT_SUCCESS;

	/* Parse arguments */
	int option;
	bool invalid_argument = false;
	char key[33] = "\0";
	bool key_provided = false;
	bool run_optimise = false;

	/* Check if parameters are present */
	if (argc < 2)
	{
		printf("Missing parameters\n");
		exit(EXIT_FAILURE);	
	}

	struct mz_job job;
	*job.path = 0;
	memset(job.mz_id, 0, 2);
	job.mz = NULL;
	job.mz_ln = 0;
	job.id = NULL;
	job.ln = 0;
	job.data = NULL;        // Uncompressed data
	job.data_ln = 0;
	job.zdata = NULL;      // Compressed data
	job.zdata_ln = 0;
	job.ptr = NULL;        // Temporary data
	job.ptr_ln = 0;
	job.dup_c = 0;
	job.igl_c = 0;
	job.orp_c = 0;
	job.min_c = 0;
	job.md5[32] = 0;
	job.check_only = false;
	job.dump_keys = false;
	job.orphan_rm = false;
	job.key = NULL;
	job.xkeys = NULL;
	job.xkeys_ln = 0;
	job.licenses = NULL;
	job.license_count = 0;
	
	while ((option = getopt(argc, argv, ":p:k:c:x:K:l:C:Q:L:o:D:O:Y:X:hv")) != -1)
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
				argcpy(job.path, optarg);
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

			case 'X':
				job.xkeys = file_read(optarg, &job.xkeys_ln);
				break;

			case 'c':
				job.check_only = true;
				argcpy(job.path, optarg);
				mz_list_check(&job);
				break;

			case 'x':
				argcpy(job.path, optarg);
				mz_extract(&job);
				break;

			case 'K':
				job.dump_keys = true;
				argcpy(job.path, optarg);
				mz_list_check(&job);
				break;

			case 'l':
				argcpy(job.path, optarg);
				mz_list_check(&job);
				break;

			case 'C':
				argcpy(job.path, optarg);
				mz_mine_copyright(&job);
				break;

			case 'Q':
				argcpy(job.path, optarg);
				mz_mine_quality(&job);
				break;

			case 'o':
				run_optimise = true;
				argcpy(job.path, optarg);
				break;

			case 'O':
				run_optimise = true;
				job.orphan_rm = true;
				argcpy(job.path, optarg);
				break;
			case 'D':
				argcpy(job.path, optarg);
				mz_optimise(&job, MZ_OPTIMISE_DUP);
				break;

			case 'L':
				job.licenses = load_licenses(&job.license_count);
				argcpy(job.path, optarg);
				mz_mine_license(&job);
				free(job.licenses);
				break;
			case 'Y':
				argcpy(job.path, optarg);
				load_crypto_definitions();
				mz_mine_crypto(&job);
				clean_crypto_definitions();
				break;

			case 'h':
				help();
				break;

			case 'v':
				printf("mz is part of scanoss-minr-%s\n", MINR_VERSION);
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

	/* Process -O and -o requests */
	if (run_optimise) mz_optimise(&job, MZ_OPTIMISE_ALL);

	/* Process -k request */
	else if (key_provided) mz_cat(&job, key);

	if (invalid_argument)
	{
		printf("Error parsing arguments\n");
	}

	return exit_status;
}
