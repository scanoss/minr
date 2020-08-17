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

#include "global.c"
#include "hex.c"
#include "file.c"
#include "md5.c"
#include "mz.c"

#define VERSION "1.04"

void help()
{
	printf("unmz-%s\n", VERSION);
	printf("usage: unmz command mzfile\n");
	printf("-x extract files\n");
	printf("-l list files, validating integrity\n");
	printf("-c check .mz container integrity\n");
	printf("-h print this help\n");
}

int main(int argc, char *argv[])
{
	int exit_status = EXIT_SUCCESS;

	/* Parse arguments */
	int option;
	bool invalid_argument = false;

	char *src = calloc(MAX_FILE_SIZE + 1, 1);
	uint8_t *zsrc = calloc((MAX_FILE_SIZE + 1) * 2, 1);

	while ((option = getopt(argc, argv, ":x:l:c:h")) != -1)
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
			case 'h':
				help();
				break;

			case 'x':
				mz_extract(optarg, true, zsrc, src);
				break;

			case 'l':
				mz_extract(optarg, false, zsrc, src);
				break;

			case 'c':
				if (!mz_check(optarg))
				{
					printf(".mz validation failed!\n");
					exit_status = EXIT_FAILURE;
				}
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

	free(src);
	free(zsrc);
	return exit_status;
}
