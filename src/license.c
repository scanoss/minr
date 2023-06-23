// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/license.c
 *
 * SCANOSS License detection subroutines
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
  * @date 2 Sep 2021 
  * @brief Implements functions for license processing
  */

#include <libgen.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include "minr.h"
#include "license.h"

bool is_file(char *path);
bool is_dir(char *path);
bool not_a_dot(char *path);
void normalize_src(char *src, uint64_t src_ln, char *out, int max_in, int max_out);
bool strn_icmp(char *a, char *b, int len);

/**
 * @brief Check if SPDX-License-Identifier is found at *src
 * @param src 
 * @return true 
 * @return false 
 */
bool is_spdx_license_identifier(char *src)
{
	int tag_len = 24; // length of "SPDX-License-Identifier:"
	return strn_icmp(src, "SPDX-License-Identifier:", tag_len);
}

/**
 * @brief Returns pointer to SPDX-License-Identifier tag (null if not found)
 * 
 * @param src source string
 * @return char* 
 */
char *spdx_license_identifier(char *src)
{
	int tag_len = 24; // length of "SPDX-License-Identifier:"
	char *s = src + tag_len;
	char license[MAX_LICENSE_TEXT] = "\0";
	/* Skip until SPDX License starts */
	while (*s)
	{
		if (isalpha(*s))
			break;
		if (*s == '\n' || *s == '\r')
			return NULL;
		s++;
	}
	char * line_end = strchr(s,'\n');
	if (!line_end || (line_end - s) > MAX_LICENSE_TEXT -1)
	{
		return NULL;
	}
	
	char * line = strndup(s, line_end - s);

	char * and = NULL;
	char * or = NULL;
	
	if (!line)
		return NULL;
	s = line;
	/* End string at end of tag */
	do
	{
		char *out = s;
		if (!s)
			break;
		and = strcasestr(s, " AND ");
		or = strcasestr(s, " OR ");
		if (and && or)
		{
			if (and < or)
			{
				s = and + 5;
				strncat(license, out, and - out);
			}
			else
			{
				s = or + 4;
				strncat(license, out, or - out);
			}
		}
		else if (and)
		{
			s = and + 5;
			strncat(license, out, and - out);
		}
		else if (or)
		{
			s = or + 4;
			strncat(license, out, or - out);
		}
		else
			strcat(license, s);
		
		strcat(license, "#");
	} while ( and || or);
	
	free(line);
	return strdup(license);
}

/**
 * @brief Return a pointer to the SPDX-License-Identifier if found in src header.
 * 
 * @param src source string
 * @param src_ln source lenght
 * @return char* pointer to spdx
 */
char *mine_spdx_license_identifier(char *src, uint64_t src_ln)
{
	/* Max bytes/lines to analyze */
	int max_bytes = MAX_FILE_HEADER;
	if (src_ln < max_bytes)
		max_bytes = src_ln;
	int max_lines = MAX_HEADER_LINES;
	int line = 0;

	char *s = src;
	while (*s)
	{
		if (*s == '\n')
			line++;
		else if (is_spdx_license_identifier(s))
		{
			char *license = spdx_license_identifier(s);

			return license;
		}
		if (((s++) - src) > max_bytes || line > max_lines)
			return NULL;
	}
	return NULL;
}

/**
 * @brief Normalize the license in *path and output license definition lines for license_ids.c
 * 
 * @param path path to file
 * @param counter extern counter
 */
void normalize_license(char *path, int counter)
{
	/* Open file */
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return;

	/* Obtain file size */
	uint64_t size = lseek64(fd, 0, SEEK_END);
	if (!size)
		return;

	/* Read file header to memory */
	if (size > MAX_FILE_HEADER)
		size = MAX_FILE_HEADER;
	char *src = malloc(size + 1);
	lseek64(fd, 0, SEEK_SET);
	if (!read(fd, src, size))
	{
		free(src);
		return;
	}

	char normalized[MAX_FILE_HEADER];
	src[size] = 0;
	normalize_src(src, size, normalized, MAX_FILE_HEADER - 1, MAX_LICENSE_TEXT - 1);

	printf("  strcpy(licenses[%d].spdx_id, \"%s\");\n", counter, basename(dirname(path)));
	printf("  strcpy(licenses[%d].text,\"%s\");\n", counter, normalized);
	printf("  licenses[%d].ln = %ld;\n\n", counter, strlen(normalized));

	close(fd);
	free(src);
}

/**
 * @brief Return the number of files present in the path dir structure
 * 
 * @param path dir path
 * @param count [out] number of files
 * @return number of files
 */

int count_files(char *path, int *count)
{
	char newpath[MAX_PATH_LEN];

	/* Open directory */
	struct dirent *dp;
	DIR *dir = opendir(path);
	if (!dir)
		return *count;

	while ((dp = readdir(dir)) != NULL)
	{
		if (not_a_dot(dp->d_name))
		{
			sprintf(newpath, "%s/%s", path, dp->d_name);
			if (is_file(newpath))
				(*count)++;
			else if (is_dir(newpath))
				*count = count_files(newpath, count);
		}
	}
	closedir(dir);
	return *count;
}

