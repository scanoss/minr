// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/file.c
 *
 * File handling functions
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
  * @file file.c
  * @date 7 Feb 2021
  * @brief Utilities functions to process files.
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
#include "minr_log.h"
/**
 * @brief Verify if a path is a file and exists.
 * 
 * @param path Path to verify.
 * @return true the path is a file, false otherwise. 
 */
bool is_file(char *path)
{
    struct stat pstat;
	
    if (!stat(path, &pstat))
	{
		if (pstat.st_mode == 33024)
		{
			minr_log("Warning the file %s will be ignored - st_mode = 33024\n", path);
			return false;
		}
		if (S_ISREG(pstat.st_mode)) 
		{
			/*discard hidden files */
			char * file_name = strrchr(path, '/');
			if (file_name && file_name[1] == '.')
				return false;
			return true;
		}
	}
    return false;

}

/**
 * @brief Verify if a path is a directory and exists.
 * 
 * @param path Path to verify. 
 * @return true The path is a directory, false otherwise.
 */
bool is_dir(char *path)
{
    struct stat pstat;
    if (!stat(path, &pstat)) if (S_ISDIR(pstat.st_mode)) return true;
    return false;
}

/**
 * @brief Creates a directory with 0755 permissions.
 * If the folder already exists the folder is not replaced.
 * 
 * @param path Path to the folder to create.
 * @return true Folder created or already exists. False in error case.
 */
bool create_dir(char *path)
{
  if (is_dir(path)) return true;
  if (mkdir(path, 0755)) return false;
  return true;
}

/**
 * @brief Checks if a concatenation of path and filename exists not exceeds the max_path_length.
 * 
 * @param dir 
 * @param file 
 * @return true valid path. False otherwise.
 */
bool valid_path(char *dir, char *file)
{
	int path_len = strlen(dir) + 1 + strlen(file);
	return path_len < MAX_PATH_LEN;
}



/**
 * @brief Returns true if "path" is not a single nor a double dot 
 * 
 * @param path 
 * @return true 
 */
bool not_a_dot (char *path)
{
	if (strcmp(path, ".") && strcmp(path, "..")) return true;
	return false;
}

/**
 * @brief Open 256 "file" descriptors
 * 
 * @param mined_path 
 * @return FILE** 
 */
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


/**
 * @brief Open 256 "snippet" descriptors
 * 
 * @param base_path 
 * @return int* 
 */
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

/**
 * @brief Gets the size of a file.
 * 
 * @param path Path to the file. 
 * @return uint64_t Size of the file in bytes. 
 */
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

/**
 * @brief Verify if there is enough space to store X bytes in a disk.
 * 
 * @param file the pathname of any file within the mounted filesystem
 * @param needed space needed in bytes
 * @return true 
 */
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
