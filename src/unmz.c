// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/unmz.c
 *
 * The unmz command extracts files from .mz archives
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
#include "blacklist.h"
#include "string.c"
#include "license_ids.c"
#include "license.c"
#include "copyright.c"
#include "quality.c"
#include "hex.c"
#include "file.c"
#include "md5.c"
#include "mz.c"

void help()
{
	printf("usage: unmz command mzfile\n");
	printf("-x MZ   extract all files from MZ file\n");
	printf("-l MZ   list directory of MZ file, validating integrity\n");
	printf("-c MZ   check MZ container integrity\n");
	printf("-s MZ   detect license (SPDX ID) in all file headers\n");
	printf("-a MZ   detect author (Copyright declaration) in all files\n");
	printf("-q MZ   extract code quality information from all files\n");
	printf("-k MD5  extracts file with id MD5 and display contents via STDOUT\n");
	printf("-p PATH specify mined/ directory (default: mined/)\n");
	printf("-v      print version\n");
	printf("-h      print this help\n");
}

bool validate_md5(char *txt)
{
    /* Check length */
    if (strlen(txt) != 32) { printf("f1\n");return false;}

    /* Check digits (and convert to lowercase) */
    for (int i = 0; i < 32; i++)
    {
        txt[i] = tolower(txt[i]);
        if (txt[i] > 'f') return false;
        if (txt[i] < '0') return false;
        if (txt[i] > '9' && txt[i] < 'a')  return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
	int exit_status = EXIT_SUCCESS;

	/* Parse arguments */
	int option;
	bool invalid_argument = false;
	char mined[MAX_PATH_LEN] = "\0";
	strcpy(mined, "mined");
	char key[33] = "\0";
	bool key_provided = false;

	char *src = calloc(MAX_FILE_SIZE + 1, 1);
	uint8_t *zsrc = calloc((MAX_FILE_SIZE + 1) * 2, 1);

	while ((option = getopt(argc, argv, ":p:k:c:x:l:a:q:s:hv")) != -1)
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
				strcpy(mined, optarg);
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
				if (!mz_check(optarg))
				{
					printf(".mz validation failed!\n");
					exit_status = EXIT_FAILURE;
				}
				break;
			
			case 'x':
				mz_extract(optarg, true, none, zsrc, src);
				break;

			case 'l':
				mz_extract(optarg, false, none, zsrc, src);
				break;

			case 'a':
				mz_extract(optarg, false, copyright, zsrc, src);
				break;

			case 'q':
				mz_extract(optarg, false, quality, zsrc, src);
				break;

			case 's':
				load_licenses();
				mz_extract(optarg, false, license, zsrc, src);
				break;

			case 'h':
				help();
				break;

			case 'v':
				printf("unmz is part of scanoss-minr-%s\n", MINR_VERSION);
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
	if (key_provided) mz_cat(mined, key, zsrc, src);

	free(src);
	free(zsrc);

	if (invalid_argument)
	{
		printf("Error parsing arguments\n");
	}

	return exit_status;
}
