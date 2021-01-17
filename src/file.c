// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/file.c
 *
 * File handling functions
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


#include <sys/stat.h>
#include <openssl/md5.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <sys/types.h>
#include <unistd.h>


#include "minr.h"
#include "file.h"

bool is_file(char *path)
{

    struct stat pstat;
    if (!stat(path, &pstat)) if (S_ISREG(pstat.st_mode)) return true;
    return false;

}

/* Returns true if "path" is a directory */
bool is_dir(char *path)
{
    struct stat pstat;
    if (!stat(path, &pstat)) if (S_ISDIR(pstat.st_mode)) return true;
    return false;
}

bool create_dir(char *path)
{
  if (is_dir(path)) return true;
  if (mkdir(path, 0755)) return false;
  return true;
}
		
bool valid_path(char *dir, char *file)
{
	int path_len = strlen(dir) + 1 + strlen(file);
	return path_len < MAX_PATH_LEN;
}


/* Returns true if "path" is not a single nor a double dot */
bool not_a_dot (char *path)
{
	if (strcmp(path, ".") && strcmp(path, "..")) return true;
	return false;
}

/* Open 256 "file" descriptors */
FILE **open_file (char *mined_path)
{
	char *path = calloc(MAX_PATH_LEN, 1);

	/* Create files directory */
	sprintf(path, "%s/files", mined_path);
	if (!create_dir(path))
	{
		printf("Cannot create file %s\n", path);
		exit(EXIT_FAILURE);
	}

	/* Open all 256 files (append, text) */
	FILE **out = calloc(sizeof(FILE*) * 256, 1);
	for (int i=0; i < 256; i++)
	{
		sprintf(path, "%s/files/%02x.csv", mined_path, i);
		out[i] = fopen(path, "a");
		if (out[i] == NULL)
		{
			printf("Cannot open file %s\n", path);
			exit(EXIT_FAILURE);
		}
	}

	free(path);
	return out;
}

/* Open 256 "snippet" descriptors */
int *open_snippet (char *base_path)
{
	char *path = calloc(MAX_PATH_LEN, 1);

	/* Create files directory */
	sprintf(path, "%s/snippets", base_path);
	if (!create_dir(path))
	{
		printf("Cannot create file %s\n", path);
		exit(EXIT_FAILURE);
	}

	/* Open all 256 files (append, binary) */
	int *out = calloc(sizeof(int*) * 256, 1);
	for (int i=0; i < 256; i++)
	{
		sprintf(path, "%s/snippets/%02x.bin", base_path, i);
		out[i] = open(path, O_RDWR | O_APPEND | O_CREAT, 0644);
		if (out[i] < 0)
		{
			printf("Cannot open file %s\n", path);
			exit(EXIT_FAILURE);
		}
	}

	free(path);
	return out;
}

uint64_t file_size(char *path)
{
	/* Open file */
	int file = open(path, O_RDONLY);
	if (file < 0) return 0;

	/* Obtain file size */
	uint64_t size = lseek64(file, 0, SEEK_END);
	close(file);

	return size;
}

bool check_disk_free(char *file, uint64_t needed)
{
	struct statvfs stat;
	if (statvfs(file, &stat) != 0)
	{
		printf("Cannot check available disk space\n");
		return false;
	}

	if (stat.f_bsize * stat.f_bavail < (needed * 2))
	{
		printf("Not enough free disk space to perform the operation\n");
		return false;
	}
	return true;
}