/**
 * @brief Recurse the provided directory
 * 
 * @param path string path
 * @param counter[out] number of files
 * @return int number of files
 */
int recurse_dir(char *path, int *counter)
{
	char newpath[MAX_PATH_LEN];

	/* Open directory */
	struct dirent *dp;
	DIR *dir = opendir(path);
	if (!dir)
		return *counter;

	while ((dp = readdir(dir)) != NULL)
	{
		if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
		{
			sprintf(newpath, "%s/%s", path, dp->d_name);
			if (is_file(newpath))
			{
				normalize_license(newpath, *counter);
				(*counter)++;
			}
			else if (is_dir(newpath))
			{
				*counter = recurse_dir(newpath, counter);
			}
		}
	}
	closedir(dir);
	return *counter;
}

/**
 * @brief Auto-generate a license_ids.c file with normalized license texts for license detection
 * 
 * @param path string path
 */
void generate_license_ids_c(char *path)
{
	int count = 0;
	count = count_files(path, &count);

	printf("// This file is autogenerated by SCANOSS Minr, available at https://github.com/scanoss/minr\n\n#define MAX_LICENSE_ID 64\n#define MAX_LICENSE_TEXT 1024\n#include <string.h>\n#include <stdlib.h>\n\ntypedef struct normalized_license\n{\n  char spdx_id[MAX_LICENSE_ID];\n  char text[MAX_LICENSE_TEXT];\n  int ln;\n} normalized_license;\n\nnormalized_license *load_licenses(int *count)\n{\n  normalized_license *licenses = calloc(%d + 1, sizeof(normalized_license));\n\n", count);

	int counter = 0;
	recurse_dir(path, &counter);

	printf("\n  *count = %d;\n  return licenses;\n}\n", counter);
}

/**
 * @brief Return true if *l is entirely found at *s
 * 
 * @param s input string
 * @param l input string
 * @return true 
 * @return false 
 */
bool license_cmp(char *s, char *l)
{
	while (*s && *l)
		if (*(l++) != *(s++))
			return false;
	if (!*l)
		return true;
	return false;
}

/**
 * @brief Attempt license detection in the header of *src
 * 
 * @param src input buffer
 * @param src_ln input buffer lenght
 * @param licenses pointer to license list
 * @param license_count number of licenses
 * @return char* pointer to found license
 */
char *mine_license_header(char *src, uint64_t src_ln, normalized_license *licenses, int license_count)
{
	/* Max bytes/lines to analyze */
	int max_bytes = MAX_FILE_HEADER - 1;
	if (src_ln < max_bytes)
		max_bytes = src_ln;

	char normalized[MAX_LICENSE_TEXT];
	src[max_bytes] = 0;
	normalize_src(src, src_ln, normalized, max_bytes, MAX_LICENSE_TEXT - 1);

	char *s = normalized;
	while (*s)
	{
		for (int i = 0; i < license_count; i++)
			if (license_cmp(s, licenses[i].text))
				return licenses[i].spdx_id;
		s++;
	}
	return NULL;
}

/**
 * @brief License sources
 *    	0 = Declared in component
 *   	1 = Declared in file with SPDX-License-Identifier
 *    	2 = Detected in header
 *  	3 = Declared in LICENSE file
 * 		4 = external scancode detection
 * 		5 = internal scancode detection
 * @param job pointer to minr job
 * @param id ID string
 * @param license_file tru for license files
 */
void mine_license(struct minr_job *job, char *id, bool license_file)
{
	FILE *fp;

	/* Assemble csv path */
	char csv_path[MAX_PATH_LEN] = "\0";
	if (!job->local_mining)
	{
		strcpy(csv_path, job->mined_path);
		strcat(csv_path,"/"TABLE_NAME_LICENSE".csv");
	}

	/* SPDX license tag detection */
	char *license = mine_spdx_license_identifier(job->src, job->src_ln);
	
	if (license)
	{
		char * lic = strtok(license,"#");
		while (lic)
		{		
			char * l = lic + strlen(lic);
			/* Eliminate trailing punctuation */
			while (l > lic)
			{
				if (*l != '+' && !isalnum(*l))
					*l = 0;
				else
					break;
				l--;
			}

			if (!job->local_mining)
			{
				fp = fopen(csv_path, "a");
				fprintf(fp, "%s,%d,%s\n", id, license_file ? 3 : 1, lic);
				fclose(fp);
			}
			else
				printf("%s,%s\n", id, lic);
			lic = strtok(NULL,"#");
		}
		free(license);
	}

	/* License header detection */
	else
	{
		license = mine_license_header(job->src, job->src_ln, job->licenses, job->license_count);
		if (license)
		{
			char license_source[] = "2";
			if (license_file)
				*license_source = '3';

			if (!job->local_mining)
			{
				fp = fopen(csv_path, "a");
				fprintf(fp, "%s,%s,%s\n", id, license_source, license);
				fclose(fp);
			}
			else
				printf("%s,%s\n", id, license);
		}
	}
}
